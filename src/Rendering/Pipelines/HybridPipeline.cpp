#include "HybridPipeline.hpp"
#include "HybridPipeline.hpp"
#include "../RenderData.hpp"
#include "../../UserSettings.hpp"

void Diligent::HybridPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain)
{
    pSwapChain = _pSwapChain;
    m_pDevice = pDevice;

    Uint8 sampleCount = UserSettings::GetInstance().GetEnableMSAA() ? 4 : 1;

    if (!m_pTLAS) {
        InitializeTLAS(pDevice);
    }

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "PBR Rendering PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    SetupDefaultGraphicsPipeline(GraphicsPipeline);
    GraphicsPipeline.SmplDesc.Count = sampleCount;

    std::vector<LayoutElement> std_layout = VertexLayouts::GetStandardLayout();
    GraphicsPipeline.InputLayout.LayoutElements = std_layout.data();
    GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(std_layout.size());

    auto pVS = ShaderManager::GetInstance().GetShader("hybrid_vs.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("hybrid_ps.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    std::vector<ShaderResourceVariableDesc> Variables = GetStandardShaderVariables();
    Variables.push_back({ SHADER_TYPE_PIXEL, "g_TLAS", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC }); // Hybrid-only requirement

    // Global Buffers for Ray Tracing
    Variables.push_back({ SHADER_TYPE_PIXEL, "g_GlobalVertices", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC });
    Variables.push_back({ SHADER_TYPE_PIXEL, "g_GlobalIndices", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC });
    Variables.push_back({ SHADER_TYPE_PIXEL, "g_InstanceData", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC });
    Variables.push_back({ SHADER_TYPE_PIXEL, "g_MaterialData", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC });

    // The Bindless Array (Mutable allows SetArray calls)
    Variables.push_back({ SHADER_TYPE_PIXEL, "g_BindlessTextures", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE });

    PSODesc.ResourceLayout.Variables = Variables.data();
    PSODesc.ResourceLayout.NumVariables = static_cast<Uint32>(Variables.size());

    std::vector<ImmutableSamplerDesc> ImtblSamplers = GetStandardImmutableSamplers();

	// Immutable sampler for the bindless texture array
    ImtblSamplers.push_back({ SHADER_TYPE_PIXEL, "g_LinearSampler", GetLinearWrapSamplerDesc() });

    PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers.data();
    PSODesc.ResourceLayout.NumImmutableSamplers = static_cast<Uint32>(ImtblSamplers.size());

    m_pPSO.Release();
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    if (!m_pCameraCB) CreateCameraConstantBuffer(pDevice);
    if (!m_pModelCB) CreateModelConstantBuffer(pDevice);

    // Utilize refactored buffer helper
    CreateCommonConstantBuffers(pDevice);

    m_pSRB.Release();
    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants")) pVar->Set(m_pCameraCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "ModelConstants"))  pVar->Set(m_pModelCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "LightConstants"))   pVar->Set(m_pLightCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "PBRMaterialConstants")) pVar->Set(m_pMaterialCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "ShadowSettings"))   pVar->Set(m_pShadowCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_TLAS"))           pVar->Set(m_pTLAS);

    auto CreateDummyBuffer = [&](RefCntAutoPtr<IBuffer>& pBuffer, const char* name, Uint32 elementStride) {
        BufferDesc desc;
        desc.Name = name;
        desc.Size = elementStride; // Just 1 element size to keep Vulkan happy!
        desc.Usage = USAGE_DEFAULT;
        desc.BindFlags = BIND_SHADER_RESOURCE;
        desc.Mode = BUFFER_MODE_STRUCTURED;
        desc.ElementByteStride = elementStride;
        m_pDevice->CreateBuffer(desc, nullptr, &pBuffer);
        };

    // Create dummy buffers if the scene starts empty
    if (!m_pGlobalVertexBuffer) CreateDummyBuffer(m_pGlobalVertexBuffer, "Dummy Global Vertex Buffer", sizeof(Vertex));
    if (!m_pGlobalIndexBuffer)  CreateDummyBuffer(m_pGlobalIndexBuffer, "Dummy Global Index Buffer", sizeof(Uint32));
    if (!m_pInstanceBuffer)     CreateDummyBuffer(m_pInstanceBuffer, "Dummy Global Instance Buffer", sizeof(BindlessInstanceData));
    if (!m_pMaterialBuffer)     CreateDummyBuffer(m_pMaterialBuffer, "Dummy Global Material Buffer", sizeof(BindlessMaterial));
    // ---------------------------------

    // Now it is perfectly safe to get their views! No more null pointers! ☆
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GlobalVertices")) {
        pVar->Set(m_pGlobalVertexBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
    }
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GlobalIndices")) {
        pVar->Set(m_pGlobalIndexBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
    }
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_InstanceData")) {
        pVar->Set(m_pInstanceBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
    }
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_MaterialData")) {
        pVar->Set(m_pMaterialBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
    }
}

void Diligent::HybridPipeline::StartFrameRender(IDeviceContext* pContext, RenderData renderData)
{
    BasePipeline::StartFrameRender(pContext, renderData);
    BuildSceneTLAS(pContext, renderData);
    UpdateBindlessResources(pContext, renderData);
}

void Diligent::HybridPipeline::InitializeTLAS(IRenderDevice* pDevice, Uint32 maxInstances)
{
    TopLevelASDesc TLASDesc;
    TLASDesc.Name = "Scene TLAS";
    TLASDesc.MaxInstanceCount = maxInstances;
    TLASDesc.Flags = RAYTRACING_BUILD_AS_PREFER_FAST_TRACE | RAYTRACING_BUILD_AS_ALLOW_UPDATE;

    pDevice->CreateTLAS(TLASDesc, &m_pTLAS);

    ScratchBufferSizes ScratchSizes = m_pTLAS->GetScratchBufferSizes();

    BufferDesc ScratchBuffDesc;
    ScratchBuffDesc.Name = "TLAS Build Scratch Buffer";
    ScratchBuffDesc.Size = ScratchSizes.Build;
    ScratchBuffDesc.Usage = USAGE_DEFAULT;
    ScratchBuffDesc.BindFlags = BIND_RAY_TRACING;

    pDevice->CreateBuffer(ScratchBuffDesc, nullptr, &m_pTLASScratchBuffer);
}

void Diligent::HybridPipeline::BuildSceneTLAS(IDeviceContext* pContext, const RenderData& renderData)
{
    if (renderData.Models.empty() || !m_pTLAS) return;

    std::vector<TLASBuildInstanceData> Instances;
    Instances.reserve(renderData.Models.size());

    for (size_t i = 0; i < renderData.Models.size(); ++i) {
        const auto& modelInstance = renderData.Models[i];
        if (!modelInstance.ModelData->pBLAS) continue;

        if (!modelInstance.ModelData->pBLAS || modelInstance.OpaqueSubmeshIndices.empty())
            continue;

        TLASBuildInstanceData tlasInst{};
        tlasInst.InstanceName = "ModelInstance " + i;
        tlasInst.pBLAS = modelInstance.ModelData->pBLAS;
        tlasInst.CustomId = static_cast<Uint32>(i);
        tlasInst.Flags = RAYTRACING_INSTANCE_NONE;
        tlasInst.Mask = 0xFF;

        const glm::mat4& world = modelInstance.WorldTransform;
        float* pTransformData = reinterpret_cast<float*>(&tlasInst.Transform);

        pTransformData[0] = world[0][0]; pTransformData[1] = world[1][0];
        pTransformData[2] = world[2][0]; pTransformData[3] = world[3][0];

        pTransformData[4] = world[0][1]; pTransformData[5] = world[1][1];
        pTransformData[6] = world[2][1]; pTransformData[7] = world[3][1];

        pTransformData[8] = world[0][2]; pTransformData[9] = world[1][2];
        pTransformData[10] = world[2][2]; pTransformData[11] = world[3][2];

        Instances.push_back(tlasInst);
    }

    if (Instances.empty()) return;

    Uint32 requiredInstanceBufferSize = static_cast<Uint32>(Instances.size() * sizeof(TLASBuildInstanceData));

    if (!m_pTLASInstanceBuffer || m_pTLASInstanceBuffer->GetDesc().Size < requiredInstanceBufferSize) {
        BufferDesc InstBuffDesc;
        InstBuffDesc.Name = "TLAS Instance Buffer";
        InstBuffDesc.Size = requiredInstanceBufferSize * 2;

        InstBuffDesc.Usage = USAGE_DEFAULT;
        InstBuffDesc.BindFlags = BIND_RAY_TRACING;
        InstBuffDesc.CPUAccessFlags = CPU_ACCESS_NONE;

        m_pTLASInstanceBuffer.Release();
        m_pDevice->CreateBuffer(InstBuffDesc, nullptr, &m_pTLASInstanceBuffer);
    }

    BuildTLASAttribs BuildAttribs;
    BuildAttribs.pTLAS = m_pTLAS;
    BuildAttribs.pInstances = Instances.data();
    BuildAttribs.InstanceCount = static_cast<Uint32>(Instances.size());
    BuildAttribs.pInstanceBuffer = m_pTLASInstanceBuffer;
    BuildAttribs.pScratchBuffer = m_pTLASScratchBuffer;

    BuildAttribs.TLASTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    BuildAttribs.BLASTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    BuildAttribs.InstanceBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    BuildAttribs.ScratchBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    pContext->BuildTLAS(BuildAttribs);
}

void Diligent::HybridPipeline::UpdateBindlessResources(IDeviceContext* pContext, const RenderData& renderData)
{
    if (renderData.Models.empty()) return;

    // Gather global vertices, indices, instances, and unique materials across all render items
    std::vector<Vertex> allVertices;
    std::vector<Uint32> allIndices;
    std::vector<BindlessInstanceData> allInstances;
    std::vector<BindlessMaterial> allMaterials;

    std::unordered_map<const PBRMaterial*, Uint32> materialToIndexMap;

    for (size_t i = 0; i < renderData.Models.size(); ++i) {
        const auto& modelInstance = renderData.Models[i];
        Model* model = modelInstance.ModelData;
        if (!model) continue;

        // For simplicity, we aggregate submeshes based on model layout chunks.
        Uint32 vertexOffsetVal = static_cast<Uint32>(allVertices.size());
        Uint32 indexOffsetVal = static_cast<Uint32>(allIndices.size());

        allVertices.insert(allVertices.end(), model->CPUVertices.begin(), model->CPUVertices.end());
        allIndices.insert(allIndices.end(), model->CPUIndices.begin(), model->CPUIndices.end());

        // Replace the submesh loop with a single instance push for the whole model!
        Uint32 matIdx = 0;

        // Grab the first material of the model to map 1:1 with the TLAS CustomId!
        if (!modelInstance.OpaqueSubmeshIndices.empty()) {
            const auto& sub = model->SubMeshes[modelInstance.OpaqueSubmeshIndices[0]];
            const PBRMaterial& pbrMat = model->PBRMaterials[sub.MaterialIndex];

            auto matIt = materialToIndexMap.find(&pbrMat);
            if (matIt != materialToIndexMap.end()) {
                matIdx = matIt->second;
            }
            else {
                matIdx = static_cast<Uint32>(allMaterials.size());
                materialToIndexMap[&pbrMat] = matIdx;

                BindlessMaterial bindlessMat{};
                auto cachedTextures = ModelManager::GetInstance().GetCachedTextures();
                int texIndex = -1;
                if (pbrMat.BaseColor) {
                    for (size_t t = 0; t < cachedTextures.size(); ++t) {
                        if (cachedTextures[t] == pbrMat.BaseColor) {
                            texIndex = static_cast<int>(t);
                            break;
                        }
                    }
                }
                bindlessMat.BaseColorTexIdx = texIndex;
                allMaterials.push_back(bindlessMat);
            }
        }

        BindlessInstanceData instData{};
        instData.VertexOffset = vertexOffsetVal; // Start of the ENTIRE model's vertices
        instData.IndexOffset = indexOffsetVal;   // Start of the ENTIRE model's indices
        instData.MaterialIndex = matIdx;
        allInstances.push_back(instData);
    }

    // 2. Reuse ModelManager's m_TextureCache via GetCachedTextures()
    auto cachedTextures = ModelManager::GetInstance().GetCachedTextures();
    ITextureView* pDefaultTex = ModelManager::GetInstance().GetDefaultTexture();

    // Create an array of exactly 1024 elements and fill it with the default white texture! ☆
    std::vector<IDeviceObject*> paddedTextures(1024, pDefaultTex);

    // Copy all of our actually loaded textures into the beginning of the array
    for (size_t t = 0; t < cachedTextures.size() && t < 1024; ++t) {
        paddedTextures[t] = cachedTextures[t];
    }

    // Now bind the fully padded array so Vulkan's validation layers stay perfectly happy!
    if (auto* pTexVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_BindlessTextures")) {
        pTexVar->SetArray(paddedTextures.data(), 0, 1024);
    }

    if (!allVertices.empty()) {
        if (!m_pGlobalVertexBuffer || m_pGlobalVertexBuffer->GetDesc().Size < allVertices.size() * sizeof(Vertex)) {
            BufferDesc desc;
            desc.Name = "Global Vertex Buffer";
            desc.Size = allVertices.size() * sizeof(Vertex) * 2; // Extra headroom
            desc.Usage = USAGE_DEFAULT;
            desc.BindFlags = BIND_SHADER_RESOURCE;
            desc.Mode = BUFFER_MODE_STRUCTURED;
            desc.ElementByteStride = sizeof(Vertex);

            m_pGlobalVertexBuffer.Release();
            m_pDevice->CreateBuffer(desc, nullptr, &m_pGlobalVertexBuffer);
        }

        // Upload the stitched array to the GPU
        pContext->UpdateBuffer(m_pGlobalVertexBuffer, 0, allVertices.size() * sizeof(Vertex), allVertices.data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // Bind the fresh buffer view to the Shader Resource Binding
        if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GlobalVertices")) {
            pVar->Set(m_pGlobalVertexBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
        }
    }

    // Update the Global Index Buffer
    if (!allIndices.empty()) {
        if (!m_pGlobalIndexBuffer || m_pGlobalIndexBuffer->GetDesc().Size < allIndices.size() * sizeof(Uint32)) {
            BufferDesc desc;
            desc.Name = "Global Index Buffer";
            desc.Size = allIndices.size() * sizeof(Uint32) * 2; // Extra headroom
            desc.Usage = USAGE_DEFAULT;
            desc.BindFlags = BIND_SHADER_RESOURCE;
            desc.Mode = BUFFER_MODE_STRUCTURED;
            desc.ElementByteStride = sizeof(Uint32);

            m_pGlobalIndexBuffer.Release();
            m_pDevice->CreateBuffer(desc, nullptr, &m_pGlobalIndexBuffer);
        }

        // Upload the stitched array to the GPU
        pContext->UpdateBuffer(m_pGlobalIndexBuffer, 0, allIndices.size() * sizeof(Uint32), allIndices.data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // Bind the fresh buffer view to the Shader Resource Binding
        if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GlobalIndices")) {
            pVar->Set(m_pGlobalIndexBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
        }
    }

    // 3. Re-create / Update Dynamic or Default Buffers for Global Data if sizes change
    // (Implementation detail: create buffers using USAGE_DYNAMIC or USAGE_DEFAULT with UpdateBuffer calls)
    // Example for binding material and instance buffers:
    if (!allMaterials.empty()) {
        if (!m_pMaterialBuffer || m_pMaterialBuffer->GetDesc().Size < allMaterials.size() * sizeof(BindlessMaterial)) {
            BufferDesc desc;
            desc.Name = "Global Bindless Materials Buffer";
            desc.Size = allMaterials.size() * sizeof(BindlessMaterial) * 2; // Extra headroom
            desc.Usage = USAGE_DEFAULT;
            desc.BindFlags = BIND_SHADER_RESOURCE;

            // 💖 Added missing structure information!
            desc.Mode = BUFFER_MODE_STRUCTURED;
            desc.ElementByteStride = sizeof(BindlessMaterial);

            m_pMaterialBuffer.Release();
            m_pDevice->CreateBuffer(desc, nullptr, &m_pMaterialBuffer);
        }
        pContext->UpdateBuffer(m_pMaterialBuffer, 0, allMaterials.size() * sizeof(BindlessMaterial), allMaterials.data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_MaterialData")) {
            pVar->Set(m_pMaterialBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
        }

        
    }

    if (!allInstances.empty()) {
        if (!m_pInstanceBuffer || m_pInstanceBuffer->GetDesc().Size < allInstances.size() * sizeof(BindlessInstanceData)) {
            BufferDesc desc;
            desc.Name = "Global Bindless Instances Buffer";
            desc.Size = allInstances.size() * sizeof(BindlessInstanceData) * 2;
            desc.Usage = USAGE_DEFAULT;
            desc.BindFlags = BIND_SHADER_RESOURCE;

            // 💖 Added missing structure information!
            desc.Mode = BUFFER_MODE_STRUCTURED;
            desc.ElementByteStride = sizeof(BindlessInstanceData);

            m_pInstanceBuffer.Release();
            m_pDevice->CreateBuffer(desc, nullptr, &m_pInstanceBuffer);
        }
        pContext->UpdateBuffer(m_pInstanceBuffer, 0, allInstances.size() * sizeof(BindlessInstanceData), allInstances.data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_InstanceData")) {
            pVar->Set(m_pInstanceBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
        }

        
    }
}
