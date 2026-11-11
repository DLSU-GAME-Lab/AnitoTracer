#pragma once

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "../../RenderData.hpp"
#include "../Data/PipelineDefs.hpp"
#include "../../../UserSettings.hpp"

namespace Diligent {

    // Temporal anti-aliasing for one view (Shaders/PostProcess/taa_ps.hlsl). It keeps that view's history, so every view needs its own.
    // Runs after the scene, overlays and MSAA resolve, and before the PostProcessStack:
    //   1. Begin   - picks this frame's jitter and returns the texture to draw the scene into, with GetJitteredProjection
    //   2. Resolve - blends that texture with the reprojected history and writes the anti-aliased image into the next stage
    class TemporalAAPass {
    public:
        void Initialize(IRenderDevice* pDevice, TEXTURE_FORMAT RTVFormat);

        // Drops the accumulated history, so the next frame starts over (e.g. when TAA was off for this view)
        void ResetHistory() { m_HistoryValid = false; }

        // Returns where the scene should be drawn this frame, or nullptr if the textures couldn't be created
        ITextureView* Begin(const TAASettings& settings, const glm::mat4& view, const glm::mat4& proj, Uint32 width, Uint32 height);

        // proj from Begin with this frame's sub-pixel offset; every scene pass should draw with it
        const glm::mat4& GetJitteredProjection() const { return m_JitteredProj; }

        // Frames this pass has begun, 1 or more after the first Begin; seeds RenderData::NoiseFrame
        Uint32 GetFrameIndex() const { return m_FrameIndex; }

        // pDepthSRV is the scene depth to reproject with. Without it every pixel is reprojected as if it were at the far plane,
        // which handles camera rotation but ghosts more when the camera moves.
        void Resolve(IDeviceContext* pContext, ITextureView* pDepthSRV, ITextureView* pDstRTV);

    private:
        void CreateTextures(Uint32 width, Uint32 height);

        RefCntAutoPtr<IRenderDevice> m_pDevice;
        RefCntAutoPtr<IPipelineState> m_pPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
        RefCntAutoPtr<IBuffer> m_pConstantsCB;
        RefCntAutoPtr<ITexture> m_pFallbackDepth;
        TEXTURE_FORMAT m_Format = TEX_FORMAT_UNKNOWN;

        RefCntAutoPtr<ITexture> m_pInput;       // This frame's jittered scene
        RefCntAutoPtr<ITexture> m_pHistory[2];  // Ping-ponged: one read as last frame's result while the other is written
        Uint32 m_HistoryIndex = 0;              // m_pHistory entry holding last frame's result
        bool m_HistoryValid = false;
        Uint32 m_Width = 0;
        Uint32 m_Height = 0;

        Uint32 m_FrameIndex = 0;
        glm::mat4 m_JitteredProj{ 1.0f };
        glm::mat4 m_ViewProj{ 1.0f };
        glm::mat4 m_PrevViewProj{ 1.0f };
        TemporalAAConstants m_Constants;
    };

}
