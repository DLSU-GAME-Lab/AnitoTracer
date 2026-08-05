#include "BasePipeline.hpp"
#include "../RenderData.hpp"

void Diligent::BasePipeline::StartFrameRender(IDeviceContext* pContext, RenderData renderData)
{
    glm::mat4 view = renderData.ViewMatrix;
    glm::mat4 proj = renderData.ProjectionMatrix;

    {
        MapHelper<CameraConstants> CBData(pContext, m_pCameraCB, MAP_WRITE, MAP_FLAG_DISCARD);
        CBData->View = glm::transpose(view);
        CBData->Proj = glm::transpose(proj);
    }

    pContext->SetPipelineState(m_pPSO);
}

void Diligent::BasePipeline::UpdateLights(IDeviceContext* pContext, const LightConstants& lights)
{
    MapHelper<LightConstants> CBData(pContext, m_pLightCB, MAP_WRITE, MAP_FLAG_DISCARD);
    *CBData = lights;
}

void Diligent::BasePipeline::UpdateShadowSettings(IDeviceContext* pContext, const ShadowSettings& settings)
{
    MapHelper<ShadowSettings> CBData(pContext, m_pShadowCB, MAP_WRITE, MAP_FLAG_DISCARD);
    *CBData = settings;
}

void Diligent::BasePipeline::RenderModel(IDeviceContext* pContext, const ModelRenderInstance modelData)
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

            ITextureView* pSafeFallbackTexture = mat.BaseColor;

            if (auto* pBaseColorVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_BaseColorMap")) {
                pBaseColorVar->Set(mat.BaseColor);
            }
            matCBData.UseBaseColorMap = mat.BaseColor ? 1.0f : 0.0f;

            if (auto* pMRVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_MetallicRoughnessMap")) {
                pMRVar->Set(mat.MetallicRoughness ? mat.MetallicRoughness : pSafeFallbackTexture);
            }
            matCBData.UseMetallicRoughnessMap = mat.MetallicRoughness ? 1.0f : 0.0f;

            if (auto* pNormalVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_NormalMap")) {
                pNormalVar->Set(mat.Normal ? mat.Normal : pSafeFallbackTexture);
            }
            matCBData.UseNormalMap = mat.Normal ? 1.0f : 0.0f;

            if (auto* pAOVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_AOMap")) {
                pAOVar->Set(mat.AO ? mat.AO : pSafeFallbackTexture);
            }
            matCBData.UseAOMap = mat.AO ? 1.0f : 0.0f;

            if (auto* pEmissiveVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_EmissiveMap")) {
                pEmissiveVar->Set(mat.Emissive ? mat.Emissive : pSafeFallbackTexture);
            }
            matCBData.UseEmissiveMap = mat.Emissive ? 1.0f : 0.0f;

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

void Diligent::BasePipeline::RenderModels(IDeviceContext* pContext, RenderData renderData)
{
    for (int i = 0; i < renderData.Models.size(); i++) {
        RenderModel(pContext, renderData.Models[i]);
    }
}

void Diligent::BasePipeline::CreateCameraConstantBuffer(IRenderDevice* pDevice)
{
    BufferDesc CBDesc;
    CBDesc.Name = "Camera Constant Buffer";
    CBDesc.Size = sizeof(float4x4) * 2;
    CBDesc.Usage = USAGE_DYNAMIC;
    CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

    pDevice->CreateBuffer(CBDesc, nullptr, &m_pCameraCB);
}

void Diligent::BasePipeline::CreateModelConstantBuffer(IRenderDevice* pDevice)
{
    BufferDesc CBDesc;
    CBDesc.Name = "Model Constant Buffer";
    CBDesc.Size = sizeof(float4x4);
    CBDesc.Usage = USAGE_DYNAMIC;
    CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

    pDevice->CreateBuffer(CBDesc, nullptr, &m_pModelCB);
}

void Diligent::BasePipeline::CreateCommonConstantBuffers(IRenderDevice* pDevice)
{
    if (!m_pMaterialCB) {
        BufferDesc MatCBDesc;
        MatCBDesc.Name = "PBR Material Constant Buffer";
        MatCBDesc.Size = sizeof(PBRMaterialConstants);
        MatCBDesc.Usage = USAGE_DYNAMIC;
        MatCBDesc.BindFlags = BIND_UNIFORM_BUFFER;
        MatCBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        pDevice->CreateBuffer(MatCBDesc, nullptr, &m_pMaterialCB);
    }

    if (!m_pLightCB) {
        BufferDesc LightCBDesc;
        LightCBDesc.Name = "Light Constant Buffer";
        LightCBDesc.Size = sizeof(LightConstants);
        LightCBDesc.Usage = USAGE_DYNAMIC;
        LightCBDesc.BindFlags = BIND_UNIFORM_BUFFER;
        LightCBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        pDevice->CreateBuffer(LightCBDesc, nullptr, &m_pLightCB);
    }

    if (!m_pShadowCB) {
        BufferDesc ShadowCBDesc;
        ShadowCBDesc.Name = "Shadow Settings Constant Buffer";
        ShadowCBDesc.Size = sizeof(ShadowSettings);
        ShadowCBDesc.Usage = USAGE_DYNAMIC;
        ShadowCBDesc.BindFlags = BIND_UNIFORM_BUFFER;
        ShadowCBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        pDevice->CreateBuffer(ShadowCBDesc, nullptr, &m_pShadowCB);
    }
}

void Diligent::BasePipeline::SetupDefaultGraphicsPipeline(GraphicsPipelineDesc& GraphicsPipeline)
{
    GraphicsPipeline.NumRenderTargets = 1;
    GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;
    GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_LESS_EQUAL;
}

SamplerDesc Diligent::BasePipeline::GetLinearWrapSamplerDesc()
{
    SamplerDesc SamLinearWrapDesc;
    SamLinearWrapDesc.MinFilter = FILTER_TYPE_LINEAR;
    SamLinearWrapDesc.MagFilter = FILTER_TYPE_LINEAR;
    SamLinearWrapDesc.MipFilter = FILTER_TYPE_LINEAR;
    SamLinearWrapDesc.AddressU = TEXTURE_ADDRESS_WRAP;
    SamLinearWrapDesc.AddressV = TEXTURE_ADDRESS_WRAP;
    SamLinearWrapDesc.AddressW = TEXTURE_ADDRESS_WRAP;

    return SamLinearWrapDesc;
}

std::vector<ShaderResourceVariableDesc> Diligent::BasePipeline::GetStandardShaderVariables()
{
    return {
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "CameraConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "ModelConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "LightConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "PBRMaterialConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "ShadowSettings", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

        // Dynamic Textures for Material Submeshes
        {SHADER_TYPE_PIXEL, "g_BaseColorMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_MetallicRoughnessMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_NormalMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_AOMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_EmissiveMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
}

std::vector<ImmutableSamplerDesc> Diligent::BasePipeline::GetStandardImmutableSamplers()
{
    SamplerDesc SamLinearWrapDesc = GetLinearWrapSamplerDesc();

    return {
        {SHADER_TYPE_PIXEL, "g_BaseColorMap_sampler", SamLinearWrapDesc},
        {SHADER_TYPE_PIXEL, "g_MetallicRoughnessMap_sampler", SamLinearWrapDesc},
        {SHADER_TYPE_PIXEL, "g_NormalMap_sampler", SamLinearWrapDesc},
        {SHADER_TYPE_PIXEL, "g_AOMap_sampler", SamLinearWrapDesc},
        {SHADER_TYPE_PIXEL, "g_EmissiveMap_sampler", SamLinearWrapDesc}
    };
}