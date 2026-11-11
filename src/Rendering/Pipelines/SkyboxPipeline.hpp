#pragma once

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "Data/PipelineDefs.hpp" // Provides CameraConstants
#include "RenderData.hpp"   // Need this for the new Render signature!

namespace Diligent {

    // Draws the sky cubemap built by SkyEnvironment behind the scene, wherever no geometry is in front of it
    class SkyboxPipeline {
    public:
        // Formats of the targets it draws into; the sample count follows the MSAA setting
        void Initialize(IRenderDevice* pDevice, TEXTURE_FORMAT RTVFormat, TEXTURE_FORMAT DSVFormat);

        // Binds the cubemap texture to the shader resource binding
        void SetCubemap(ITextureView* pCubemapSRV);

        // Uploads fog parameters used to fog the sky
        void UpdateFogConstants(IDeviceContext* pContext, const FogConstants& fog);

        // sky carries the rotation and brightness from the project's sky settings (BuildSkyboxConstants)
        void Render(IDeviceContext* pContext, const RenderData& renderData, const SkyboxConstants& sky);

    private:
        RefCntAutoPtr<IPipelineState> m_pPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
        RefCntAutoPtr<IShaderResourceVariable> m_pTextureVar;

        RefCntAutoPtr<IBuffer> m_pCameraCB;
        RefCntAutoPtr<IBuffer> m_pLightCB; // Lights scatter into sky fog
        RefCntAutoPtr<IBuffer> m_pFogCB;
        RefCntAutoPtr<IBuffer> m_pSkyCB;
    };

}