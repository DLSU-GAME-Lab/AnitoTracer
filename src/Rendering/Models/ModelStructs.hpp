#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
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
    float4 pos;       // xyz = pos, w = 1.0 (or padding)
    float4 normal;    // xyz = normal, w = 0.0
    float4 uv;        // xy = uv, zw = 0.0
    float4 tangent;   // xyz = tangent, w = 0.0
    float4 bitangent; // xyz = bitangent, w = 0.0
};

// C++ equivalent of the HLSL BindlessMaterial struct
struct BindlessMaterial {
    int BaseColorTexIdx = -1;
    Uint32 Padding1 = 0;
    Uint32 Padding2 = 0;
    Uint32 Padding3 = 0;
};

// C++ equivalent of the HLSL GeometryData struct
struct BindlessGeometryData {
    Uint32 IndexOffset;
    Uint32 MaterialIndex;
    Uint32 Padding1;
    Uint32 Padding2;
};

// C++ equivalent of the HLSL InstanceData struct
struct BindlessInstanceData {
    Uint32 VertexOffset;
    Uint32 GeometryOffset;
    Uint32 Padding1;
    Uint32 Padding2;
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

    bool IsTransparent = false;
};

// Holds the loaded GPU resources for a complete model
struct Model : public IModel {
    RefCntAutoPtr<IBuffer> pVertexBuffer;
    RefCntAutoPtr<IBuffer> pIndexBuffer;

    RefCntAutoPtr<IBottomLevelAS> pBLAS;

    std::vector<Vertex> CPUVertices;
    std::vector<Uint32> CPUIndices;

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
};