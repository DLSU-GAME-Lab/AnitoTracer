#include "PostProcessPipeline.hpp"
#include "../Utils/PSOBuilder.hpp"

namespace Diligent {

    PostProcessPipeline::PostProcessPipeline(const Desc& desc) : m_Desc(desc) {}

    void PostProcessPipeline::Initialize(IRenderDevice* pDevice, TEXTURE_FORMAT RTVFormat)
    {
        m_pDevice = pDevice;
        m_Format = RTVFormat;
        m_pIntermediate.Release();

        m_pPSO = PSOBuilder(std::string(m_Desc.Name) + " PSO")
            .Shaders("Deferred/fullscreen_vs.hlsl", m_Desc.PixelShader)
            .RenderTargets({ RTVFormat }, TEX_FORMAT_UNKNOWN)
            .Cull(CULL_MODE_NONE)
            .Variable(SHADER_TYPE_PIXEL, "g_SceneColor")
            .Variable(SHADER_TYPE_PIXEL, m_Desc.ConstantsName)
            .Sampler(SHADER_TYPE_PIXEL, "g_SceneColor_sampler", m_Desc.SceneSampler)
            .Build(pDevice);
        m_pSRB = PipelineUtils::CreateSRB(m_pPSO);

        const std::string cbName = std::string(m_Desc.Name) + " Constant Buffer";
        m_pConstantsCB = PipelineUtils::CreateDynamicCB(pDevice, cbName.c_str(), m_Desc.ConstantsSize);
        PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, m_Desc.ConstantsName, m_pConstantsCB);
    }

    ITextureView* PostProcessPipeline::GetIntermediateRTV(Uint32 width, Uint32 height)
    {
        if (!m_pDevice || width == 0 || height == 0) return nullptr;

        if (!m_pIntermediate ||
            m_pIntermediate->GetDesc().Width != width ||
            m_pIntermediate->GetDesc().Height != height)
        {
            const std::string name = std::string(m_Desc.Name) + " Intermediate";

            TextureDesc TexDesc;
            TexDesc.Name = name.c_str();
            TexDesc.Type = RESOURCE_DIM_TEX_2D;
            TexDesc.Width = width;
            TexDesc.Height = height;
            TexDesc.Format = m_Format;
            TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;

            m_pIntermediate.Release();
            m_pDevice->CreateTexture(TexDesc, nullptr, &m_pIntermediate);
        }

        return m_pIntermediate ? m_pIntermediate->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET) : nullptr;
    }

    void PostProcessPipeline::Apply(IDeviceContext* pContext, ITextureView* pDstRTV)
    {
        if (!m_pPSO || !m_pSRB || !m_pIntermediate || !pDstRTV) return;

        WriteConstants(pContext, m_pConstantsCB);
        PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, "g_SceneColor", m_pIntermediate->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

        pContext->SetRenderTargets(1, &pDstRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        const auto& DstDesc = pDstRTV->GetTexture()->GetDesc();
        PipelineUtils::SetViewport(pContext, DstDesc.Width, DstDesc.Height, DstDesc.Width, DstDesc.Height);

        pContext->SetPipelineState(m_pPSO);
        pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        PipelineUtils::DrawFullscreenTriangle(pContext);
    }

}
