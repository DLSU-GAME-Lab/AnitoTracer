#include "SignalDenoiser.hpp"
#include "PSOBuilder.hpp"
#include "PipelineUtils.hpp"
#include <algorithm>
#include <cmath>

namespace Diligent {

    namespace {
        // Signal sizes kept in the scratch cache: full and half resolution, plus one spare while the view is resized
        constexpr size_t kMaxScratchSizes = 3;

        // Mirrors cbuffer DenoiseConstants in Shaders/DeferredHybrid/denoise_common.hlsli. Matrices are transposed like CameraConstants.
        struct DenoiseConstants
        {
            glm::mat4 InvViewProj{ 1.0f };          // This frame's clip space -> world
            glm::mat4 ViewProjNoJitter{ 1.0f };     // World -> clip space this frame, without TAA jitter
            glm::mat4 PrevViewProjNoJitter{ 1.0f }; // The same for last frame
            glm::vec4 CameraPos{ 0.0f };
            glm::vec4 PrevCameraPos{ 0.0f };
            glm::vec4 ViewportSize{ 0.0f };         // xy: view size in pixels, zw: 1 / size
            glm::vec4 Size{ 0.0f };                 // xy: signal size in pixels, zw: 1 / size

            uint32_t Scale = 1;                     // View pixels per signal pixel along each axis
            uint32_t Kind = 0;
            float HistoryValid = 0.0f;              // 0: there is no last frame to blend with
            float MinBlend = 0.1f;

            float ClampSigma = 0.0f;
            float BrightnessSigma = 0.0f;
            int32_t FilterStep = 1;                 // Pixel spacing of the blur pass being drawn
            float UncertaintyScale = 1.0f;          // Shrinks the uncertainty for the later blur passes

            float PixelAngle = 0.0f;
            float MaxRoughness = 1.0f;
            float VirtualMotion = 0.0f;
            float Padding = 0.0f;
        };
        static_assert(sizeof(DenoiseConstants) % 16 == 0, "DenoiseConstants must be 16-byte aligned for HLSL");

        RefCntAutoPtr<ITexture> CreateTarget(IRenderDevice* pDevice, const char* name, Uint32 width, Uint32 height)
        {
            TextureDesc Desc;
            Desc.Name = name;
            Desc.Type = RESOURCE_DIM_TEX_2D;
            Desc.Width = width;
            Desc.Height = height;
            Desc.Format = SignalDenoiser::kSignalFormat;
            Desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;

            RefCntAutoPtr<ITexture> pTexture;
            pDevice->CreateTexture(Desc, nullptr, &pTexture);
            return pTexture;
        }

        ITextureView* SRV(ITexture* pTexture) { return pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE); }
        ITextureView* RTV(ITexture* pTexture) { return pTexture->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET); }
    }

    ITextureView* SignalDenoiser::History::GetResultSRV() const
    {
        return (m_Valid && m_pLastResult) ? SRV(m_pLastResult) : nullptr;
    }

    void SignalDenoiser::Initialize(IRenderDevice* pDevice)
    {
        m_pDevice = pDevice;
        if (!m_pConstantsCB) m_pConstantsCB = PipelineUtils::CreateDynamicCB<DenoiseConstants>(pDevice, "Signal Denoiser Constant Buffer");

        // Every texture is dynamic: the same two passes serve every signal, with other textures each time

        // 1. Temporal: this frame's rays and last frame's result in; the new result, its moments and its variance out
        m_pTemporalPSO = PSOBuilder("Signal Denoiser Temporal PSO")
            .Shaders("Deferred/fullscreen_vs.hlsl", "DeferredHybrid/denoise_temporal_ps.hlsl")
            .RenderTargets({ kSignalFormat, kSignalFormat, kSignalFormat }, TEX_FORMAT_UNKNOWN)
            .Cull(CULL_MODE_NONE)
            .Variable(SHADER_TYPE_PIXEL, "g_DnSignal")
            .Variable(SHADER_TYPE_PIXEL, "g_DnGuide")
            .Variable(SHADER_TYPE_PIXEL, "g_DnPrevGuide")
            .Variable(SHADER_TYPE_PIXEL, "g_DnVelocity")
            .Variable(SHADER_TYPE_PIXEL, "g_DnPrevSignal")
            .Variable(SHADER_TYPE_PIXEL, "g_DnPrevMoments")
            .Variable(SHADER_TYPE_PIXEL, "DenoiseConstants")
            .Build(pDevice);
        m_pTemporalSRB = PipelineUtils::CreateSRB(m_pTemporalPSO);
        PipelineUtils::BindVariable(m_pTemporalSRB, SHADER_TYPE_PIXEL, "DenoiseConstants", m_pConstantsCB);

        // 2. Filter: one edge-stopping blur pass, run several times with a growing step
        m_pFilterPSO = PSOBuilder("Signal Denoiser Filter PSO")
            .Shaders("Deferred/fullscreen_vs.hlsl", "DeferredHybrid/denoise_filter_ps.hlsl")
            .RenderTargets({ kSignalFormat }, TEX_FORMAT_UNKNOWN)
            .Cull(CULL_MODE_NONE)
            .Variable(SHADER_TYPE_PIXEL, "g_DnSource")
            .Variable(SHADER_TYPE_PIXEL, "g_DnGuide")
            .Variable(SHADER_TYPE_PIXEL, "g_DnVariance")
            .Variable(SHADER_TYPE_PIXEL, "DenoiseConstants")
            .Build(pDevice);
        m_pFilterSRB = PipelineUtils::CreateSRB(m_pFilterPSO);
        PipelineUtils::BindVariable(m_pFilterSRB, SHADER_TYPE_PIXEL, "DenoiseConstants", m_pConstantsCB);
    }

    bool SignalDenoiser::PrepareHistory(History& history, Uint32 width, Uint32 height)
    {
        const bool complete = history.m_pSignal[0] && history.m_pSignal[1] && history.m_pMoments[0] && history.m_pMoments[1] && history.m_pResult;
        if (complete && history.m_Width == width && history.m_Height == height) return true;

        history.m_Width = width;
        history.m_Height = height;
        history.m_Valid = false;
        history.m_pLastResult = nullptr;

        const char* signalNames[] = { "Denoiser History Signal 0", "Denoiser History Signal 1" };
        const char* momentNames[] = { "Denoiser History Moments 0", "Denoiser History Moments 1" };
        for (Uint32 i = 0; i < 2; ++i) {
            history.m_pSignal[i] = CreateTarget(m_pDevice, signalNames[i], width, height);
            history.m_pMoments[i] = CreateTarget(m_pDevice, momentNames[i], width, height);
        }
        history.m_pResult = CreateTarget(m_pDevice, "Denoiser Result", width, height);

        return history.m_pSignal[0] && history.m_pSignal[1] && history.m_pMoments[0] && history.m_pMoments[1] && history.m_pResult;
    }

    SignalDenoiser::Scratch* SignalDenoiser::GetScratch(Uint32 width, Uint32 height)
    {
        for (Scratch& scratch : m_Scratch) {
            if (scratch.Width == width && scratch.Height == height) return &scratch;
        }

        // The oldest size is the one a resize left behind
        if (m_Scratch.size() >= kMaxScratchSizes) m_Scratch.erase(m_Scratch.begin());

        Scratch scratch;
        scratch.Width = width;
        scratch.Height = height;
        scratch.pTemporal = CreateTarget(m_pDevice, "Denoiser Scratch Temporal", width, height);
        scratch.pVariance = CreateTarget(m_pDevice, "Denoiser Scratch Variance", width, height);
        scratch.pPing[0] = CreateTarget(m_pDevice, "Denoiser Scratch Blur 0", width, height);
        scratch.pPing[1] = CreateTarget(m_pDevice, "Denoiser Scratch Blur 1", width, height);
        if (!scratch.pTemporal || !scratch.pVariance || !scratch.pPing[0] || !scratch.pPing[1]) return nullptr;

        m_Scratch.push_back(scratch);
        return &m_Scratch.back();
    }

    ITextureView* SignalDenoiser::Denoise(IDeviceContext* pContext, const Frame& frame, History& history, ITextureView* pSignal,
        Uint32 scale, const Settings& settings)
    {
        // Anything missing: hand the rays back as they are
        if (!IsInitialized() || !pSignal || !frame.pGuide || frame.Width == 0 || frame.Height == 0) {
            history.Reset();
            return pSignal;
        }

        scale = (std::max)(scale, 1u);
        const Uint32 width = (frame.Width + scale - 1) / scale;
        const Uint32 height = (frame.Height + scale - 1) / scale;

        Scratch* pScratch = GetScratch(width, height);
        if (!PrepareHistory(history, width, height) || !pScratch) {
            history.Reset();
            return pSignal;
        }

        const Uint32 previous = history.m_Current;
        const Uint32 current = 1 - history.m_Current;
        const bool historyValid = frame.HistoryValid && history.m_Valid && frame.pPrevGuide && frame.pVelocity;
        const int passes = glm::clamp(settings.FilterPasses, 0, kMaxFilterPasses);

        DenoiseConstants constants;
        constants.InvViewProj = glm::transpose(glm::inverse(frame.ViewProj));
        constants.ViewProjNoJitter = glm::transpose(frame.ViewProjNoJitter);
        constants.PrevViewProjNoJitter = glm::transpose(frame.PrevViewProjNoJitter);
        constants.CameraPos = glm::vec4(frame.CameraPos, 1.0f);
        constants.PrevCameraPos = glm::vec4(frame.PrevCameraPos, 1.0f);
        constants.ViewportSize = glm::vec4(frame.Width, frame.Height, 1.0f / static_cast<float>(frame.Width), 1.0f / static_cast<float>(frame.Height));
        constants.Size = glm::vec4(width, height, 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height));
        constants.Scale = scale;
        constants.Kind = settings.SignalKind;
        constants.HistoryValid = historyValid ? 1.0f : 0.0f;
        constants.MinBlend = glm::clamp(settings.MinBlend, 0.005f, 1.0f);
        constants.ClampSigma = (std::max)(settings.ClampSigma, 0.0f);
        constants.BrightnessSigma = (std::max)(settings.BrightnessSigma, 0.0f);
        constants.PixelAngle = frame.PixelAngle;
        constants.MaxRoughness = settings.MaxRoughness;
        constants.VirtualMotion = settings.VirtualMotion ? 1.0f : 0.0f;
        PipelineUtils::WriteCB(pContext, m_pConstantsCB, constants);

        // 1. Temporal. With blur passes it writes a scratch texture, and the first blur pass writes the texture
        //    kept for next frame: blurring a little before the result is reused lets the blur build up over frames,
        //    which removes far more noise than the same passes on one frame could. The blur stops at edges, so it
        //    doesn't smear across them however long it builds up.
        ITexture* pTemporalTarget = passes > 0 ? pScratch->pTemporal.RawPtr() : history.m_pSignal[current].RawPtr();
        {
            // Nothing reads last frame's textures while the history is invalid, but every variable needs one bound
            PipelineUtils::BindVariable(m_pTemporalSRB, SHADER_TYPE_PIXEL, "g_DnSignal", pSignal);
            PipelineUtils::BindVariable(m_pTemporalSRB, SHADER_TYPE_PIXEL, "g_DnGuide", frame.pGuide);
            PipelineUtils::BindVariable(m_pTemporalSRB, SHADER_TYPE_PIXEL, "g_DnPrevGuide", frame.pPrevGuide ? frame.pPrevGuide : frame.pGuide);
            PipelineUtils::BindVariable(m_pTemporalSRB, SHADER_TYPE_PIXEL, "g_DnVelocity", frame.pVelocity ? frame.pVelocity : frame.pGuide);
            PipelineUtils::BindVariable(m_pTemporalSRB, SHADER_TYPE_PIXEL, "g_DnPrevSignal", SRV(history.m_pSignal[previous]));
            PipelineUtils::BindVariable(m_pTemporalSRB, SHADER_TYPE_PIXEL, "g_DnPrevMoments", SRV(history.m_pMoments[previous]));

            ITextureView* pRTVs[] = { RTV(pTemporalTarget), RTV(history.m_pMoments[current]), RTV(pScratch->pVariance) };
            pContext->SetRenderTargets(3, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            pContext->SetPipelineState(m_pTemporalPSO);
            pContext->CommitShaderResources(m_pTemporalSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            PipelineUtils::DrawFullscreenTriangle(pContext);
        }

        // 2. Filter. The first pass produces next frame's history; the wider ones after it only shape what is shown,
        //    and the last of them writes the texture this signal keeps as its result.
        ITexture* pResult = pTemporalTarget;
        for (int pass = 0; pass < passes; ++pass) {
            ITexture* pDestination = pass == 0 ? history.m_pSignal[current].RawPtr()
                : pass == passes - 1 ? history.m_pResult.RawPtr()
                : pScratch->pPing[pass % 2].RawPtr();

            constants.FilterStep = 1 << pass;
            // Each pass leaves the signal cleaner, so differences between neighbours mean more
            constants.UncertaintyScale = std::pow(0.5f, static_cast<float>(pass));
            PipelineUtils::WriteCB(pContext, m_pConstantsCB, constants);

            PipelineUtils::BindVariable(m_pFilterSRB, SHADER_TYPE_PIXEL, "g_DnSource", SRV(pResult));
            PipelineUtils::BindVariable(m_pFilterSRB, SHADER_TYPE_PIXEL, "g_DnGuide", frame.pGuide);
            PipelineUtils::BindVariable(m_pFilterSRB, SHADER_TYPE_PIXEL, "g_DnVariance", SRV(pScratch->pVariance));

            ITextureView* pRTV = RTV(pDestination);
            pContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            pContext->SetPipelineState(m_pFilterPSO);
            pContext->CommitShaderResources(m_pFilterSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            PipelineUtils::DrawFullscreenTriangle(pContext);

            pResult = pDestination;
        }

        history.m_pLastResult = pResult;
        history.m_Current = current;
        history.m_Valid = true;
        return SRV(pResult);
    }

}
