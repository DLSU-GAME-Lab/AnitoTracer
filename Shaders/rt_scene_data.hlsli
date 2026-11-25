#ifndef RT_SCENE_DATA_HLSLI
#define RT_SCENE_DATA_HLSLI

// The tables rays read to find out what they hit, and the parts of that every ray needs, not only the ones that shade
// the hit: whether a cutout's pixel is there, what colour glass lets through (see rt_scene.hlsli for the rest).
//
// Only compiled when the pipeline defines RT_SCENE_SHADING (HybridPipeline::GetRayTracingMacros).
// Requires light_struct.hlsli, and the TRANSPARENCY_* constants of rt_shadows.hlsli.

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
    float AlphaCutoff;              // Cutout materials: pixels with less alpha than this are not there

    float Transmission;             // Glass: the share of the light that goes through. 1 for a blended material.
    float EmissiveLight;            // How many times brighter the light the glow casts on its surroundings is than the glow looks (HIT_EMISSIVE_LIGHT)
    float Padding0;                 // Single floats rather than a vector, which a structured buffer would align
    float Padding1;
};

static const uint SCENE_GEOMETRY_IN_POOL = 1u;          // The triangle's vertices can be read; otherwise the hit can't be shaded
static const uint SCENE_GEOMETRY_ROUGHNESS_FIXED = 2u;  // Roughness is a per-object override: ignore the texture's channel
static const uint SCENE_GEOMETRY_METALLIC_FIXED = 4u;   // Same for metallic
static const uint SCENE_GEOMETRY_ALPHA_MASK = 8u;       // A cutout: rays test the base color's alpha against AlphaCutoff
static const uint SCENE_GEOMETRY_BLEND = 16u;           // Glass: the base color's rgb tints what passes through, its alpha says how much
static const uint SCENE_GEOMETRY_TRANSMISSIVE = 32u;    // With BLEND: an opaque material that lets light through; its alpha means nothing

StructuredBuffer<SceneVertex> g_SceneVertices;
StructuredBuffer<uint> g_SceneIndices;
StructuredBuffer<SceneGeometryRecord> g_SceneGeometry;

Texture2D g_SceneTextures[SCENE_TEXTURE_COUNT];
SamplerState g_SceneTextures_sampler;

// Base color of a triangle at a point inside it (barycentric coordinates as a ray query reports them), read at the top
// mip: an alpha test has no footprint to pick a mip from.
float4 SceneBaseColorAt(SceneGeometryRecord record, uint primitive, float2 bary)
{
    float4 color = record.BaseColor;
    if (record.BaseColorTexture >= 0 && (record.Flags & SCENE_GEOMETRY_IN_POOL) != 0u)
    {
        uint firstIndex = record.FirstIndex + primitive * 3u;
        SceneVertex v0 = g_SceneVertices[g_SceneIndices[firstIndex] + record.BaseVertex];
        SceneVertex v1 = g_SceneVertices[g_SceneIndices[firstIndex + 1u] + record.BaseVertex];
        SceneVertex v2 = g_SceneVertices[g_SceneIndices[firstIndex + 2u] + record.BaseVertex];

        float2 uv = float2(v0.U, v0.V) * (1.0 - bary.x - bary.y) + float2(v1.U, v1.V) * bary.x + float2(v2.U, v2.V) * bary.y;
        color *= g_SceneTextures[NonUniformResourceIndex(record.BaseColorTexture)].SampleLevel(g_SceneTextures_sampler, uv, 0.0);
    }
    return color;
}

// What a pane of glass lets through at that point: its color, as much of it as the glass is opaque, and no more light than
// its transmission lets pass (all of it for plain glass). A material that is opaque but lets light through has an alpha
// that means nothing: the color tints all of what passes.
float3 GlassTransmittanceAt(SceneGeometryRecord record, uint primitive, float2 bary)
{
    float4 color = SceneBaseColorAt(record, primitive, bary);
    float alpha = (record.Flags & SCENE_GEOMETRY_TRANSMISSIVE) != 0u ? 1.0 : saturate(color.a);
    return lerp(float3(1.0, 1.0, 1.0), color.rgb, alpha) * record.Transmission;
}

// Whether a ray stops at a cutout's triangle: it does where the material's alpha reaches its cutoff, and goes
// through the gaps. With cutouts switched off in the project the whole triangle is solid, as it was before.
bool SceneCutoutBlocks(SceneGeometryRecord record, uint primitive, float2 bary)
{
    if ((g_TransparencyFlags & TRANSPARENCY_CUTOUT) == 0u)
    {
        return true;
    }
    return SceneBaseColorAt(record, primitive, bary).a >= record.AlphaCutoff;
}

#endif // RT_SCENE_DATA_HLSLI
