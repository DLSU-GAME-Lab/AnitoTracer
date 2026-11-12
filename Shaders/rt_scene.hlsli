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

// SceneGeometryPool::kMaxTextures. The pipeline passes the real value; this default is for compiling the file on its own.
#ifndef SCENE_TEXTURE_COUNT
#define SCENE_TEXTURE_COUNT 512
#endif

// Mirrors SceneGeometryPool::PoolVertex. Object space.
struct SceneVertex
{
    float3 Position;
    float U;
    float3 Normal;
    float V;
};

// Mirrors SceneGeometryRecord in PipelineDefs.hpp
struct SceneGeometryRecord
{
    uint FirstIndex;                // Of the submesh, in g_SceneIndices
    uint BaseVertex;                // Of the model, in g_SceneVertices; added to every index
    int BaseColorTexture;           // Slots in g_SceneTextures, -1: no map
    int MetallicRoughnessTexture;

    float4 BaseColor;

    float3 Emissive;                // Color * strength
    int EmissiveTexture;

    float Roughness;
    float Metallic;
    uint Flags;                     // SCENE_GEOMETRY_* bits
    float Padding;
};

static const uint SCENE_GEOMETRY_IN_POOL = 1u;          // The triangle's vertices can be read; otherwise the hit can't be shaded
static const uint SCENE_GEOMETRY_ROUGHNESS_FIXED = 2u;  // Roughness is a per-object override: ignore the texture's channel
static const uint SCENE_GEOMETRY_METALLIC_FIXED = 4u;   // Same for metallic

StructuredBuffer<SceneVertex> g_SceneVertices;
StructuredBuffer<uint> g_SceneIndices;
StructuredBuffer<SceneGeometryRecord> g_SceneGeometry;

Texture2D g_SceneTextures[SCENE_TEXTURE_COUNT];
SamplerState g_SceneTextures_sampler;

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
    SceneGeometryRecord Material;
};

// Finds the closest surface along a ray. Returns false when the ray leaves the scene without hitting anything.
// Every surface counts as solid: cut-out and transparent materials block the ray like the rest.
bool TraceSceneRay(float3 origin, float3 direction, float maxDistance, out SceneHit hit)
{
    hit = (SceneHit) 0;

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = maxDistance;

    RayQuery < RAY_FLAG_FORCE_OPAQUE > q;
    q.TraceRayInline(g_TLAS, RAY_FLAG_NONE, 0xFF, ray);
    while (q.Proceed())
    {
    }

    if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
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

    // While TAA averages frames, a different point of the light is tested each frame, which adds up to a soft edge.
    // A single still ray can't be soft, so without TAA it aims at the light's centre.
    if (g_NoiseFrame != 0u)
    {
        float3 t, b;
        BuildBasis(L, t, b);
        float2 d = VogelDisk(0, 1, noise.x * 2.0 * PI, noise.y) * spread;
        dir = normalize(L + t * d.x + b * d.y);
    }

    // The hit position is rebuilt from the ray, so it drifts off the surface a little more the farther the ray went
    return TraceShadowRay(hit.Position, hit.GeoN, hit.Distance * 0.0005, dir, maxDistance);
}

// Light leaving a ray hit back along the ray: the scene's lights (one shadow ray each when traceShadows is set),
// the sky and the surface's own glow. This is what a reflection shows of the object.
// coneWidth: width of the ray's cone at the hit, in world units (the pixel's size carried along the ray, plus the
// spread a rough reflector adds). It decides how blurry the hit's textures are read.
// The hit is not checked for what is around it: sky light reaches it unoccluded, and it does not reflect other objects.
float3 ShadeSceneHit(SceneHit hit, float3 rayDirection, float coneWidth, float2 noise, bool traceShadows)
{
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

        Lo += ComputeLightRadiance(N, V, L, radiance, albedo, metallic, roughness, F0, shadowFactor);
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

        Lo += ComputeLightRadiance(N, V, L, radiance, albedo, metallic, roughness, F0, shadowFactor);
    }

    // 3. Ambient: flat ambient color plus the sky, as diffuse light and as the hit's own (sky-only) reflection
    float NdotV = saturate(dot(N, V)) + 0.0001;
    float2 envBRDF = EnvBRDFApprox(NdotV, roughness);
    float3 specularColor = F0 * envBRDF.x + envBRDF.y;

    float3 irradiance = g_AmbientColor.rgb + SampleEnvIrradiance(N) * g_SkyLightIntensity;
    float3 ambient = irradiance * albedo * (1.0 - specularColor) * (1.0 - metallic);
    ambient += SampleEnvSpecular(reflect(rayDirection, N), roughness) * g_SkyLightIntensity * specularColor * g_ReflectionIntensity;

    return ambient * g_AmbientMultiplier + Lo + emissive;
}

#endif // RT_SCENE_HLSLI
