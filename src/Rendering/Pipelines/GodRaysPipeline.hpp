#pragma once

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "PipelineDefs.hpp"

namespace Diligent {

    // Full-screen screen-space light shafts from the main directional light, for the game output.
    // The scene is rendered into this pipeline's intermediate texture, then the shafts are added on top into the destination.
    class GodRaysPipeline {
    public:
        void Initialize(IRenderDevice* pDevice, TEXTURE_FORMAT RTVFormat);

        // Returns an intermediate render target matching the final target, (re)creating it on resize
        ITextureView* GetIntermediateRTV(Uint32 Width, Uint32 Height);

        // Samples the intermediate texture, adds the light shafts and writes the result to pDstRTV
        void Apply(IDeviceContext* pContext, ITextureView* pDstRTV, const GodRaysConstants& constants);

    private:
        RefCntAutoPtr<IRenderDevice> m_pDevice;
        RefCntAutoPtr<IPipelineState> m_pPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
        RefCntAutoPtr<IBuffer> m_pConstantsCB;

        RefCntAutoPtr<ITexture> m_pIntermediate;
        TEXTURE_FORMAT m_Format = TEX_FORMAT_UNKNOWN;
    };

}
