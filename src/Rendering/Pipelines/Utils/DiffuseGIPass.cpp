#include "DiffuseGIPass.hpp"
#include "PSOBuilder.hpp"
#include "PipelineUtils.hpp"
#include <algorithm>

namespace Diligent {

    namespace {
        // Light is stored in half floats: plenty for HDR, and filtered light bands visibly in anything smaller
        constexpr TEXTURE_FORMAT kLightFormat = TEX_FORMAT_RGBA16_FLOAT;
        constexpr TEXTURE_FORMAT kGuideFormat = TEX_FORMAT_RGBA16_FLOAT;
        constexpr TEXTURE_FORMAT kHistoryLengthFormat = TEX_FORMAT_R16_FLOAT;

        constexpr int kMaxFilterPasses = 5;
        constexpr int kMaxRaysPerPixel = 8;

        // Bits of GIConstants::Flags. Mirror the GI_FLAG_* constants in Shaders/DeferredHybrid/gi_common.hlsli
        enum GIFlags : uint32_t {
            GI_FLAG_HIT_SHADOWS = 1u << 0,
            GI_FLAG_SKY_BOUNCE = 1u << 1,
            GI_FLAG_MORE_BOUNCES = 1u << 2,
        };

        // Mirrors cbuffer GIConstants in Shaders/DeferredHybrid/gi_common.hlsli. Matrices are transposed like CameraConstants.
        struct GIConstants
        {
            glm::mat4 InvViewProj{ 1.0f };      // This frame's clip space -> world
            glm::mat4 PrevViewProj{ 1.0f };     // World -> last frame's clip space
            glm::vec4 CameraPos{ 0.0f };
            glm::vec4 PrevCameraPos{ 0.0f };
            glm::vec4 ViewportSize{ 0.0f };     // xy: view size in pixels, zw: 1 / size
            glm::vec4 Size{ 0.0f };             // xy: GI buffer size in pixels, zw: 1 / size

            uint32_t Scale = 2;                 // View pixels per GI pixel along each axis
            uint32_t Frame = 0;                 // Counts up every frame; varies the rays so they add up over time
            float HistoryValid = 0.0f;          // 0: there is no last frame to blend with
            float MinBlend = 0.05f;             // Smallest share the new frame gets

            float MaxDistance = 100.0f;
            uint32_t Flags = 0;
            int32_t FilterStep = 1;             // Pixel spacing of the blur pass being drawn
            uint32_t RayCount = 1;              // Rays traced per GI pixel every frame
        };
        static_assert(sizeof(GIConstants) % 16 == 0, "GIConstants must be 16-byte aligned for HLSL");

        RefCntAutoPtr<ITexture> CreateTarget(IRenderDevice* pDevice, const char* name, TEXTURE_FORMAT format, Uint32 width, Uint32 height)
        {
            TextureDesc Desc;
            Desc.Name = name;
            Desc.Type = RESOURCE_DIM_TEX_2D;
            Desc.Width = width;
            Desc.Height = height;
            Desc.Format = format;
            Desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;

            RefCntAutoPtr<ITexture> pTexture;
            pDevice->CreateTexture(Desc, nullptr, &pTexture);
            return pTexture;
        }

        ITextureView* SRV(ITexture* pTexture) { return pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE); }
        ITextureView* RTV(ITexture* pTexture) { return pTexture->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET); }
    }

    void DiffuseGIPass::Initialize(IRenderDevice* pDevice, const TraceSetup& rayTracing)
    {
        m_pDevice = pDevice;
        m_HistoryValid = false;
        m_pResultSRV = nullptr;
        m_pGuideSRV = nullptr;

        if (!m_pConstantsCB) m_pConstantsCB = PipelineUtils::CreateDynamicCB<GIConstants>(pDevice, "Diffuse GI Constant Buffer");

        // 1. Trace: the G-buffer in, this frame's rays and the guide out
        m_pTracePSO = PSOBuilder("Diffuse GI Trace PSO")
            .Shaders("Deferred/fullscreen_vs.hlsl", "DeferredHybrid/gi_trace_ps.hlsl")
            .PixelMacros(rayTracing.Macros)
            .RenderTargets({ kLightFormat, kGuideFormat }, TEX_FORMAT_UNKNOWN)
            .Cull(CULL_MODE_NONE)
            .Variable(SHADER_TYPE_PIXEL, "g_GBufferNormals", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
            .Variable(SHADER_TYPE_PIXEL, "g_GBufferDepth", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
            .Variable(SHADER_TYPE_PIXEL, "g_GIPrevAccum")
            .Variable(SHADER_TYPE_PIXEL, "g_GIPrevGuide")
            .Variable(SHADER_TYPE_PIXEL, "GIConstants")
            .Variable(SHADER_TYPE_PIXEL, "LightConstants")
            .Variable(SHADER_TYPE_PIXEL, "ShadowSettings")
            .Variables(rayTracing.Variables)
            .Samplers(rayTracing.Samplers)
            .Build(pDevice);
        m_pTraceSRB = PipelineUtils::CreateSRB(m_pTracePSO);
        PipelineUtils::BindVariable(m_pTraceSRB, SHADER_TYPE_PIXEL, "GIConstants", m_pConstantsCB);

        // 2. Accumulate: this frame's rays and last frame's result in, the new result and its history length out
        m_pAccumulatePSO = PSOBuilder("Diffuse GI Accumulate PSO")
            .Shaders("Deferred/fullscreen_vs.hlsl", "DeferredHybrid/gi_accumulate_ps.hlsl")
            .RenderTargets({ kLightFormat, kHistoryLengthFormat }, TEX_FORMAT_UNKNOWN)
            .Cull(CULL_MODE_NONE)
            .Variable(SHADER_TYPE_PIXEL, "g_GITrace")
            .Variable(SHADER_TYPE_PIXEL, "g_GIGuide")
            .Variable(SHADER_TYPE_PIXEL, "g_GIPrevAccum")
            .Variable(SHADER_TYPE_PIXEL, "g_GIPrevGuide")
            .Variable(SHADER_TYPE_PIXEL, "g_GIPrevHistoryLength")
            .Variable(SHADER_TYPE_PIXEL, "GIConstants")
            .Build(pDevice);
        m_pAccumulateSRB = PipelineUtils::CreateSRB(m_pAccumulatePSO);
        PipelineUtils::BindVariable(m_pAccumulateSRB, SHADER_TYPE_PIXEL, "GIConstants", m_pConstantsCB);

        // 3. Filter: one edge-stopping blur pass, run several times with a growing step
        m_pFilterPSO = PSOBuilder("Diffuse GI Filter PSO")
            .Shaders("Deferred/fullscreen_vs.hlsl", "DeferredHybrid/gi_filter_ps.hlsl")
            .RenderTargets({ kLightFormat }, TEX_FORMAT_UNKNOWN)
            .Cull(CULL_MODE_NONE)
            .Variable(SHADER_TYPE_PIXEL, "g_GISource")
            .Variable(SHADER_TYPE_PIXEL, "g_GIGuide")
            .Variable(SHADER_TYPE_PIXEL, "GIConstants")
            .Build(pDevice);
        m_pFilterSRB = PipelineUtils::CreateSRB(m_pFilterPSO);
        PipelineUtils::BindVariable(m_pFilterSRB, SHADER_TYPE_PIXEL, "GIConstants", m_pConstantsCB);
    }

    void DiffuseGIPass::CreateTextures(Uint32 width, Uint32 height)
    {
        m_Width = width;
        m_Height = height;
        m_HistoryValid = false;
        m_pLastResult = nullptr;
        m_pResultSRV = nullptr;
        m_pGuideSRV = nullptr;

        m_pTrace = CreateTarget
(m_pDevice, "Diffuse GI Rays", kLightFormat, width, height);
        m_pAccumRaw = CreateTarget(m_pDevice, "Diffuse GI Accumulated Unfiltered", kLightFormat, width, height);

        const char* guideNames[] = { "Diffuse GI Guide 0", "Diffuse GI Guide 1" };
        const char* accumNames[] = { "Diffuse GI Accumulated 0", "Diffuse GI Accumulated 1" };
        const char* lengthNames[] = { "Diffuse GI History Length 0", "Diffuse GI History Length 1" };
        const char* filteredNames[] = { "Diffuse GI Filtered 0", "Diffuse GI Filtered 1" };
        for (Uint32 i = 0; i < 2; ++i) {
            m_pGuide[i] = CreateTarget(m_pDevice, guideNames[i], kGuideFormat, width, height);
            m_pAccum[i] = CreateTarget(m_pDevice, accumNames[i], kLightFormat, width, height);
            m_pHistoryLength[i] = CreateTarget(m_pDevice, lengthNames[i], kHistoryLengthFormat, width, height);
            m_pFiltered[i] = CreateTarget(m_pDevice, filteredNames[i], kLightFormat, width, height);
        }
    }

    void DiffuseGIPass::Render(IDeviceContext* pContext, const View& view, const ProjectGISettings& settings)
    {
        if (!IsInitialized() || view.Width == 0 || view.Height == 0) return;

        const Uint32 scale = settings.half_resolution ? 2u : 1u;
        const Uint32 width = (view.Width + scale - 1) / scale;
        const Uint32 height = (view.Height + scale - 1) / scale;
        if (scale != m_Scale || width != m_Width || height != m_Height || !m_pTrace) {
            m_Scale = scale;
            CreateTextures(width, height);
        }
        if (!m_pTrace || !m_pAccumRaw || !m_pGuide[1] || !m_pAccum[1] || !m_pHistoryLength[1] || !m_pFiltered[1]) return;

        const Uint32 previous = m_Current;
        const Uint32 current = 1 - m_Current;

        GIConstants constants;
        constants.InvViewProj = glm::transpose(glm::inverse(view.ViewProj));
        constants.PrevViewProj = glm::transpose(m_HistoryValid ? m_PrevViewProj : view.ViewProj);
        constants.CameraPos = glm::vec4(view.CameraPos, 1.0f);
        constants.PrevCameraPos = glm::vec4(m_HistoryValid ? m_PrevCameraPos : view.CameraPos, 1.0f);
        constants.ViewportSize = glm::vec4(view.Width, view.Height, 1.0f / static_cast<float>(view.Width), 1.0f / static_cast<float>(view.Height));
        constants.Size = glm::vec4(width, height, 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height));
        constants.Scale = scale;
        constants.Frame = m_FrameIndex++;
        constants.HistoryValid = m_HistoryValid ? 1.0f : 0.0f;
        constants.MinBlend = 1.0f - glm::clamp(settings.history_blend, 0.0f, 0.99f);
        constants.MaxDistance = (std::max)(settings.max_distance, 0.01f);
        if (settings.hit_shadows) constants.Flags |= GI_FLAG_HIT_SHADOWS;
        if (settings.sky_bounce) constants.Flags |= GI_FLAG_SKY_BOUNCE;
        if (settings.more_bounces) constants.Flags |= GI_FLAG_MORE_BOUNCES;
        constants.RayCount = static_cast<uint32_t>(glm::clamp(settings.rays_per_pixel, 1, kMaxRaysPerPixel));
        PipelineUtils::WriteCB(pContext, m_pConstantsCB, constants);

        const int passes = glm::clamp(settings.filter_passes, 0, kMaxFilterPasses);

        // 1. Trace. Last frame's result is an input too: what a ray hits is lit by the GI gathered there before.
        //    It reads the fully blurred result that was displayed, not the lightly blurred one kept as history:
        //    whatever noise goes in comes back out with every bounce, so the smoother input matters.
        {
            ITexture* pLastResult = (m_HistoryValid && m_pLastResult) ? m_pLastResult : m_pAccum[previous].RawPtr();
            PipelineUtils::BindVariable(m_pTraceSRB, SHADER_TYPE_PIXEL, "g_GIPrevAccum", SRV(pLastResult));
            PipelineUtils::BindVariable(m_pTraceSRB, SHADER_TYPE_PIXEL, "g_GIPrevGuide", SRV(m_pGuide[previous]));

            ITextureView* pRTVs[] = { RTV(m_pTrace), RTV(m_pGuide[current]) };
            pContext->SetRenderTargets(2, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            pContext->SetPipelineState(m_pTracePSO);
            pContext->CommitShaderResources(m_pTraceSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            PipelineUtils::DrawFullscreenTriangle(pContext);
        }

        // 2. Accumulate. With blur passes it writes a scratch texture, and the first blur pass writes the texture
        //    kept for next frame: blurring a little before the result is reused lets the blur build up over frames,
        //    which removes far more noise than the same passes on one frame could. The blur stops at edges, so it
        //    doesn't smear across them however long it builds up.
        ITexture* pAccumTarget = passes > 0 ? m_pAccumRaw : m_pAccum[current];
        {
            PipelineUtils::BindVariable(m_pAccumulateSRB, SHADER_TYPE_PIXEL, "g_GITrace", SRV(m_pTrace));
            PipelineUtils::BindVariable(m_pAccumulateSRB, SHADER_TYPE_PIXEL, "g_GIGuide", SRV(m_pGuide[current]));
            PipelineUtils::BindVariable(m_pAccumulateSRB, SHADER_TYPE_PIXEL, "g_GIPrevAccum", SRV(m_pAccum[previous]));
            PipelineUtils::BindVariable(m_pAccumulateSRB, SHADER_TYPE_PIXEL, "g_GIPrevGuide", SRV(m_pGuide[previous]));
            PipelineUtils::BindVariable(m_pAccumulateSRB, SHADER_TYPE_PIXEL, "g_GIPrevHistoryLength", SRV(m_pHistoryLength[previous]));

            ITextureView* pRTVs[] = { RTV(pAccumTarget), RTV(m_pHistoryLength[current]) };
            pContext->SetRenderTargets(2, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            pContext->SetPipelineState(m_pAccumulatePSO);
            pContext->CommitShaderResources(m_pAccumulateSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            PipelineUtils::DrawFullscreenTriangle(pContext);
        }

        // 3. Filter. The first pass produces next frame's history; the wider ones after it only shape what is shown.
        ITexture* pResult = pAccumTarget;
        for (int pass = 0; pass < passes; ++pass) {
            ITexture* pDestination = pass == 0 ? m_pAccum[current] : m_pFiltered[pass % 2];


            constants.FilterStep = 1 << pass;
            PipelineUtils::WriteCB(pContext, m_pConstantsCB, constants);

            PipelineUtils::BindVariable(m_pFilterSRB, SHADER_TYPE_PIXEL, "g_GISource", SRV(pResult));
            PipelineUtils::BindVariable(m_pFilterSRB, SHADER_TYPE_PIXEL, "g_GIGuide", SRV(m_pGuide[current]));

            ITextureView* pRTV = RTV(pDestination);
            pContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            pContext->SetPipelineState(m_pFilterPSO);
            pContext->CommitShaderResources(m_pFilterSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            PipelineUtils::DrawFullscreenTriangle(pContext);

            pResult = pDestination;
        }

        m_pLastResult = pResult;
        m_pResultSRV = SRV(pResult);
        m_pGuideSRV = SRV(m_pGuide[current]);

        m_Current = current;
        m_HistoryValid = true;
        m_PrevViewProj = view.ViewProj;
        m_PrevCameraPos = view.CameraPos;
    }

}
