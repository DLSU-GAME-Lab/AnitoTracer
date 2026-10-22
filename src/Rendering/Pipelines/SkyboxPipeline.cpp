#include "SkyboxPipeline.hpp"
#include "Shaders/ShaderManager.hpp" // Assuming this handles shader loading[cite: 2]
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"

namespace Diligent {

    void SkyboxPipeline::Initialize(IRenderDevice* pDevice, TEXTURE_FORMAT RTVFormat, TEXTURE_FORMAT DSVFormat) {
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
        GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = "Skybox PSO";
        PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

        // Route to the current formats provided by RendererManager
        GraphicsPipeline.NumRenderTargets = 1;
        GraphicsPipeline.RTVFormats[0] = RTVFormat;
        GraphicsPipeline.DSVFormat = DSVFormat;
        GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // We render the inside of the cube, so we cull front faces (or disable culling)
        GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_FRONT;

        // Depth settings: Read-only, draw if depth is <= 1.0f (background)
        GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
        GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = False;
        GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_LESS_EQUAL;

        // Load Shaders (Master's shader paths go here! 🎀)
        auto pVS = ShaderManager::GetInstance().GetShader("Skybox/Skybox_vs.hlsl", SHADER_TYPE_VERTEX, "main_vs");
        auto pPS = ShaderManager::GetInstance().GetShader("Skybox/Skybox_ps.hlsl", SHADER_TYPE_PIXEL, "main_ps");
        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        // Define Shader Variables
        ShaderResourceVariableDesc Variables[] = {
            { SHADER_TYPE_VERTEX, "CameraConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
            { SHADER_TYPE_PIXEL, "g_Cubemap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE }
        };
        PSODesc.ResourceLayout.Variables = Variables;
        PSODesc.ResourceLayout.NumVariables = _countof(Variables);

        SamplerDesc SamLinearWrapDesc;
        SamLinearWrapDesc.MinFilter = FILTER_TYPE_LINEAR;
        SamLinearWrapDesc.MagFilter = FILTER_TYPE_LINEAR;
        SamLinearWrapDesc.MipFilter = FILTER_TYPE_LINEAR;
        SamLinearWrapDesc.AddressU = TEXTURE_ADDRESS_WRAP;
        SamLinearWrapDesc.AddressV = TEXTURE_ADDRESS_WRAP;
        SamLinearWrapDesc.AddressW = TEXTURE_ADDRESS_WRAP;

        ImmutableSamplerDesc ImtblSamplers[] = {
            { SHADER_TYPE_PIXEL, "g_Cubemap_sampler", SamLinearWrapDesc }
        };
        PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers;
        PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

        m_pPSO.Release();
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

        m_pSRB.Release();
        m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

        m_pTextureVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Cubemap");

        BufferDesc CBDesc;
        CBDesc.Name = "Skybox Camera Constant Buffer";
        CBDesc.Size = sizeof(CameraConstants);
        CBDesc.Usage = USAGE_DYNAMIC;
        CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        pDevice->CreateBuffer(CBDesc, nullptr, &m_pCameraCB);

        // Bind the buffer to the shader variable permanently
        if (auto* pCameraVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants")) {
            pCameraVar->Set(m_pCameraCB);
        }
    }

    void SkyboxPipeline::SetCubemap(ITextureView* pCubemapSRV) {
        if (m_pTextureVar) {
            m_pTextureVar->Set(pCubemapSRV);
        }
    }

    void SkyboxPipeline::Render(IDeviceContext* pContext, const RenderData& renderData) {
        if (!m_pPSO || !m_pSRB) return;

        // Map the view/proj matrices dynamically every frame!
        {
            MapHelper<CameraConstants> CBData(pContext, m_pCameraCB, MAP_WRITE, MAP_FLAG_DISCARD);
            CBData->View = glm::transpose(renderData.ViewMatrix);
            CBData->Proj = glm::transpose(renderData.ProjectionMatrix);
        }

        pContext->SetPipelineState(m_pPSO);
        pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawAttribs DrawAttrs;
        DrawAttrs.NumVertices = 36;
        DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
        pContext->Draw(DrawAttrs);
    }
}