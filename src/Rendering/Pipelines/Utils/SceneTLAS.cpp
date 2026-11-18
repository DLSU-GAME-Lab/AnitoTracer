#include "SceneTLAS.hpp"
#include "../../RenderData.hpp"
#include "Models/ModelManager.hpp"
#include <string>
#include <vector>

namespace Diligent {

    namespace {
        // One record per geometry of the model's BLAS, in the BLAS's order, with the instance's overrides applied
        // the same way BasePipeline::RenderModelWithSRB applies them when the model is drawn
        void AppendGeometryRecords(const ModelRenderInstance& instance, std::vector<SceneGeometryRecord>& records)
        {
            const Model& model = *instance.ModelData;
            const SceneGeometryPool& pool = ModelManager::GetInstance().GetSceneGeometryPool();
            const bool inPool = model.PoolVertexBase != SceneGeometryPool::kNotInPool && model.PoolIndexBase != SceneGeometryPool::kNotInPool;

            for (Uint32 submeshIdx : model.BLASSubmeshes) {
                const SubMesh& submesh = model.SubMeshes[submeshIdx];

                SceneGeometryRecord record;
                if (inPool) {
                    record.FirstIndex = model.PoolIndexBase + submesh.IndexOffset;
                    record.BaseVertex = model.PoolVertexBase;
                    record.Flags |= SCENE_GEOMETRY_IN_POOL;
                }

                if (submesh.MaterialIndex < model.PBRMaterials.size()) {
                    const PBRMaterial& mat = model.PBRMaterials[submesh.MaterialIndex];

                    record.BaseColor = glm::vec4(mat.BaseColorFactor.r, mat.BaseColorFactor.g, mat.BaseColorFactor.b, mat.BaseColorFactor.a);
                    record.Roughness = instance.OverrideRoughness ? instance.Roughness : mat.RoughnessFactor;
                    record.Metallic = instance.OverrideMetallic ? instance.Metallic : mat.MetallicFactor;
                    if (instance.OverrideRoughness) record.Flags |= SCENE_GEOMETRY_ROUGHNESS_FIXED;
                    if (instance.OverrideMetallic) record.Flags |= SCENE_GEOMETRY_METALLIC_FIXED;

                    const glm::vec3 materialEmissive = glm::vec3(mat.EmissiveFactor.r, mat.EmissiveFactor.g, mat.EmissiveFactor.b) * mat.EmissiveStrength;
                    record.Emissive = instance.OverrideEmissive ? instance.Emissive : materialEmissive;

                    record.BaseColorTexture = pool.GetTextureSlot(mat.BaseColor);
                    record.MetallicRoughnessTexture = pool.GetTextureSlot(mat.MetallicRoughness);
                    record.EmissiveTexture = pool.GetTextureSlot(mat.Emissive);
                }

                records.push_back(record);
            }
        }
    }

    void SceneTLAS::Initialize(IRenderDevice* pDevice, Uint32 maxInstances, bool geometryRecords)
    {
        m_pDevice = pDevice;

        m_WriteGeometryRecords = geometryRecords && ModelManager::GetInstance().GetSceneGeometryPool().IsEnabled();
        if (m_WriteGeometryRecords && !m_GeometryRecords.IsInitialized()) {
            m_GeometryRecords.Initialize(pDevice, "Scene Geometry Records", sizeof(SceneGeometryRecord), 256);
        }

        TopLevelASDesc TLASDesc;
        TLASDesc.Name = "Scene TLAS";
        TLASDesc.MaxInstanceCount = maxInstances;
        TLASDesc.Flags = RAYTRACING_BUILD_AS_PREFER_FAST_TRACE | RAYTRACING_BUILD_AS_ALLOW_UPDATE;

        m_pTLAS.Release();
        pDevice->CreateTLAS(TLASDesc, &m_pTLAS);
        if (!m_pTLAS) return;

        BufferDesc ScratchBuffDesc;
        ScratchBuffDesc.Name = "TLAS Build Scratch Buffer";
        ScratchBuffDesc.Size = m_pTLAS->GetScratchBufferSizes().Build;
        ScratchBuffDesc.Usage = USAGE_DEFAULT;
        ScratchBuffDesc.BindFlags = BIND_RAY_TRACING;

        m_pScratchBuffer.Release();
        pDevice->CreateBuffer(ScratchBuffDesc, nullptr, &m_pScratchBuffer);
    }

    void SceneTLAS::EnsureInstanceBufferSize(Uint64 requiredSize)
    {
        if (m_pInstanceBuffer && m_pInstanceBuffer->GetDesc().Size >= requiredSize) return;

        BufferDesc InstBuffDesc;
        InstBuffDesc.Name = "TLAS Instance Buffer";
        InstBuffDesc.Size = requiredSize * 2; // Headroom so a growing scene doesn't reallocate every frame
        InstBuffDesc.Usage = USAGE_DEFAULT;
        InstBuffDesc.BindFlags = BIND_RAY_TRACING;
        InstBuffDesc.CPUAccessFlags = CPU_ACCESS_NONE;

        m_pInstanceBuffer.Release();
        m_pDevice->CreateBuffer(InstBuffDesc, nullptr, &m_pInstanceBuffer);
    }

    void SceneTLAS::Build(IDeviceContext* pContext, const RenderData& renderData)
    {
        if (!m_pTLAS) return;

        std::vector<TLASBuildInstanceData> Instances;
        Instances.reserve(renderData.Models.size());

        // TLASBuildInstanceData only points at its name, so the strings must outlive BuildTLAS
        std::vector<std::string> InstanceNames;
        InstanceNames.reserve(renderData.Models.size());

        std::vector<SceneGeometryRecord> GeometryRecords;
        if (m_WriteGeometryRecords) GeometryRecords.reserve(renderData.Models.size());

        for (size_t i = 0; i < renderData.Models.size(); ++i) {
            const auto& modelInstance = renderData.Models[i];

            // Catch models loaded during deferred rendering and build their BLAS now
            if (!modelInstance.ModelData->pBLAS) {
                ModelManager::GetInstance().BuildBLAS(modelInstance.ModelData);
            }

            if (!modelInstance.ModelData->pBLAS || modelInstance.OpaqueSubmeshIndices.empty())
                continue;

            TLASBuildInstanceData tlasInst{};

            InstanceNames.push_back("ModelInstance_" + std::to_string(i));
            tlasInst.InstanceName = InstanceNames.back().c_str();

            tlasInst.pBLAS = modelInstance.ModelData->pBLAS;
            tlasInst.CustomId = static_cast<Uint32>(i);
            if (m_WriteGeometryRecords) {
                // The shader reads a hit's record at InstanceID + GeometryIndex.
                // (A custom ID has 24 bits: room for 16 million records, far beyond what the TLAS's instance limit allows.)
                tlasInst.CustomId = static_cast<Uint32>(GeometryRecords.size());
                AppendGeometryRecords(modelInstance, GeometryRecords);
            }
            tlasInst.Flags = RAYTRACING_INSTANCE_NONE;
            tlasInst.Mask = 0xFF;

            // 3x4 row-major transform from the column-major world matrix
            const glm::mat4& world = modelInstance.WorldTransform;
            float* pTransformData = reinterpret_cast<float*>(&tlasInst.Transform);
            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 4; ++col) {
                    pTransformData[row * 4 + col] = world[col][row];
                }
            }

            Instances.push_back(tlasInst);
        }

        // An empty build would pass a null pInstances; the previous frame's TLAS stays valid instead
        if (Instances.empty()) return;

        // Rebuilt together with the TLAS, since the instances' custom IDs point into it
        if (m_WriteGeometryRecords) {
            m_GeometryRecords.Assign(pContext, GeometryRecords.data(), static_cast<Uint32>(GeometryRecords.size()));
        }

        EnsureInstanceBufferSize(Instances.size() * sizeof(TLASBuildInstanceData));

        BuildTLASAttribs BuildAttribs;
        BuildAttribs.pTLAS = m_pTLAS;
        BuildAttribs.pInstances = Instances.data();
        BuildAttribs.InstanceCount = static_cast<Uint32>(Instances.size());
        BuildAttribs.pInstanceBuffer = m_pInstanceBuffer;
        BuildAttribs.pScratchBuffer = m_pScratchBuffer;

        BuildAttribs.TLASTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        BuildAttribs.BLASTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        BuildAttribs.InstanceBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        BuildAttribs.ScratchBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

        pContext->BuildTLAS(BuildAttribs);
    }

}
