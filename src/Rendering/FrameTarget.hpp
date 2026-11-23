#pragma once
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/TextureView.h"

namespace Diligent {

    // Non-owning color + depth views that a pass draws into, with their size.
    // Built from a RenderTarget, the swap chain, or a pipeline's intermediate texture.
    struct FrameTarget {
        ITextureView* pRTV = nullptr;
        ITextureView* pDSV = nullptr;
        Uint32 Width = 0;
        Uint32 Height = 0;

        bool IsValid() const { return pRTV && Width > 0 && Height > 0; }

        // Binds the color and depth views and a viewport covering the whole target
        void Bind(IDeviceContext* pContext) const {
            ITextureView* pRTVs[] = { pRTV };
            pContext->SetRenderTargets(1, pRTVs, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            Viewport VP;
            VP.Width = static_cast<float>(Width);
            VP.Height = static_cast<float>(Height);
            VP.MinDepth = 0.0f;
            VP.MaxDepth = 1.0f;
            pContext->SetViewports(1, &VP, Width, Height);
        }

        void Clear(IDeviceContext* pContext, const float* clearColor) const {
            pContext->ClearRenderTarget(pRTV, clearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            if (pDSV) {
                pContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }
        }
    };

}
