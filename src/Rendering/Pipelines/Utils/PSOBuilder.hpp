#pragma once

#include <string>
#include <vector>
#include <initializer_list>

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Common/interface/RefCntAutoPtr.hpp"

namespace Diligent {

    // Fluent builder for graphics PSOs. Owns every array the create info points to, so nothing has to outlive a local scope.
    //
    //   m_pPSO = PSOBuilder("My PSO")
    //       .Shaders("my_vs.hlsl", "my_ps.hlsl")
    //       .RenderTargets({ colorFormat }, depthFormat)
    //       .Variable(SHADER_TYPE_PIXEL, "MyConstants")
    //       .Sampler(SHADER_TYPE_PIXEL, "g_Texture_sampler", PipelineUtils::LinearClampSampler())
    //       .Build(pDevice);
    //
    // Defaults: triangle list, back-face culling, depth test + write with LESS_EQUAL, 1 sample.
    class PSOBuilder {
    public:
        explicit PSOBuilder(std::string name);

        // Loaded through ShaderManager (cached)
        PSOBuilder& Shaders(const char* vsFile, const char* psFile, const char* vsEntry = "main_vs", const char* psEntry = "main_ps");

        // TEX_FORMAT_UNKNOWN for the depth format means no depth buffer, and also disables the depth test
        PSOBuilder& RenderTargets(std::initializer_list<TEXTURE_FORMAT> rtvFormats, TEXTURE_FORMAT dsvFormat);
        PSOBuilder& RenderTargets(const std::vector<TEXTURE_FORMAT>& rtvFormats, TEXTURE_FORMAT dsvFormat);
        PSOBuilder& SampleCount(Uint8 count);
        PSOBuilder& Topology(PRIMITIVE_TOPOLOGY topology);
        PSOBuilder& Cull(CULL_MODE mode);
        PSOBuilder& Depth(bool enable, bool write = true, COMPARISON_FUNCTION func = COMPARISON_FUNC_LESS_EQUAL);
        PSOBuilder& Blend(Uint32 renderTarget, const RenderTargetBlendDesc& blend);
        PSOBuilder& InputLayout(std::vector<LayoutElement> layout);

        PSOBuilder& Variable(SHADER_TYPE shaderTypes, const char* name, SHADER_RESOURCE_VARIABLE_TYPE type = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
        PSOBuilder& Variables(const std::vector<ShaderResourceVariableDesc>& variables);
        PSOBuilder& Sampler(SHADER_TYPE shaderTypes, const char* name, const SamplerDesc& sampler);
        PSOBuilder& Samplers(const std::vector<ImmutableSamplerDesc>& samplers);

        // Direct access for settings the builder doesn't wrap
        GraphicsPipelineDesc& Graphics() { return m_CreateInfo.GraphicsPipeline; }

        // Returns nullptr (and logs) if a shader is missing or creation fails
        RefCntAutoPtr<IPipelineState> Build(IRenderDevice* pDevice);

    private:
        std::string m_Name;
        GraphicsPipelineStateCreateInfo m_CreateInfo;
        IShader* m_pVS = nullptr;
        IShader* m_pPS = nullptr;
        std::string m_VSFile;
        std::string m_PSFile;

        std::vector<LayoutElement> m_Layout;
        std::vector<ShaderResourceVariableDesc> m_Variables;
        std::vector<ImmutableSamplerDesc> m_Samplers;
    };

}
