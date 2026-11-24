#include "CrossHatchPipeline.hpp"
#include "../RenderData.hpp"
#include "../SceneConstants.hpp"

#include <algorithm>
#include <string>

namespace Diligent {

    namespace {
        constexpr int kNoiseTextureCount = 11;
        constexpr const char* kNoiseTextureDir = "Assets/CrossHatch/Noise/";
        constexpr const char* kHatchTexturePath = "Assets/CrossHatch/hatch_strokes.png";
    }

    void CrossHatchPipeline::CreatePasses()
    {
        if (!m_pStyleCB) m_pStyleCB = PipelineUtils::CreateDynamicCB<CrossHatchConstants>(m_pDevice, "CrossHatch Constant Buffer");
        if (m_RayTracedShadows && !m_SceneTLAS.IsInitialized()) m_SceneTLAS.Initialize(m_pDevice);
        LoadHatchTexture();

        PSOBuilder builder = m_RayTracedShadows
            ? CreateModelPSOBuilder("CrossHatch RT Shadows PSO", "CrossHatch/crosshatch_rt_ps.hlsl")
            : CreateModelPSOBuilder("CrossHatch PSO", "CrossHatch/crosshatch_ps.hlsl");
        builder.SampleCount(PipelineUtils::GetMSAASampleCount())
            .Variables(GetCrossHatchVariables())
            .Sampler(SHADER_TYPE_PIXEL, "g_NoiseTex_sampler", PipelineUtils::LinearWrapSampler())
            .Sampler(SHADER_TYPE_PIXEL, "g_HatchTex_sampler", PipelineUtils::LinearWrapSampler());
        if (m_RayTracedShadows) builder.Variable(SHADER_TYPE_PIXEL, "g_TLAS");

        BuildMainPass(builder);
        PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, "g_HatchTex", m_pHatchTexture);

        // The TLAS is bound here, so it MUST have a valid state before rendering!
        if (m_RayTracedShadows) PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, "g_TLAS", m_SceneTLAS.Get());

        BuildOutlinePass(CreateModelPSOBuilder("CrossHatch Outline PSO", "CrossHatch/outline_ps.hlsl", "CrossHatch/outline_vs.hlsl")
            .Variables(GetCrossHatchVariables()));

        // New SRB, so the noise texture is rebound on the next frame
        m_BoundNoiseTexture = 0;
    }

    std::vector<ShaderResourceVariableDesc> CrossHatchPipeline::GetCrossHatchVariables()
    {
        return {
            // Dynamic so the noise texture can be switched from the project settings
            { SHADER_TYPE_PIXEL, "g_NoiseTex", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
            { SHADER_TYPE_PIXEL, "g_HatchTex", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC }
        };
    }

    void CrossHatchPipeline::LoadHatchTexture()
    {
        if (m_pHatchTexture) return;

        if (!m_pFallbackTexture) m_pFallbackTexture = PipelineUtils::CreateSolidColorTexture(m_pDevice, "CrossHatch Fallback Texture", 0xFFFFFFFF);

        m_pHatchTexture = ModelManager::GetInstance().LoadTexture(kHatchTexturePath);
        if (!m_pHatchTexture) {
            std::cout << "[Warn] CrossHatch stroke texture missing (" << kHatchTexturePath << "), toon texture hatching will be blank." << std::endl;
            m_pHatchTexture = m_pFallbackTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        }
    }

    void CrossHatchPipeline::BindNoiseTexture(int index)
    {
        index = std::clamp(index, 1, kNoiseTextureCount);
        if (index == m_BoundNoiseTexture) return;

        const std::string path = kNoiseTextureDir + std::string("noise") + std::to_string(index) + ".dds";
        ITextureView* pNoise = ModelManager::GetInstance().LoadTexture(path);
        if (!pNoise) {
            std::cout << "[Warn] CrossHatch noise texture missing (" << path << "), noise hatching will be flat." << std::endl;
            pNoise = m_pFallbackTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        }

        PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, "g_NoiseTex", pNoise);
        m_BoundNoiseTexture = index;
    }

    void CrossHatchPipeline::StartFrameRender(IDeviceContext* pContext, const RenderData& renderData)
    {
        StylizedPipeline::StartFrameRender(pContext, renderData);
        if (m_RayTracedShadows) m_SceneTLAS.Build(pContext, renderData);
    }

    void CrossHatchPipeline::UpdateProjectSettings(IDeviceContext* pContext, const ProjectSettings& settings)
    {
        const ProjectCrossHatchSettings& style = settings.crosshatch;

        CrossHatchConstants constants = BuildCrossHatchConstants(style);
        constants.CameraPos = m_CameraPos;
        PipelineUtils::WriteCB(pContext, m_pStyleCB, constants);

        m_OutlineEnabled = style.outline_enabled && style.outline_width > 0.0f;
        BindNoiseTexture(style.noise_texture);
    }

}
