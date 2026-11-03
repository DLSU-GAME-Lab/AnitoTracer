#include "SceneTLAS.hpp"
#include "../../RenderData.hpp"
#include "Models/ModelManager.hpp"
#include <string>
#include <vector>

namespace Diligent {

    void SceneTLAS::Initialize(IRenderDevice* pDevice, Uint32 maxInstances)
    {
        m_pDevice = pDevice;

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
