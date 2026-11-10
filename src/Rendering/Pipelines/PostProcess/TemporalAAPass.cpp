#include "TemporalAAPass.hpp"
#include "../Utils/PSOBuilder.hpp"
#include "../Utils/PipelineUtils.hpp"

namespace Diligent {

    namespace {
        // Filtered precision for the accumulated colors; an 8-bit history visibly bands as it converges
        constexpr TEXTURE_FORMAT kHistoryFormat = TEX_FORMAT_RGBA16_FLOAT;

        // Jitter positions cycle through the first kJitterSampleCount points of the Halton(2, 3) sequence
        constexpr Uint32 kJitterSampleCount = 8;

        float Halton(Uint32 index, Uint32 base)
        {
            float fraction = 1.0f;
            float result = 0.0f;
            while (index > 0) {
                fraction /= static_cast<float>(base);
                result += fraction * static_cast<float>(index % base);
                index /= base;
            }
            return result;
        }
    }

    void TemporalAAPass::Initialize(IRenderDevice* pDevice, TEXTURE_FORMAT RTVFormat)
    {
        m_pDevice = pDevice;
        m_Format = RTVFormat;
        m_pInput.Release();
        m_pHistory[0].Release();
        m_pHistory[1].Release();
        m_Width = 0;
        m_Height = 0;
        m_HistoryValid = false;

        m_pPSO = PSOBuilder("Temporal AA PSO")
            .Shaders("Deferred/fullscreen_vs.hlsl", "PostProcess/taa_ps.hlsl")
            .RenderTargets({ RTVFormat, kHistoryFormat }, TEX_FORMAT_UNKNOWN)
            .Cull(CULL_MODE_NONE)
            .Variable(SHADER_TYPE_PIXEL, "g_CurrentColor")
            .Variable(SHADER_TYPE_PIXEL, "g_HistoryColor")
            .Variable(SHADER_TYPE_PIXEL, "g_SceneDepth")
            .Variable(SHADER_TYPE_PIXEL, "TemporalAAConstants")
            .Sampler(SHADER_TYPE_PIXEL, "g_HistoryColor_sampler", PipelineUtils::LinearClampSampler())
            .Build(pDevice);
        m_pSRB = PipelineUtils::CreateSRB(m_pPSO);

        if (!m_pConstantsCB) m_pConstantsCB = PipelineUtils::CreateDynamicCB<TemporalAAConstants>(pDevice, "Temporal AA Constant Buffer");
        PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, "TemporalAAConstants", m_pConstantsCB);

        // Bound when there is no scene depth; the shader doesn't read it then
        if (!m_pFallbackDepth) m_pFallbackDepth = PipelineUtils::CreateSolidColorTexture(pDevice, "Temporal AA Fallback Depth", 0xFFFFFFFF);
    }

    void TemporalAAPass::CreateTextures(Uint32 width, Uint32 height)
    {
        m_pInput.Release();
        m_pHistory[0].Release();
        m_pHistory[1].Release();
        m_Width = width;
        m_Height = height;
        m_HistoryValid = false;

        TextureDesc TexDesc;
        TexDesc.Type = RESOURCE_DIM_TEX_2D;
        TexDesc.Width = width;
        TexDesc.Height = height;
        TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;

        TexDesc.Name = "Temporal AA Input";
        TexDesc.Format = m_Format;
        m_pDevice->CreateTexture(TexDesc, nullptr, &m_pInput);

        const char* historyNames[] = { "Temporal AA History 0", "Temporal AA History 1" };
        TexDesc.Format = kHistoryFormat;
        for (Uint32 i = 0; i < 2; ++i) {
            TexDesc.Name = historyNames[i];
            m_pDevice->CreateTexture(TexDesc, nullptr, &m_pHistory[i]);
        }
    }

    ITextureView* TemporalAAPass::Begin(const TAASettings& settings, const glm::mat4& view, const glm::mat4& proj, Uint32 width, Uint32 height)
    {
        if (!m_pDevice || !m_pPSO || width == 0 || height == 0) return nullptr;

        if (width != m_Width || height != m_Height || !m_pInput) {
            CreateTextures(width, height);
        }
        if (!m_pInput || !m_pHistory[0] || !m_pHistory[1]) return nullptr;

        // Offset in pixels within [-spread/2, spread/2], converted to NDC (2 units across the view)
        const Uint32 sampleIndex = (m_FrameIndex++ % kJitterSampleCount) + 1; // Halton index 0 is always 0
        const glm::vec2 jitterPixels = (glm::vec2(Halton(sampleIndex, 2), Halton(sampleIndex, 3)) - 0.5f) * settings.JitterSpread;
        const glm::vec2 jitterNDC = jitterPixels * glm::vec2(2.0f / static_cast<float>(width), 2.0f / static_cast<float>(height));

        // Translating after the projection shifts the whole image by the same amount in NDC, for perspective and orthographic cameras alike
        m_JitteredProj = glm::translate(glm::mat4(1.0f), glm::vec3(jitterNDC, 0.0f)) * proj;

        m_ViewProj = proj * view;
        if (!m_HistoryValid) m_PrevViewProj = m_ViewProj;

        m_Constants.InvViewProj = glm::transpose(glm::inverse(m_JitteredProj * view));
        m_Constants.ViewProj = glm::transpose(m_ViewProj);
        m_Constants.PrevViewProj = glm::transpose(m_PrevViewProj);
        m_Constants.ViewportSize = glm::vec4(width, height, 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height));
        m_Constants.HistoryBlend = glm::clamp(settings.HistoryBlend, 0.0f, 0.99f);
        m_Constants.Sharpness = (glm::max)(settings.Sharpness, 0.0f);

        return m_pInput->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    }

    void TemporalAAPass::Resolve(IDeviceContext* pContext, ITextureView* pDepthSRV, ITextureView* pDstRTV)
    {
        if (!m_pPSO || !m_pSRB || !m_pInput || !pDstRTV) return;

        const Uint32 readIndex = m_HistoryIndex;
        const Uint32 writeIndex = 1 - m_HistoryIndex;

        m_Constants.HistoryValid = m_HistoryValid ? 1.0f : 0.0f;
        m_Constants.HasDepth = pDepthSRV ? 1.0f : 0.0f;
        PipelineUtils::WriteCB(pContext, m_pConstantsCB, m_Constants);

        PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, "g_CurrentColor", m_pInput->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, "g_HistoryColor", m_pHistory[readIndex]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, "g_SceneDepth",
            pDepthSRV ? pDepthSRV : m_pFallbackDepth->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

        ITextureView* pRTVs[] = { pDstRTV, m_pHistory[writeIndex]->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET) };
        pContext->SetRenderTargets(2, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        PipelineUtils::SetViewport(pContext, m_Width, m_Height, m_Width, m_Height);

        pContext->SetPipelineState(m_pPSO);
        pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        PipelineUtils::DrawFullscreenTriangle(pContext);

        m_HistoryIndex = writeIndex;
        m_HistoryValid = true;
        m_PrevViewProj = m_ViewProj;
    }

}
