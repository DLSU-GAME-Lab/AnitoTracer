#pragma once
#include <iostream>
#include <vector>

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "Common/interface/BasicMath.hpp"
#include "Shaders/ShaderManager.hpp"
#include "Models/ModelManager.hpp"
#include "Shaders/VertexLayouts.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Data/PipelineDefs.hpp"
#include "Utils/PipelineUtils.hpp"
#include "Utils/PSOBuilder.hpp"
#include "../RenderData.hpp"
#include "../../Asset/ProjectSettings.hpp"

namespace Diligent {

    class RenderData;
    struct ModelRenderInstance;

    // Common base for every pipeline that draws with the camera: owns the device, the main PSO/SRB and the
    // camera/model/material constant buffers, and knows how to draw a model's submeshes with their PBR materials.
    // Lit scene pipelines derive from ScenePipeline instead.
    class BasePipeline {
    public:
        virtual ~BasePipeline() = default;

        virtual void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) = 0;

        // Uploads the camera and sets m_pPSO
        virtual void StartFrameRender(IDeviceContext* pContext, const RenderData& renderData);

        virtual void RenderModel(IDeviceContext* pContext, const ModelRenderInstance& model, bool renderOpaque = true);
        virtual void RenderModels(IDeviceContext* pContext, const RenderData& renderData, bool renderOpaque = true);

    protected:
        void SetDevice(IRenderDevice* pDevice, ISwapChain* pSwapChain);

        // Each only creates its buffer if it doesn't exist yet, so they are safe to call on re-initialization
        void CreateCameraConstantBuffer();
        void CreateModelConstantBuffer();
        void CreateMaterialConstantBuffer();

        void UpdateCameraConstants(IDeviceContext* pContext, const RenderData& renderData);

        // Binds the constant buffers this pipeline owns to the variables pSRB declares
        virtual void BindCommonBuffers(IShaderResourceBinding* pSRB);

        // Draws a model's submeshes binding materials through pSRB instead of m_pSRB (e.g. for an extra pass with its own PSO)
        void RenderModelWithSRB(IDeviceContext* pContext, const ModelRenderInstance& modelData, bool renderOpaque, IShaderResourceBinding* pSRB);

        // PSO for drawing models with the standard vertex layout, constants and material textures, into the swap chain formats
        PSOBuilder CreateModelPSOBuilder(const char* name, const char* psFile, const char* vsFile = "main_vs.hlsl");

        virtual std::vector<ShaderResourceVariableDesc> GetStandardShaderVariables();
        virtual std::vector<ImmutableSamplerDesc> GetStandardImmutableSamplers();

        TEXTURE_FORMAT GetColorFormat() const { return m_pSwapChain->GetDesc().ColorBufferFormat; }
        TEXTURE_FORMAT GetDepthFormat() const { return m_pSwapChain->GetDesc().DepthBufferFormat; }

        RefCntAutoPtr<IPipelineState> m_pPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pSRB;

        RefCntAutoPtr<IBuffer> m_pCameraCB;
        RefCntAutoPtr<IBuffer> m_pModelCB;
        RefCntAutoPtr<IBuffer> m_pMaterialCB;

        RefCntAutoPtr<ISwapChain> m_pSwapChain;
        RefCntAutoPtr<IRenderDevice> m_pDevice;
    };

}
