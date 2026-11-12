#pragma once

#include <vector>
#include <string>
#include <unordered_map>
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

    bool IsTransparent = false;
};

// Holds the loaded GPU resources for a complete model
struct Model : public IModel {
    RefCntAutoPtr<IBuffer> pVertexBuffer;
    RefCntAutoPtr<IBuffer> pIndexBuffer;

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

    //For local raytrace obj picking
    glm::vec3 AABBMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 AABBMax = glm::vec3(std::numeric_limits<float>::lowest());

    // For physics mesh colliders
    std::shared_ptr<CollisionMeshData> CollisionData;
};