#pragma once

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "Common/interface/BasicMath.hpp"
#include "../Objects/CameraObj.hpp"
#include "Shaders/ShaderManager.hpp"
#include "../Objects/Models/ModelManager.hpp"
#include "Shaders/VertexLayouts.hpp"


#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_FORCE_LEFT_HANDED    


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Diligent {

    class TexturedPipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain);
        void StartFrameRender(IDeviceContext* pContext, CameraObj camera);
        void RenderModel(IDeviceContext* pContext, Model* model);

    private:
        RefCntAutoPtr<IPipelineState> m_pPSO;
        RefCntAutoPtr<IBuffer> m_pCameraCB;
        RefCntAutoPtr<IBuffer> m_pMaterialCB;
        RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
        RefCntAutoPtr<ISwapChain> pSwapChain;
    };

}