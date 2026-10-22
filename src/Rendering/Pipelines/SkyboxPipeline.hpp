#pragma once

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "PipelineDefs.hpp" // Provides CameraConstants
#include "RenderData.hpp"   // Need this for the new Render signature!

namespace Diligent {

    class SkyboxPipeline {
    public:
        // We pass the RTV and DSV formats dynamically so it supports both MSAA and standard targets
        void Initialize(IRenderDevice* pDevice, TEXTURE_FORMAT RTVFormat, TEXTURE_FORMAT DSVFormat);

        // Binds the cubemap texture to the shader resource binding
        void SetCubemap(ITextureView* pCubemapSRV);

        // Dispatches the draw call, requiring the shared Camera Constant Buffer
        void Render(IDeviceContext* pContext, const RenderData& renderData);

    private:
        RefCntAutoPtr<IPipelineState> m_pPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
        RefCntAutoPtr<IShaderResourceVariable> m_pTextureVar;

        RefCntAutoPtr<IBuffer> m_pCameraCB;
    };

}