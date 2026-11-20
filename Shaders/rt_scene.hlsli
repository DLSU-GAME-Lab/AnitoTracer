#ifndef RT_SCENE_HLSLI
#define RT_SCENE_HLSLI

// Lets a ray shade what it hits. A rasterized pixel gets its surface from the mesh being drawn, but a ray can land
// on any triangle of any model, so everything needed to light that point has to be reachable from here:
//
//   g_SceneVertices / g_SceneIndices   every loaded model's geometry in two buffers (SceneGeometryPool)
//   g_SceneGeometry                    one record per (model instance, submesh), rebuilt each frame with the TLAS
//                                      (SceneTLAS): where the submesh's triangles are, and its material
//   g_SceneTextures                    every material texture, in one array
//
// A hit reports its instance, geometry (submesh) and triangle. The instance's ID is the index of its first record,
// so InstanceID + GeometryIndex is the record, which leads to the three vertices and the material.
//
// Only compiled when the pipeline defines RT_SCENE_SHADING (HybridPipeline::GetRayTracingMacros): the device must be
// able to pick a texture from an array per pixel. Included by hybrid_shading.hlsli, after the sky lighting functions it uses.
// Requires light_struct.hlsli, pbr_defs.hlsli and rt_shadows.hlsli.

#include "rt_scene_data.hlsli"

// The surface point a ray landed on
struct SceneHit
{
    float Distance;         // Along the ray
    float3 Position;        // World space
    float3 N;               // Interpolated vertex normal, turned to the side the ray arrived on
    float3 GeoN;            // The triangle's own normal, same side. Used to offset rays that start here.
    float2 UV;
    float UVPerWorldArea;   // The triangle's area in UV space over its area in the world: how densely its texture covers it
    bool Shadeable;         // False when the triangle's data isn't available. Only Distance and Position are meaningful then.
    bool BackFacing;        // The ray arrived on the side the surface's normal points away from: from inside a closed object
    SceneGeometryRecord Material;
};

// How a ray treats glass (blended materials)
static const uint GLASS_IGNORE = 0u;    // Goes straight through: glass isn't seen
static const uint GLASS_SOLID = 1u;     // A surface like any other: the ray stops there
static const uint GLASS_TINT = 2u;      // Goes through, and what the ray finds beyond is lit by the glass's color (see `tint`)

// Panes of glass a GLASS_TINT ray keeps track of
static const uint MAX_TRACKED_PANES = 4u;

// Finds the closest surface along a ray. Returns false when the ray leaves the scene without hitting anything.
// Cutout materials are solid where their alpha reaches the cutoff and see-through elsewhere. Glass is handled as
// glassMode says; in GLASS_TINT mode `tint` is the color of the panes the ray crossed before the hit (white without any),
// which whatever the ray brings back has to be multiplied by.
bool TraceSceneRay(float3 origin, float3 direction, float maxDistance, uint glassMode, out float3 tint, out SceneHit hit)
{
    hit = (SceneHit) 0;
    tint = float3(1.0, 1.0, 1.0);

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = maxDistance;

    bool found;

#ifdef RT_ALPHA_TEST
    // Without the project's switch every triangle is solid, as before cutouts and glass
    RayQuery < RAY_FLAG_NONE > q;
    q.TraceRayInline(g_TLAS, (g_TransparencyFlags & TRANSPARENCY_TEST_RAYS) != 0u ? RAY_FLAG_NONE : RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);

    // The panes the ray went through. Triangles are met in no particular order, and one beyond the final hit
    // mustn't tint anything, so they are only applied once the hit is known.
    float paneDistance[MAX_TRACKED_PANES];
    float3 paneTint[MAX_TRACKED_PANES];
    uint paneCount = 0u;

    while (q.Proceed())
    {
        // Triangles flagged opaque are final without being asked; these are cutouts and glass
        if (q.CandidateType() != CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            continue;
        }

        SceneGeometryRecord candidate = g_SceneGeometry[q.CandidateInstanceID() + q.CandidateGeometryIndex()];
        uint primitive = q.CandidatePrimitiveIndex();
        float2 bary = q.CandidateTriangleBarycentrics();

        if ((candidate.Flags & SCENE_GEOMETRY_BLEND) != 0u)
        {
            if ((g_TransparencyFlags & TRANSPARENCY_GLASS) == 0u || glassMode == GLASS_IGNORE)
            {
                continue;
            }
            if (glassMode == GLASS_SOLID)
            {
                q.CommitNonOpaqueTriangleHit();
            }
            else if (paneCount < MAX_TRACKED_PANES)
            {
                paneDistance[paneCount] = q.CandidateTriangleRayT();
                paneTint[paneCount] = GlassTransmittanceAt(candidate, primitive, bary);
                ++paneCount;
            }
            continue;
        }

        if (SceneCutoutBlocks(candidate, primitive, bary))
        {
            q.CommitNonOpaqueTriangleHit();
        }
    }

    found = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
    float endDistance = found ? q.CommittedRayT() : maxDistance;
    for (uint pane = 0u; pane < paneCount; ++pane)
    {
        if (paneDistance[pane] < endDistance)
        {
            tint *= paneTint[pane];
        }
    }

#else
    // The scene has no cutouts or glass: every triangle is solid, and the ray takes the fastest path there is
    RayQuery < RAY_FLAG_FORCE_OPAQUE > q;
    q.TraceRayInline(g_TLAS, RAY_FLAG_NONE, 0xFF, ray);
    while (q.Proceed())
    {
    }
    found = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
#endif

    if (!found)
    {
        return false;
    }

    hit.Distance = q.CommittedRayT();
    hit.Position = origin + direction * hit.Distance;
    hit.N = -direction;
    hit.GeoN = -direction;
    hit.Material = g_SceneGeometry[q.CommittedInstanceID() + q.CommittedGeometryIndex()];
    hit.Shadeable = (hit.Material.Flags & SCENE_GEOMETRY_IN_POOL) != 0u;
    if (!hit.Shadeable)
    {
        return true;
    }

    // The triangle's three vertices
    uint firstIndex = hit.Material.FirstIndex + q.CommittedPrimitiveIndex() * 3u;
    SceneVertex v0 = g_SceneVertices[g_SceneIndices[firstIndex] + hit.Material.BaseVertex];
    SceneVertex v1 = g_SceneVertices[g_SceneIndices[firstIndex + 1u] + hit.Material.BaseVertex];
    SceneVertex v2 = g_SceneVertices[g_SceneIndices[firstIndex + 2u] + hit.Material.BaseVertex];

    float2 bary = q.CommittedTriangleBarycentrics();
    float3 weights = float3(1.0 - bary.x - bary.y, bary.x, bary.y);

    float2 uv0 = float2(v0.U, v0.V);
    float2 uv1 = float2(v1.U, v1.V);
    float2 uv2 = float2(v2.U, v2.V);
    hit.UV = uv0 * weights.x + uv1 * weights.y + uv2 * weights.z;

    // Object -> world for directions; normals go through the inverse transpose so non-uniform scale keeps them perpendicular
    float3x3 objectToWorld = (float3x3) q.CommittedObjectToWorld3x4();
    float3x3 worldToObject = (float3x3) q.CommittedWorldToObject3x4();

    float3 edge1 = mul(objectToWorld, v1.Position - v0.Position);
    float3 edge2 = mul(objectToWorld, v2.Position - v0.Position);
    float3 faceNormal = cross(edge1, edge2); // Length is twice the triangle's world area
    float worldArea2 = length(faceNormal);
    if (worldArea2 > 0.0)
    {
        hit.GeoN = faceNormal / worldArea2;
    }

    float2 uvEdge1 = uv1 - uv0;
    float2 uvEdge2 = uv2 - uv0;
    float uvArea2 = abs(uvEdge1.x * uvEdge2.y - uvEdge1.y * uvEdge2.x);
    hit.UVPerWorldArea = uvArea2 / max(worldArea2, 1e-12);

    float3 objectNormal = v0.Normal * weights.x + v1.Normal * weights.y + v2.Normal * weights.z;
    float3 worldNormal = mul(objectNormal, worldToObject);
    float normalLength = length(worldNormal);
    hit.N = normalLength > 1e-6 ? worldNormal / normalLength : hit.GeoN;
    // The surface faces away from the ray: it came from inside a closed object, such as the far side of a glass ball
    hit.BackFacing = dot(hit.N, direction) > 0.0;

    // Surfaces are two-sided for rays: use the side the ray arrived on
    if (dot(hit.GeoN, direction) > 0.0)
    {
        hit.GeoN = -hit.GeoN;
    }
    if (dot(hit.N, hit.GeoN) < 0.0)
    {
        hit.N = -hit.N;
    }

    return true;
}


// Same, with glass left out of the picture: rays go through it (reflections and indirect light don't show glass)
bool TraceSceneRay(float3 origin, float3 direction, float maxDistance, out SceneHit hit)
{
    float3 tint;
    return TraceSceneRay(origin, direction, maxDistance, GLASS_IGNORE, tint, hit);
}

// Samples a material texture at a ray hit.
// A pixel shader picks the mip from how fast the UV changes between neighbouring pixels. A ray hit has no
// neighbours, so the mip comes from how many texels the ray's footprint covers instead; without this, distant
// reflections would always read the sharpest mip and shimmer.
// footprint: width of the ray's cone where it meets the surface, in world units.
float4 SampleSceneTexture(int slot, float2 uv, float uvPerWorldArea, float footprint)
{
    uint width, height, mips;
    g_SceneTextures[NonUniformResourceIndex(slot)].GetDimensions(0, width, height, mips);

    // Texels per world unit along the surface, times the footprint's width
    float texelsAcross = footprint * sqrt(uvPerWorldArea * float(width) * float(height));
    float lod = clamp(log2(max(texelsAcross, 1.0)), 0.0, float(mips) - 1.0);
    return g_SceneTextures[NonUniformResourceIndex(slot)].SampleLevel(g_SceneTextures_sampler, uv, lod);
}

// One shadow ray from a ray hit toward a light.
// spread: the light's radius as seen from the hit, per unit of distance (tan of its angular radius).
float TraceHitShadow(SceneHit hit, float3 L, float maxDistance, float spread, float2 noise)
{
    float3 dir = L;

    // While frames are averaged (by a denoiser or TAA), a different point of the light is tested each frame, which
    // adds up to a soft edge. A single still ray can't be soft, so otherwise it aims at the light's centre.
    if (RaysAveraged())
    {
        float3 t, b;
        BuildBasis(L, t, b);
        float2 d = VogelDisk(0, 1, noise.x * 2.0 * PI, noise.y) * spread;
        dir = normalize(L + t * d.x + b * d.y);
    }

    // The hit position is rebuilt from the ray, so it drifts off the surface a little more the farther the ray went
    return TraceShadowRay(hit.Position, hit.GeoN, hit.Distance * 0.0005, dir, maxDistance);
}

// What ShadeSceneHit takes into account
static const uint HIT_SHADOWS = 1u;         // One shadow ray per light from the hit; without it lights pass through everything
static const uint HIT_SPECULAR = 2u;        // Highlights and the hit's own reflection of its surroundings, as a mirror shows
                                            // them. Without it the hit is shaded as a matte surface, which is all bounce light needs.
// Indirect light arriving at the hit (pick one, or none for direct light only):
static const uint HIT_AMBIENT_SKY = 4u;     // The sky and the flat ambient color, as if nothing were in the way
static const uint HIT_AMBIENT_GIVEN = 8u;   // The caller's `ambient` (average radiance arriving), e.g. the GI at the reflecting pixel
static const uint HIT_AMBIENT_TRACED = 16u; // The sky, only where one more ray finds it: no sky light leaks into closed rooms

// Light leaving a ray hit back along the ray: the scene's lights, the surface's own glow and, depending on flags
// (HIT_* above), indirect light. This is what a reflection shows of the object, or what a GI ray brings back from it.
// coneWidth: width of the ray's cone at the hit, in world units (the pixel's size carried along the ray, plus the
// spread a rough reflector adds). It decides how blurry the hit's textures are read.
// The hit does not reflect other objects.
float3 ShadeSceneHit(SceneHit hit, float3 rayDirection, float coneWidth, float2 noise, uint flags, float3 ambient)
{
    bool traceShadows = (flags & HIT_SHADOWS) != 0u;
    bool specular = (flags & HIT_SPECULAR) != 0u;

    SceneGeometryRecord material = hit.Material;

    // Seen at a slant, the cone covers a longer stretch of the surface
    float footprint = coneWidth / max(abs(dot(hit.GeoN, rayDirection)), 0.2);

    float3 albedo = material.BaseColor.rgb;
    if (material.BaseColorTexture >= 0)
    {
        albedo *= SampleSceneTexture(material.BaseColorTexture, hit.UV, hit.UVPerWorldArea, footprint).rgb;
    }

    float metallic = material.Metallic;
    float roughness = material.Roughness;
    if (material.MetallicRoughnessTexture >= 0)
    {
        float4 mrSample = SampleSceneTexture(material.MetallicRoughnessTexture, hit.UV, hit.UVPerWorldArea, footprint);
        if ((material.Flags & SCENE_GEOMETRY_ROUGHNESS_FIXED) == 0u) roughness *= mrSample.g;
        if ((material.Flags & SCENE_GEOMETRY_METALLIC_FIXED) == 0u) metallic *= mrSample.b;
    }
    roughness = max(roughness, 0.05);

    float3 emissive = material.Emissive;
    if (material.EmissiveTexture >= 0)
    {
        emissive *= SampleSceneTexture(material.EmissiveTexture, hit.UV, hit.UVPerWorldArea, footprint).rgb;
    }

    float3 N = hit.N;
    float3 V = -rayDirection;
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    float3 Lo = float3(0.0, 0.0, 0.0);

    // 1. Directional lights
    for (int i = 0; i < g_NumDirLights; ++i)
    {
        float3 L = normalize(-g_DirLights[i].Direction.xyz);
        // Also against the triangle itself: a smoothed normal can face a light the surface is turned away from
        if (dot(N, L) <= 0.0 || dot(hit.GeoN, L) <= 0.0)
            continue;

        float shadowFactor = traceShadows ? TraceHitShadow(hit, L, 1000.0, tan(SUN_ANGULAR_RADIUS), ShiftNoise(noise, i)) : 1.0;
        float3 radiance = g_DirLights[i].Color.rgb * g_DirLights[i].Color.a;

        Lo += specular
            ? ComputeLightRadiance(N, V, L, radiance, albedo, metallic, roughness, F0, shadowFactor)
            : albedo / PI * radiance * dot(N, L) * shadowFactor;
    }

    // 2. Point lights
    for (int j = 0; j < g_NumPointLights; ++j)
    {
        float3 lightVec = g_PointLights[j].Position.xyz - hit.Position;
        float dist = length(lightVec);
        if (dist >= g_PointLights[j].Range || dist <= 0.0001)
            continue;

        float3 L = lightVec / dist;
        if (dot(N, L) <= 0.0 || dot(hit.GeoN, L) <= 0.0)
            continue;

        float shadowFactor = traceShadows ? TraceHitShadow(hit, L, dist - 0.01, POINT_LIGHT_RADIUS / dist, ShiftNoise(noise, 4 + j)) : 1.0;

        float attenuation = max(1.0 - (dist / g_PointLights[j].Range), 0.0);
        attenuation *= attenuation;
        float3 radiance = g_PointLights[j].Color.rgb * g_PointLights[j].Color.a * attenuation;

        Lo += specular
            ? ComputeLightRadiance(N, V, L, radiance, albedo, metallic, roughness, F0, shadowFactor)
            : albedo / PI * radiance * dot(N, L) * shadowFactor;
    }

    // 3. Indirect light arriving at the hit, as the average radiance over the hemisphere above it
    float3 irradiance = float3(0.0, 0.0, 0.0);
    bool skyReflection = false;
    if ((flags & HIT_AMBIENT_SKY) != 0u)
    {
        irradiance = g_AmbientColor.rgb + SampleEnvIrradiance(N) * g_SkyLightIntensity;
        skyReflection = true;
    }
    else if ((flags & HIT_AMBIENT_GIVEN) != 0u)
    {
        irradiance = ambient;
    }
    else if ((flags & HIT_AMBIENT_TRACED) != 0u)
    {
        // One cosine-weighted ray stands in for the whole hemisphere: averaged over frames it adds up to the sky
        // light that really reaches this point
        float3 t, b;
        BuildBasis(N, t, b);
        float2 skyNoise = ShiftNoise(noise, 15); // Apart from the lights' shadow rays, which use shifts 0 to 11
        float2 d = VogelDisk(0, 1, skyNoise.x * 2.0 * PI, skyNoise.y);
        float3 skyDir
 = normalize(t * d.x + b * d.y + N * sqrt(saturate(1.0 - dot(d, d))));
        if (dot(skyDir, hit.GeoN) > 0.0 && TraceShadowRay(hit.Position, hit.GeoN, hit.Distance * 0.0005, skyDir, 1000.0) > 0.5)
        {
            // A blurred mip, so one ray doesn't have to find a small bright sun on its own
            irradiance = SampleEnvSpecular(skyDir, 0.5) * g_SkyLightIntensity;
        }
    }

    float3 indirect;
    if (specular)
    {
        // Diffuse light, plus the hit's own reflection: of the sky when that is what lights it, else of the light around it
        float NdotV = saturate(dot(N, V)) + 0.0001;
        float2 envBRDF = EnvBRDFApprox(NdotV, roughness);
        float3 specularColor = F0 * envBRDF.x + envBRDF.y;

        float3 reflected = skyReflection ? SampleEnvSpecular(reflect(rayDirection, N), roughness) * g_SkyLightIntensity : irradiance;
        indirect = irradiance * albedo * (1.0 - specularColor) * (1.0 - metallic) + reflected * specularColor * g_ReflectionIntensity;
    }
    else
    {
        // Matte: the base color is close to the share of light a surface sends back, for metals too
        indirect = irradiance * albedo;
    }

    return indirect * g_AmbientMultiplier + Lo + emissive;
}


#endif // RT_SCENE_HLSLI
