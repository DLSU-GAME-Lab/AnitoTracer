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

#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Matches Vulkan/D3D depth range [0, 1]
#define GLM_FORCE_LEFT_HANDED       // Matches Diligent's default coordinate system

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Diligent {

    class BasePipeline {
    public:
        virtual ~BasePipeline() = default;

        virtual void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) = 0;

        // Virtual so BasicPipeline can override it to commit resources
        virtual void StartFrameRender(IDeviceContext* pContext, CameraObj camera);

        virtual void RenderModel(IDeviceContext* pContext, Model* model) = 0;

    protected:
        void CreateCameraConstantBuffer(IRenderDevice* pDevice);
        void SetupDefaultGraphicsPipeline(GraphicsPipelineDesc& GraphicsPipeline);
        SamplerDesc GetLinearWrapSamplerDesc();

        RefCntAutoPtr<IPipelineState> m_pPSO;
        RefCntAutoPtr<IBuffer> m_pCameraCB;
        RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
        RefCntAutoPtr<ISwapChain> pSwapChain;
    };

}