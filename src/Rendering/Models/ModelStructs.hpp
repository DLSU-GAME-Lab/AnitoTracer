#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "Common/interface/BasicMath.hpp"
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
};

// Holds the loaded GPU resources for a complete model
struct Model : public IModel {
    RefCntAutoPtr<IBuffer> pVertexBuffer;
    RefCntAutoPtr<IBuffer> pIndexBuffer;

    RefCntAutoPtr<IBottomLevelAS> pBLAS;

    std::vector<SubMesh> SubMeshes;
    std::vector<RefCntAutoPtr<ITextureView>> Materials; // Diffuse SRVs mapped to SubMeshes

    //PBR Mats
    std::vector<PBRMaterial> PBRMaterials;

    //Solid colors and possibly fallbacks
    std::vector<float4> MaterialColors;

    //EZ flag
    bool HasPBRProperties = false;

    //For local raytrace obj picking
    glm::vec3 AABBMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 AABBMax = glm::vec3(std::numeric_limits<float>::lowest());
};