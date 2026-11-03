#include "PSOBuilder.hpp"
#include "Shaders/ShaderManager.hpp"
#include <iostream>

namespace Diligent {

    PSOBuilder::PSOBuilder(std::string name) : m_Name(std::move(name))
    {
        m_CreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

        GraphicsPipelineDesc& GP = m_CreateInfo.GraphicsPipeline;
        GP.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        GP.RasterizerDesc.CullMode = CULL_MODE_BACK;
        GP.DepthStencilDesc.DepthEnable = True;
        GP.DepthStencilDesc.DepthWriteEnable = True;
        GP.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_LESS_EQUAL;
    }

    PSOBuilder& PSOBuilder::Shaders(const char* vsFile, const char* psFile, const char* vsEntry, const char* psEntry)
    {
        m_VSFile = vsFile;
        m_PSFile = psFile;
        m_pVS = ShaderManager::GetInstance().GetShader(vsFile, SHADER_TYPE_VERTEX, vsEntry);
        m_pPS = ShaderManager::GetInstance().GetShader(psFile, SHADER_TYPE_PIXEL, psEntry);
        return *this;
    }

    PSOBuilder& PSOBuilder::RenderTargets(std::initializer_list<TEXTURE_FORMAT> rtvFormats, TEXTURE_FORMAT dsvFormat)
    {
        return RenderTargets(std::vector<TEXTURE_FORMAT>(rtvFormats), dsvFormat);
    }

    PSOBuilder& PSOBuilder::RenderTargets(const std::vector<TEXTURE_FORMAT>& rtvFormats, TEXTURE_FORMAT dsvFormat)
    {
        GraphicsPipelineDesc& GP = m_CreateInfo.GraphicsPipeline;
        GP.NumRenderTargets = 0;
        for (TEXTURE_FORMAT format : rtvFormats) {
            if (format == TEX_FORMAT_UNKNOWN) continue;
            GP.RTVFormats[GP.NumRenderTargets++] = format;
        }
        for (Uint32 i = GP.NumRenderTargets; i < MAX_RENDER_TARGETS; ++i) {
            GP.RTVFormats[i] = TEX_FORMAT_UNKNOWN;
        }

        GP.DSVFormat = dsvFormat;
        if (dsvFormat == TEX_FORMAT_UNKNOWN) {
            Depth(false, false);
        }
        return *this;
    }

    PSOBuilder& PSOBuilder::SampleCount(Uint8 count)
    {
        m_CreateInfo.GraphicsPipeline.SmplDesc.Count = count;
        return *this;
    }

    PSOBuilder& PSOBuilder::Topology(PRIMITIVE_TOPOLOGY topology)
    {
        m_CreateInfo.GraphicsPipeline.PrimitiveTopology = topology;
        return *this;
    }

    PSOBuilder& PSOBuilder::Cull(CULL_MODE mode)
    {
        m_CreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = mode;
        return *this;
    }

    PSOBuilder& PSOBuilder::Depth(bool enable, bool write, COMPARISON_FUNCTION func)
    {
        DepthStencilStateDesc& DS = m_CreateInfo.GraphicsPipeline.DepthStencilDesc;
        DS.DepthEnable = enable;
        DS.DepthWriteEnable = enable && write;
        DS.DepthFunc = func;
        return *this;
    }

    PSOBuilder& PSOBuilder::Blend(Uint32 renderTarget, const RenderTargetBlendDesc& blend)
    {
        m_CreateInfo.GraphicsPipeline.BlendDesc.RenderTargets[renderTarget] = blend;
        return *this;
    }

    PSOBuilder& PSOBuilder::InputLayout(std::vector<LayoutElement> layout)
    {
        m_Layout = std::move(layout);
        return *this;
    }

    PSOBuilder& PSOBuilder::Variable(SHADER_TYPE shaderTypes, const char* name, SHADER_RESOURCE_VARIABLE_TYPE type)
    {
        m_Variables.push_back({ shaderTypes, name, type });
        return *this;
    }

    PSOBuilder& PSOBuilder::Variables(const std::vector<ShaderResourceVariableDesc>& variables)
    {
        m_Variables.insert(m_Variables.end(), variables.begin(), variables.end());
        return *this;
    }

    PSOBuilder& PSOBuilder::Sampler(SHADER_TYPE shaderTypes, const char* name, const SamplerDesc& sampler)
    {
        m_Samplers.push_back({ shaderTypes, name, sampler });
        return *this;
    }

    PSOBuilder& PSOBuilder::Samplers(const std::vector<ImmutableSamplerDesc>& samplers)
    {
        m_Samplers.insert(m_Samplers.end(), samplers.begin(), samplers.end());
        return *this;
    }

    RefCntAutoPtr<IPipelineState> PSOBuilder::Build(IRenderDevice* pDevice)
    {
        RefCntAutoPtr<IPipelineState> pPSO;
        if (!m_pVS || !m_pPS) {
            std::cout << "[Error] " << m_Name << ": shader failed to load (VS " << m_VSFile << ": " << (m_pVS ? "ok" : "null")
                << ", PS " << m_PSFile << ": " << (m_pPS ? "ok" : "null") << ")" << std::endl;
            return pPSO;
        }

        // Wire up the owned arrays only now, so the builder can be freely copied or returned before this point
        PipelineStateDesc& PSODesc = m_CreateInfo.PSODesc;
        PSODesc.Name = m_Name.c_str();
        PSODesc.ResourceLayout.Variables = m_Variables.data();
        PSODesc.ResourceLayout.NumVariables = static_cast<Uint32>(m_Variables.size());
        PSODesc.ResourceLayout.ImmutableSamplers = m_Samplers.data();
        PSODesc.ResourceLayout.NumImmutableSamplers = static_cast<Uint32>(m_Samplers.size());

        m_CreateInfo.GraphicsPipeline.InputLayout.LayoutElements = m_Layout.data();
        m_CreateInfo.GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(m_Layout.size());

        m_CreateInfo.pVS = m_pVS;
        m_CreateInfo.pPS = m_pPS;

        pDevice->CreateGraphicsPipelineState(m_CreateInfo, &pPSO);
        if (!pPSO) {
            std::cout << "[Error] Failed to create " << m_Name << std::endl;
        }
        return pPSO;
    }

}
