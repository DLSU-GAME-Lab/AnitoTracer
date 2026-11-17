#include "DiffuseGIPass.hpp"
#include "PSOBuilder.hpp"
#include "PipelineUtils.hpp"
#include <algorithm>

namespace Diligent {

    namespace {
        // Light is stored in half floats: plenty for HDR, and filtered light bands visibly in anything smaller
        constexpr TEXTURE_FORMAT kLightFormat = TEX_FORMAT_RGBA16_FLOAT;

        constexpr int kMaxRaysPerPixel = 8;

        // Bits of GIConstants::Flags. Mirror the GI_FLAG_* constants in Shaders/DeferredHybrid/gi_common.hlsli
        enum GIFlags : uint32_t {
            GI_FLAG_HIT_SHADOWS = 1u << 0,
            GI_FLAG_SKY_BOUNCE = 1u << 1,
            GI_FLAG_MORE_BOUNCES = 1u << 2,
            GI_FLAG_DENOISED = 1u << 3,
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
            uint32_t Frame = 0;                 // Moves the rays, so they add up over the frames that are averaged
            float HistoryValid = 0.0f;          // 0: there is no last frame to read
            float MaxDistance = 100.0f;

            uint32_t Flags = 0;
            uint32_t RayCount = 1;              // Rays traced per GI pixel every frame
            glm::vec2 Padding{ 0.0f };
        };
        static_assert(sizeof(GIConstants) % 16 == 0, "GIConstants must be 16-byte aligned for HLSL");

        ITextureView* SRV(ITexture* pTexture) { return pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE); }
        ITextureView* RTV(ITexture* pTexture) { return pTexture->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET); }
    }

    void DiffuseGIPass::Initialize(IRenderDevice* pDevice, const TraceSetup& rayTracing)
    {
        m_pDevice = pDevice;
        m_HistoryValid = false;

        if (!m_pConstantsCB) m_pConstantsCB = PipelineUtils::CreateDynamicCB<GIConstants>(pDevice, "Diffuse GI Constant Buffer");

        // The G-buffer and last frame's GI in, this frame's rays out
        m_pTracePSO = PSOBuilder("Diffuse GI Trace PSO")
            .Shaders("Deferred/fullscreen_vs.hlsl", "DeferredHybrid/gi_trace_ps.hlsl")
            .PixelMacros(rayTracing.Macros)
            .RenderTargets({ kLightFormat }, TEX_FORMAT_UNKNOWN)
            .Cull(CULL_MODE_NONE)
            .Variable(SHADER_TYPE_PIXEL, "g_GBufferNormals", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
            .Variable(SHADER_TYPE_PIXEL, "g_GBufferDepth", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
            .Variable(SHADER_TYPE_PIXEL, "g_GIPrevResult")
            .Variable(SHADER_TYPE_PIXEL, "g_GIPrevGuide")
            .Variable(SHADER_TYPE_PIXEL, "GIConstants")
            .Variable(SHADER_TYPE_PIXEL, "LightConstants")
            .Variable(SHADER_TYPE_PIXEL, "ShadowSettings")
            .Variables(rayTracing.Variables)
            .Samplers(rayTracing.Samplers)
            .Build(pDevice);
        m_pTraceSRB = PipelineUtils::CreateSRB(m_pTracePSO);
        PipelineUtils::BindVariable(m_pTraceSRB, SHADER_TYPE_PIXEL, "GIConstants", m_pConstantsCB);
    }

    void DiffuseGIPass::CreateTextures(Uint32 width, Uint32 height)
    {
        m_Width = width;
        m_Height = height;
        m_HistoryValid = false;

        const char* names[] = { "Diffuse GI Rays 0", "Diffuse GI Rays 1" };
        for (Uint32 i = 0; i < 2; ++i) {
            TextureDesc Desc;
            Desc.Name = names[i];
            Desc.Type = RESOURCE_DIM_TEX_2D;
            Desc.Width = width;
            Desc.Height = height;
            Desc.Format = kLightFormat;
            Desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;

            m_pRays[i].Release();
            m_pDevice->CreateTexture(Desc, nullptr, &m_pRays[i]);
        }
    }

    ITextureView* DiffuseGIPass::Trace(IDeviceContext* pContext, const View& view, const ProjectGISettings& settings)
    {
        if (!IsInitialized() || view.Width == 0 || view.Height == 0) return nullptr;

        const Uint32 scale = GetScale(settings);
        const Uint32 width = (view.Width + scale - 1) / scale;
        const Uint32 height = (view.Height + scale - 1) / scale;
        if (width != m_Width || height != m_Height || !m_pRays[0] || !m_pRays[1]) {
            CreateTextures(width, height);
        }
        if (!m_pRays[0] || !m_pRays[1]) return nullptr;

        const Uint32 previous = m_Current;
        const Uint32 current = 1 - m_Current;
        const bool historyValid = m_HistoryValid && view.pPrevGuide;

        GIConstants constants;
        constants.InvViewProj = glm::transpose(glm::inverse(view.ViewProj));
        constants.PrevViewProj = glm::transpose(historyValid ? m_PrevViewProj : view.ViewProj);
        constants.CameraPos = glm::vec4(view.CameraPos, 1.0f);
        constants.PrevCameraPos = glm::vec4(historyValid ? m_PrevCameraPos : view.CameraPos, 1.0f);
        constants.ViewportSize = glm::vec4(view.Width, view.Height, 1.0f / static_cast<float>(view.Width), 1.0f / static_cast<float>(view.Height));
        constants.Size = glm::vec4(width, height, 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height));
        constants.Scale = scale;
        // Averaged by a denoiser, the rays change every frame. Shown as traced, they only move when TAA averages them.
        constants.Frame = view.Denoised ? m_FrameIndex++ : view.NoiseFrame;
        constants.HistoryValid = historyValid ? 1.0f : 0.0f;
        constants.MaxDistance = (std::max)(settings.max_distance, 0.01f);
        if (settings.hit_shadows) constants.Flags |= GI_FLAG_HIT_SHADOWS;
        if (settings.sky_bounce) constants.Flags |= GI_FLAG_SKY_BOUNCE;
        if (settings.more_bounces) constants.Flags |= GI_FLAG_MORE_BOUNCES;
        if (view.Denoised) constants.Flags |= GI_FLAG_DENOISED;
        constants.RayCount = static_cast<uint32_t>(glm::clamp(settings.rays_per_pixel, 1, kMaxRaysPerPixel));
        PipelineUtils::WriteCB(pContext, m_pConstantsCB, constants);

        // Last frame's GI is an input: what a ray hits is lit by the GI gathered there before. It reads the denoised
        // result that was displayed when there is one: whatever noise goes in comes back out with every bounce.
        // (Nothing reads either texture while the history is invalid, but both variables need one bound.)
        ITextureView* pPrevResult = (historyValid && view.pPrevResult) ? view.pPrevResult : SRV(m_pRays[previous]);
        ITextureView* pPrevGuide = view.pPrevGuide ? view.pPrevGuide : SRV(m_pRays[previous]);
        PipelineUtils::BindVariable(m_pTraceSRB, SHADER_TYPE_PIXEL, "g_GIPrevResult", pPrevResult);
        PipelineUtils::BindVariable(m_pTraceSRB, SHADER_TYPE_PIXEL, "g_GIPrevGuide", pPrevGuide);

        ITextureView* pRTV = RTV(m_pRays[current]);
        pContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->SetPipelineState(m_pTracePSO);
        pContext->CommitShaderResources(m_pTraceSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        PipelineUtils::DrawFullscreenTriangle(pContext);

        m_Current = current;
        m_HistoryValid = true;
        m_PrevViewProj = view.ViewProj;
        m_PrevCameraPos = view.CameraPos;
        return SRV(m_pRays[current]);
    }

}
