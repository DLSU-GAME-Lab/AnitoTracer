#include "ColorCorrectionPipeline.hpp"
#include "Shaders/ShaderManager.hpp"
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"

namespace Diligent {

    void ColorCorrectionPipeline::Initialize(IRenderDevice* pDevice, TEXTURE_FORMAT RTVFormat) {
        m_pDevice = pDevice;
        m_Format = RTVFormat;
        m_pIntermediate.Release();

        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
        GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

        PSODesc.Name = "Color Correction PSO";
        PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

        GraphicsPipeline.NumRenderTargets = 1;
        GraphicsPipeline.RTVFormats[0] = RTVFormat;
        GraphicsPipeline.DSVFormat = TEX_FORMAT_UNKNOWN;
        GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
        GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
        GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = False;

        auto pVS = ShaderManager::GetInstance().GetShader("Deferred/fullscreen_vs.hlsl", SHADER_TYPE_VERTEX, "main_vs");
        auto pPS = ShaderManager::GetInstance().GetShader("PostProcess/color_correction_ps.hlsl", SHADER_TYPE_PIXEL, "main_ps");
        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        ShaderResourceVariableDesc Variables[] = {
            { SHADER_TYPE_PIXEL, "g_SceneColor", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
            { SHADER_TYPE_PIXEL, "ColorCorrectionConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC }
        };
        PSODesc.ResourceLayout.Variables = Variables;
        PSODesc.ResourceLayout.NumVariables = _countof(Variables);

        SamplerDesc SamPointClampDesc;
        SamPointClampDesc.MinFilter = FILTER_TYPE_POINT;
        SamPointClampDesc.MagFilter = FILTER_TYPE_POINT;
        SamPointClampDesc.MipFilter = FILTER_TYPE_POINT;
        SamPointClampDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
        SamPointClampDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
        SamPointClampDesc.AddressW = TEXTURE_ADDRESS_CLAMP;

        ImmutableSamplerDesc ImtblSamplers[] = {
            { SHADER_TYPE_PIXEL, "g_SceneColor_sampler", SamPointClampDesc }
        };
        PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers;
        PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

        m_pPSO.Release();
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

        m_pSRB.Release();
        m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

        BufferDesc CBDesc;
        CBDesc.Name = "Color Correction Constant Buffer";
        CBDesc.Size = sizeof(ColorCorrectionConstants);
        CBDesc.Usage = USAGE_DYNAMIC;
        CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        m_pConstantsCB.Release();
        pDevice->CreateBuffer(CBDesc, nullptr, &m_pConstantsCB);

        if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "ColorCorrectionConstants")) {
            pVar->Set(m_pConstantsCB);
        }
    }

    ITextureView* ColorCorrectionPipeline::GetIntermediateRTV(Uint32 Width, Uint32 Height) {
        if (!m_pDevice || Width == 0 || Height == 0) return nullptr;

        if (!m_pIntermediate ||
            m_pIntermediate->GetDesc().Width != Width ||
            m_pIntermediate->GetDesc().Height != Height)
        {
            TextureDesc TexDesc;
            TexDesc.Name = "Color Correction Intermediate";
            TexDesc.Type = RESOURCE_DIM_TEX_2D;
            TexDesc.Width = Width;
            TexDesc.Height = Height;
            TexDesc.Format = m_Format;
            TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;

            m_pIntermediate.Release();
            m_pDevice->CreateTexture(TexDesc, nullptr, &m_pIntermediate);
        }

        return m_pIntermediate ? m_pIntermediate->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET) : nullptr;
    }

    void ColorCorrectionPipeline::Apply(IDeviceContext* pContext, ITextureView* pDstRTV, const ColorCorrectionConstants& constants) {
        if (!m_pPSO || !m_pIntermediate || !pDstRTV) return;

        {
            MapHelper<ColorCorrectionConstants> CBData(pContext, m_pConstantsCB, MAP_WRITE, MAP_FLAG_DISCARD);
            *CBData = constants;
        }

        if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_SceneColor")) {
            pVar->Set(m_pIntermediate->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        }

        pContext->SetRenderTargets(1, &pDstRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const auto& DstDesc = pDstRTV->GetTexture()->GetDesc();
        Viewport VP;
        VP.Width = static_cast<float>(DstDesc.Width);
        VP.Height = static_cast<float>(DstDesc.Height);
        pContext->SetViewports(1, &VP, DstDesc.Width, DstDesc.Height);

        pContext->SetPipelineState(m_pPSO);
        pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawAttribs DrawAttrs;
        DrawAttrs.NumVertices = 3;
        DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
        pContext->Draw(DrawAttrs);
    }

}
