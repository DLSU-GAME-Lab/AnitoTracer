#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "Common/interface/BasicMath.hpp"
#include "../../Common/CollisionMeshData.hpp"
#include "TextureLoader/interface/TextureUtilities.h"
#include "Graphics/GraphicsEngine/interface/TextureView.h"
#include "Graphics/GraphicsEngine/interface/BottomLevelAS.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h> // The alpha mode and cutoff glTF materials carry
#include <iostream>
#include <filesystem> // Add this at the top of your cpp file

#include <glm/glm.hpp>

#include "Types/IModel.hpp"

using namespace Diligent;

// Standard vertex structure
struct Vertex {
    float3 pos;
    float3 normal;
    float2 uv;
    float3 tangent;
    float3 bitangent;
};

// Represents a single part of a model with a specific material
struct SubMesh {
    Uint32 IndexCount = 0;
    Uint32 IndexOffset = 0;
    Uint32 BaseVertex = 0;
    Uint32 MaterialIndex = 0;
};

// How a material's alpha channel is used. Mirrors glTF's alphaMode.
enum class MaterialAlphaMode : uint8_t {
    Opaque, // Alpha is ignored
    Mask,   // Pixels with less alpha than the cutoff are cut out, the rest are solid (fences, leaves)
    Blend,  // Alpha is how much of the surface is there; the rest shows what is behind it (glass)
};

//Holds PBR properties
struct PBRMaterial {
    RefCntAutoPtr<ITextureView> BaseColor;
    RefCntAutoPtr<ITextureView> MetallicRoughness;
    RefCntAutoPtr<ITextureView> Normal;
    RefCntAutoPtr<ITextureView> AO;
    RefCntAutoPtr<ITextureView> Emissive;

    // PBR Factors (Defaults)
    float4 BaseColorFactor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float MetallicFactor = 0.0f;
    float RoughnessFactor = 1.0f;

    // Emitted light = EmissiveFactor * EmissiveStrength * emissive map (white when there is no map)
    float3 EmissiveFactor = float3(0.0f, 0.0f, 0.0f);
    float EmissiveStrength = 1.0f;

    MaterialAlphaMode AlphaMode = MaterialAlphaMode::Opaque;
    float AlphaCutoff = 0.5f;       // Mask: pixels with less alpha than this are cut out
    float IOR = 1.5f;               // Index of refraction, for blended materials drawn as glass

    // Layers over the base material (glTF KHR_materials_clearcoat, _sheen and _transmission). 0 / black: the layer isn't there.
    float Clearcoat = 0.0f;                         // Strength of the clear lacquer on top
    float ClearcoatRoughness = 0.0f;                // How blurred its highlight is
    float3 SheenColor = float3(0.0f, 0.0f, 0.0f);   // Color of the cloth fibres' rim light
    float SheenRoughness = 0.5f;                    // How wide that rim is
    float Transmission = 0.0f;                      // Share of the light that goes through the surface, tinted by the base color

    // Blend materials are drawn after the opaque geometry (Opaque and Mask ones go with it)
    bool IsTransparent = false;
};

// Holds the loaded GPU resources for a complete model
struct Model : public IModel {
    RefCntAutoPtr<IBuffer> pVertexBuffer;
    RefCntAutoPtr<IBuffer> pIndexBuffer;

    // Second vertex stream of the Anime renderer: each vertex's smooth normal and curvature (AnimeVertexData), same order
    // as pVertexBuffer
    RefCntAutoPtr<IBuffer> pAnimeVertexBuffer;

    RefCntAutoPtr<IBottomLevelAS> pBLAS;

    // The submesh behind each geometry of pBLAS, in order. A ray hit reports the geometry it landed on,
    // which leads to the submesh and from there to its material. (Submeshes without triangles have no geometry.)
    std::vector<Uint32> BLASSubmeshes;

    // Where this model's vertices and indices start in SceneGeometryPool, for shading ray hits.
    // ~0u (SceneGeometryPool::kNotInPool) when the model isn't in the pool.
    Uint32 PoolVertexBase = ~0u;
    Uint32 PoolIndexBase = ~0u;

    std::vector<SubMesh> SubMeshes;
    std::vector<RefCntAutoPtr<ITextureView>> Materials; // Diffuse SRVs mapped to SubMeshes

    //PBR Mats
    std::vector<PBRMaterial> PBRMaterials;

    //Solid colors and possibly fallbacks
    std::vector<float4> MaterialColors;

    //EZ flag
    bool HasPBRProperties = false;

    //If any submesh has transparency
    bool HasTransparency = false;

    // If any material is a cutout or glass: rays have to judge its triangles (rt_scene.hlsli) instead of treating them as solid
    bool HasAlphaGeometry = false;

    // If any material lets light through (PBRMaterial::Transmission). Such a material counts as glass for rays and is drawn
    // with the glass, but only while the project's Materials settings have transmission on: see ModelRenderInstance::ResolveLayers.
    bool HasTransmission = false;

    //For local raytrace obj picking
    glm::vec3 AABBMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 AABBMax = glm::vec3(std::numeric_limits<float>::lowest());

    // For physics mesh colliders
    std::shared_ptr<CollisionMeshData> CollisionData;
};