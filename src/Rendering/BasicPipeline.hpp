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

#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Matches Vulkan/D3D depth range [0, 1]
#define GLM_FORCE_LEFT_HANDED       // Matches Diligent's default coordinate system

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Diligent {

/// <summary>
/// Basic pipeline that only colors a solid color
/// Gonna be my fallback pipeline asking for the bare min.
/// </summary>
class BasicPipeline{

	public:
		void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain);

		void StartFrameRender(IDeviceContext* pContext, CameraObj camera);

		void RenderModel(IDeviceContext* pContext, Model* model);

	private:
		//Pipeline Binding
		RefCntAutoPtr<IPipelineState> m_pPSO;
		//Camera Binding
		RefCntAutoPtr<IBuffer> m_pCameraCB;
		//Shader Binding
		RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
		//Swap Chain Ref
		RefCntAutoPtr<ISwapChain> pSwapChain;
};

}