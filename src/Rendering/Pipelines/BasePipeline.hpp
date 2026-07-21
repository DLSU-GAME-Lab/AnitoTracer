#pragma once
#include <iostream>

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
#include "../RenderData.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_FORCE_LEFT_HANDED       

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Diligent {

    struct CameraConstants {
        glm::mat4 View;
        glm::mat4 Proj;
    };

    class BasePipeline {
    public:
        virtual ~BasePipeline() = default;

        virtual void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) = 0;
        virtual void StartFrameRender(IDeviceContext* pContext, RenderData renderData);

        virtual void RenderModel(IDeviceContext* pContext, const ModelRenderInstance model) = 0;
        virtual void RenderModels(IDeviceContext* pContext, RenderData renderData);

    protected:
        void CreateCameraConstantBuffer(IRenderDevice* pDevice);
        void CreateModelConstantBuffer(IRenderDevice* pDevice);
        void SetupDefaultGraphicsPipeline(GraphicsPipelineDesc& GraphicsPipeline);
        SamplerDesc GetLinearWrapSamplerDesc();

        RefCntAutoPtr<IPipelineState> m_pPSO;
        RefCntAutoPtr<IBuffer> m_pCameraCB;
        RefCntAutoPtr<IBuffer> m_pModelCB;
        RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
        RefCntAutoPtr<ISwapChain> pSwapChain;
    };

}