#include "BasePipeline.hpp"
#include "../RenderData.hpp"

using namespace Diligent;

void BasePipeline::SetDevice(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    m_pDevice = pDevice;
    m_pSwapChain = pSwapChain;
}

void BasePipeline::StartFrameRender(IDeviceContext* pContext, const RenderData& renderData)
{
    UpdateCameraConstants(pContext, renderData);
    pContext->SetPipelineState(m_pPSO);
}

void BasePipeline::UpdateCameraConstants(IDeviceContext* pContext, const RenderData& renderData)
{
    CameraConstants camera;
    camera.View = glm::transpose(renderData.ViewMatrix);
    camera.Proj = glm::transpose(renderData.ProjectionMatrix);
    PipelineUtils::WriteCB(pContext, m_pCameraCB, camera);
}

void BasePipeline::RenderModel(IDeviceContext* pContext, const ModelRenderInstance& modelData, bool renderOpaque)
{
    RenderModelWithSRB(pContext, modelData, renderOpaque, m_pSRB);
}

void BasePipeline::RenderModelWithSRB(IDeviceContext* pContext, const ModelRenderInstance& modelData, bool renderOpaque, IShaderResourceBinding* pSRB)
{
    Model* model = modelData.ModelData;

    {
        ModelConstants modelConstants;
        modelConstants.Model = glm::transpose(modelData.WorldTransform);

        // Calculate the normal matrix (inverse transpose of the model matrix)
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(modelData.WorldTransform));

        // Transpose again to convert from GLM column-major to HLSL row-major expectation
        // Note: glm::transpose(glm::transpose(inverse)) simplifies to just inverse,
        // but writing it this way keeps the intent explicit and matches the Model matrix mapping.
        modelConstants.NormalMatrix = glm::transpose(normalMatrix);
        PipelineUtils::WriteCB(pContext, m_pModelCB, modelConstants);
    }

    IBuffer* pBuffs[] = { model->pVertexBuffer };
    pContext->SetVertexBuffers(0, 1, pBuffs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(model->pIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Determine which list of submesh indices to iterate over
    const auto& targetIndices = renderOpaque ? modelData.OpaqueSubmeshIndices : modelData.TransparentSubmeshIndices;

    for (uint32_t submeshIdx : targetIndices) {
        const auto& submesh = model->SubMeshes[submeshIdx];

        if (submesh.MaterialIndex < model->PBRMaterials.size()) {
            const PBRMaterial& mat = model->PBRMaterials[submesh.MaterialIndex];

            auto colorFactor = mat.BaseColorFactor;
            PBRMaterialConstants matCBData{};
            matCBData.BaseColorFactor = glm::vec4(colorFactor.r, colorFactor.g, colorFactor.b, colorFactor.a);
            matCBData.MetallicFactor = modelData.OverrideMetallic ? modelData.Metallic : mat.MetallicFactor;
            matCBData.RoughnessFactor = modelData.OverrideRoughness ? modelData.Roughness : mat.RoughnessFactor;
            matCBData.OverrideFlags = static_cast<float>((modelData.OverrideRoughness ? 1 : 0) | (modelData.OverrideMetallic ? 2 : 0));

            const glm::vec3 materialEmissive = glm::vec3(mat.EmissiveFactor.r, mat.EmissiveFactor.g, mat.EmissiveFactor.b) * mat.EmissiveStrength;
            matCBData.EmissiveFactor = glm::vec4(modelData.OverrideEmissive ? modelData.Emissive : materialEmissive, 0.0f);

            // Missing maps still need a valid texture bound; the Use*Map flags tell the shader to ignore it
            ITextureView* pSafeFallbackTexture = mat.BaseColor;
            auto BindMap = [&](const char* name, ITextureView* pMap, float& useFlag) {
                PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, name, pMap ? pMap : pSafeFallbackTexture);
                useFlag = pMap ? 1.0f : 0.0f;
            };

            BindMap("g_BaseColorMap", mat.BaseColor, matCBData.UseBaseColorMap);
            BindMap("g_MetallicRoughnessMap", mat.MetallicRoughness, matCBData.UseMetallicRoughnessMap);
            BindMap("g_NormalMap", mat.Normal, matCBData.UseNormalMap);
            BindMap("g_AOMap", mat.AO, matCBData.UseAOMap);
            BindMap("g_EmissiveMap", mat.Emissive, matCBData.UseEmissiveMap);

            PipelineUtils::WriteCB(pContext, m_pMaterialCB, matCBData);
        }

        pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawIndexedAttribs DrawAttrs;
        DrawAttrs.IndexType = VT_UINT32;
        DrawAttrs.NumIndices = submesh.IndexCount;
        DrawAttrs.FirstIndexLocation = submesh.IndexOffset;
        DrawAttrs.BaseVertex = 0;
        DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;

        pContext->DrawIndexed(DrawAttrs);
    }
}

void BasePipeline::RenderModels(IDeviceContext* pContext, const RenderData& renderData, bool renderOpaque)
{
    for (const ModelRenderInstance& model : renderData.Models) {
        RenderModel(pContext, model, renderOpaque);
    }
}

void BasePipeline::CreateCameraConstantBuffer()
{
    if (!m_pCameraCB) m_pCameraCB = PipelineUtils::CreateDynamicCB<CameraConstants>(m_pDevice, "Camera Constant Buffer");
}

void BasePipeline::CreateModelConstantBuffer()
{
    if (!m_pModelCB) m_pModelCB = PipelineUtils::CreateDynamicCB<ModelConstants>(m_pDevice, "Model Constant Buffer");
}

void BasePipeline::CreateMaterialConstantBuffer()
{
    if (!m_pMaterialCB) m_pMaterialCB = PipelineUtils::CreateDynamicCB<PBRMaterialConstants>(m_pDevice, "PBR Material Constant Buffer");
}

void BasePipeline::BindCommonBuffers(IShaderResourceBinding* pSRB)
{
    PipelineUtils::BindVariable(pSRB, SHADER_TYPE_VERTEX, "CameraConstants", m_pCameraCB);
    PipelineUtils::BindVariable(pSRB, SHADER_TYPE_VERTEX, "ModelConstants", m_pModelCB);
    PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "PBRMaterialConstants", m_pMaterialCB);
}

PSOBuilder BasePipeline::CreateModelPSOBuilder(const char* name, const char* psFile, const char* vsFile)
{
    PSOBuilder builder(name);
    builder.Shaders(vsFile, psFile)
        .RenderTargets({ GetColorFormat() }, GetDepthFormat())
        .InputLayout(VertexLayouts::GetStandardLayout())
        .Variables(GetStandardShaderVariables())
        .Samplers(GetStandardImmutableSamplers());
    return builder;
}

std::vector<ShaderResourceVariableDesc> BasePipeline::GetStandardShaderVariables()
{
    return {
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "CameraConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "ModelConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "LightConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "PBRMaterialConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "ShadowSettings", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "FogConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},

        // Dynamic Textures for Material Submeshes
        {SHADER_TYPE_PIXEL, "g_BaseColorMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_MetallicRoughnessMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_NormalMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_AOMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_EmissiveMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
}

std::vector<ImmutableSamplerDesc> BasePipeline::GetStandardImmutableSamplers()
{
    SamplerDesc SamLinearWrapDesc = PipelineUtils::LinearWrapSampler();

    return {
        {SHADER_TYPE_PIXEL, "g_BaseColorMap_sampler", SamLinearWrapDesc},
        {SHADER_TYPE_PIXEL, "g_MetallicRoughnessMap_sampler", SamLinearWrapDesc},
        {SHADER_TYPE_PIXEL, "g_NormalMap_sampler", SamLinearWrapDesc},
        {SHADER_TYPE_PIXEL, "g_AOMap_sampler", SamLinearWrapDesc},
        {SHADER_TYPE_PIXEL, "g_EmissiveMap_sampler", SamLinearWrapDesc}
    };
}
