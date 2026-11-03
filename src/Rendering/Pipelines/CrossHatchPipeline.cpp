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

    void CrossHatchPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain)
    {
        SetDevice(pDevice, pSwapChain);
        CreateCommonConstantBuffers();
        if (!m_pCrossHatchCB) m_pCrossHatchCB = PipelineUtils::CreateDynamicCB<CrossHatchConstants>(m_pDevice, "CrossHatch Constant Buffer");
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

        m_pPSO = builder.Build(pDevice);
        m_pSRB = PipelineUtils::CreateSRB(m_pPSO);
        BindCommonBuffers(m_pSRB);
        PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, "g_HatchTex", m_pHatchTexture);

        // The TLAS is bound here, so it MUST have a valid state before rendering!
        if (m_RayTracedShadows) PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, "g_TLAS", m_SceneTLAS.Get());

        InitializeOutlinePSO();

        // New SRB, so the noise texture is rebound on the next frame
        m_BoundNoiseTexture = 0;
    }

    void CrossHatchPipeline::InitializeOutlinePSO()
    {
        m_pOutlinePSO = CreateModelPSOBuilder("CrossHatch Outline PSO", "CrossHatch/outline_ps.hlsl", "CrossHatch/outline_vs.hlsl")
            .SampleCount(PipelineUtils::GetMSAASampleCount())
            .Cull(CULL_MODE_FRONT)
            .Variables(GetCrossHatchVariables())
            .Build(m_pDevice);
        m_pOutlineSRB = PipelineUtils::CreateSRB(m_pOutlinePSO);
        BindCommonBuffers(m_pOutlineSRB);
    }

    std::vector<ShaderResourceVariableDesc> CrossHatchPipeline::GetCrossHatchVariables()
    {
        return {
            { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "CrossHatchConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
            // Dynamic so the noise texture can be switched from the project settings
            { SHADER_TYPE_PIXEL, "g_NoiseTex", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
            { SHADER_TYPE_PIXEL, "g_HatchTex", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC }
        };
    }

    void CrossHatchPipeline::BindCommonBuffers(IShaderResourceBinding* pSRB)
    {
        ScenePipeline::BindCommonBuffers(pSRB);
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_VERTEX, "CrossHatchConstants", m_pCrossHatchCB);
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "CrossHatchConstants", m_pCrossHatchCB);
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
        BasePipeline::StartFrameRender(pContext, renderData);
        m_CameraPos = renderData.Lights.CameraPos;
        if (m_RayTracedShadows) m_SceneTLAS.Build(pContext, renderData);
    }

    void CrossHatchPipeline::UpdateProjectSettings(IDeviceContext* pContext, const ProjectSettings& settings)
    {
        const ProjectCrossHatchSettings& style = settings.crosshatch;

        CrossHatchConstants constants = BuildCrossHatchConstants(style);
        constants.CameraPos = m_CameraPos;
        PipelineUtils::WriteCB(pContext, m_pCrossHatchCB, constants);

        m_OutlineEnabled = style.outline_enabled && style.outline_width > 0.0f;
        BindNoiseTexture(style.noise_texture);
    }

    void CrossHatchPipeline::RenderModels(IDeviceContext* pContext, const RenderData& renderData, bool renderOpaque)
    {
        // StartFrameRender already set the hatch PSO
        ScenePipeline::RenderModels(pContext, renderData, renderOpaque);

        if (!renderOpaque || !m_OutlineEnabled || !m_pOutlinePSO) return;

        pContext->SetPipelineState(m_pOutlinePSO);
        for (const ModelRenderInstance& model : renderData.Models) {
            RenderModelWithSRB(pContext, model, renderOpaque, m_pOutlineSRB);
        }
    }

}
