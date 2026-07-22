#include "BasicLitPipeline.hpp"
#include "../RenderData.hpp"

void Diligent::LitPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain)
{
    pSwapChain = _pSwapChain;
    m_pDevice = pDevice;

    InitializeTLAS(pDevice);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "PBR Rendering PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    SetupDefaultGraphicsPipeline(GraphicsPipeline);

    GraphicsPipeline.SmplDesc.Count = 4;

    std::vector<LayoutElement> std_layout = VertexLayouts::GetStandardLayout();
    GraphicsPipeline.InputLayout.LayoutElements = std_layout.data();
    GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(std_layout.size());

    auto pVS = ShaderManager::GetInstance().GetShader("litTextured_vs.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("litTextured_ps.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    ShaderResourceVariableDesc Variables[] =
    {
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "CameraConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "ModelConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "LightConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "PBRMaterialConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "ShadowSettings", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_TLAS", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

        // Dynamic Textures for Material Submeshes
        {SHADER_TYPE_PIXEL, "g_BaseColorMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_MetallicRoughnessMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_NormalMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_AOMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_EmissiveMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    PSODesc.ResourceLayout.Variables = Variables;
    PSODesc.ResourceLayout.NumVariables = _countof(Variables);

    SamplerDesc SamLinearWrapDesc = GetLinearWrapSamplerDesc();

    ImmutableSamplerDesc ImtblSamplers[] =
    {
        {SHADER_TYPE_PIXEL, "g_BaseColorMap_sampler", SamLinearWrapDesc},
        {SHADER_TYPE_PIXEL, "g_MetallicRoughnessMap_sampler", SamLinearWrapDesc},
        {SHADER_TYPE_PIXEL, "g_NormalMap_sampler", SamLinearWrapDesc},
        {SHADER_TYPE_PIXEL, "g_AOMap_sampler", SamLinearWrapDesc},
        {SHADER_TYPE_PIXEL, "g_EmissiveMap_sampler", SamLinearWrapDesc}
    };
    PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers;
    PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    // Buffers
    CreateCameraConstantBuffer(pDevice);
    CreateModelConstantBuffer(pDevice);

    BufferDesc MatCBDesc;
    MatCBDesc.Name = "PBR Material Constant Buffer";
    MatCBDesc.Size = sizeof(PBRMaterialConstants);
    MatCBDesc.Usage = USAGE_DYNAMIC;
    MatCBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    MatCBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    pDevice->CreateBuffer(MatCBDesc, nullptr, &m_pMaterialCB);

    BufferDesc LightCBDesc;
    LightCBDesc.Name = "Light Constant Buffer";
    LightCBDesc.Size = sizeof(LightConstants);
    LightCBDesc.Usage = USAGE_DYNAMIC;
    LightCBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    LightCBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    pDevice->CreateBuffer(LightCBDesc, nullptr, &m_pLightCB);

    BufferDesc ShadowCBDesc;
    ShadowCBDesc.Name = "Shadow Settings Constant Buffer";
    ShadowCBDesc.Size = sizeof(ShadowSettings);
    ShadowCBDesc.Usage = USAGE_DYNAMIC;
    ShadowCBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    ShadowCBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    pDevice->CreateBuffer(ShadowCBDesc, nullptr, &m_pShadowCB);

    // SRB Initial Setup
    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants")) pVar->Set(m_pCameraCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "ModelConstants"))  pVar->Set(m_pModelCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "LightConstants"))   pVar->Set(m_pLightCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "PBRMaterialConstants")) pVar->Set(m_pMaterialCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "ShadowSettings"))   pVar->Set(m_pShadowCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_TLAS"))           pVar->Set(m_pTLAS);
}

void Diligent::LitPipeline::StartFrameRender(IDeviceContext* pContext, RenderData renderData)
{
    // 1. Call the base pipeline method to bind constants and the PSO
    BasePipeline::StartFrameRender(pContext, renderData);

    // 2. Build the TLAS for the current frame's models
    BuildSceneTLAS(pContext, renderData);
}

void Diligent::LitPipeline::UpdateLights(IDeviceContext* pContext, const LightConstants& lights)
{
    MapHelper<LightConstants> CBData(pContext, m_pLightCB, MAP_WRITE, MAP_FLAG_DISCARD);
    *CBData = lights;
}

void Diligent::LitPipeline::UpdateShadowSettings(IDeviceContext* pContext, const ShadowSettings& settings)
{
    MapHelper<ShadowSettings> CBData(pContext, m_pShadowCB, MAP_WRITE, MAP_FLAG_DISCARD);
    *CBData = settings;
}

void Diligent::LitPipeline::RenderModel(IDeviceContext* pContext, const ModelRenderInstance modelData)
{
    Model* model = modelData.ModelData;

    {
        MapHelper<glm::mat4> CBData(pContext, m_pModelCB, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBData = glm::transpose(modelData.WorldTransform);
    }

    IBuffer* pBuffs[] = { model->pVertexBuffer };
    pContext->SetVertexBuffers(0, 1, pBuffs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(model->pIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    for (const auto& submesh : model->SubMeshes) {
        if (submesh.MaterialIndex < model->PBRMaterials.size()) {
            const PBRMaterial& mat = model->PBRMaterials[submesh.MaterialIndex];

            auto colorFactor = mat.BaseColorFactor;
            PBRMaterialConstants matCBData{};
            matCBData.BaseColorFactor = glm::vec4(colorFactor.r, colorFactor.g, colorFactor.b, colorFactor.a);
            matCBData.MetallicFactor = mat.MetallicFactor;
            matCBData.RoughnessFactor = mat.RoughnessFactor;

            // We know mat.BaseColor is never null (it falls back to a default white texture)
            // We will use this as a safe "dummy" texture to stop Vulkan from crying about empty slots!
            ITextureView* pSafeFallbackTexture = mat.BaseColor;

            // Bind Base Color Map
            if (auto* pBaseColorVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_BaseColorMap")) {
                pBaseColorVar->Set(mat.BaseColor);
            }
            matCBData.UseBaseColorMap = mat.BaseColor ? 1.0f : 0.0f;

            // Bind Metallic-Roughness Map
            if (auto* pMRVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_MetallicRoughnessMap")) {
                pMRVar->Set(mat.MetallicRoughness ? mat.MetallicRoughness : pSafeFallbackTexture);
            }
            matCBData.UseMetallicRoughnessMap = mat.MetallicRoughness ? 1.0f : 0.0f;

            // Bind Normal Map
            if (auto* pNormalVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_NormalMap")) {
                pNormalVar->Set(mat.Normal ? mat.Normal : pSafeFallbackTexture);
            }
            matCBData.UseNormalMap = mat.Normal ? 1.0f : 0.0f;

            // Bind AO Map
            if (auto* pAOVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_AOMap")) {
                pAOVar->Set(mat.AO ? mat.AO : pSafeFallbackTexture);
            }
            matCBData.UseAOMap = mat.AO ? 1.0f : 0.0f;

            // Bind Emissive Map
            if (auto* pEmissiveVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_EmissiveMap")) {
                pEmissiveVar->Set(mat.Emissive ? mat.Emissive : pSafeFallbackTexture);
            }
            matCBData.UseEmissiveMap = mat.Emissive ? 1.0f : 0.0f;

            // Update Material Uniform Buffer
            {
                MapHelper<PBRMaterialConstants> CBData(pContext, m_pMaterialCB, MAP_WRITE, MAP_FLAG_DISCARD);
                *CBData = matCBData;
            }
        }

        pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawIndexedAttribs DrawAttrs;
        DrawAttrs.IndexType = VT_UINT32;
        DrawAttrs.NumIndices = submesh.IndexCount;
        DrawAttrs.FirstIndexLocation = submesh.IndexOffset;
        DrawAttrs.BaseVertex = 0;
        DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;

        pContext->DrawIndexed(DrawAttrs);
    }
}

void Diligent::LitPipeline::InitializeTLAS(IRenderDevice* pDevice, Uint32 maxInstances)
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

void Diligent::LitPipeline::BuildSceneTLAS(IDeviceContext* pContext, const RenderData& renderData)
{
    if (renderData.Models.empty() || !m_pTLAS) return;

    std::vector<TLASBuildInstanceData> Instances;
    Instances.reserve(renderData.Models.size());

    for (size_t i = 0; i < renderData.Models.size(); ++i) {
        const auto& modelInstance = renderData.Models[i];

        // Skip models that don't have a valid BLAS
        if (!modelInstance.ModelData->pBLAS) continue;

        TLASBuildInstanceData tlasInst{};
        tlasInst.InstanceName = "ModelInstance " + i;
        tlasInst.pBLAS = modelInstance.ModelData->pBLAS;
        tlasInst.CustomId = static_cast<Uint32>(i);
        tlasInst.Flags = RAYTRACING_INSTANCE_NONE;
        tlasInst.Mask = 0xFF;

        // Map glm::mat4 (Column-Major) to Diligent's 3x4 Transform (Row-Major)
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

        m_pDevice->CreateBuffer(InstBuffDesc, nullptr, &m_pTLASInstanceBuffer);
    }

    // 2. Setup Build Attributes
    BuildTLASAttribs BuildAttribs;
    BuildAttribs.pTLAS = m_pTLAS;

    // ASSIGN THE CPU INSTANCES POINTER HERE
    BuildAttribs.pInstances = Instances.data();

    BuildAttribs.InstanceCount = static_cast<Uint32>(Instances.size());
    BuildAttribs.pInstanceBuffer = m_pTLASInstanceBuffer;
    BuildAttribs.pScratchBuffer = m_pTLASScratchBuffer;

    BuildAttribs.TLASTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    BuildAttribs.BLASTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    //Add transition states so Vulkan safely waits for the buffer uploads!
    BuildAttribs.InstanceBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    BuildAttribs.ScratchBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    pContext->BuildTLAS(BuildAttribs);
}