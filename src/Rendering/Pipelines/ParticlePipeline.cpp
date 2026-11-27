#include "ParticlePipeline.hpp"
#include "../RenderData.hpp"

#include <algorithm>

void Diligent::ParticlePipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    m_Alpha = {};
    m_Additive = {};
    m_Instances.Release();

    SetDevice(pDevice, pSwapChain);
    CreateCameraConstantBuffer();
    if (!m_pParticleCB) m_pParticleCB = PipelineUtils::CreateDynamicCB<ParticleConstants>(pDevice, "Particle Constant Buffer");

    if (!m_pWhiteSRV) {
        if (auto pTexture = PipelineUtils::CreateSolidColorTexture(pDevice, "Particle White Texture", 0xFFFFFFFFu)) {
            m_pWhiteSRV = pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        }
    }

    m_Alpha = CreateBlendPass(ParticleBlendMode::Alpha);
    m_Additive = CreateBlendPass(ParticleBlendMode::Additive);
}

Diligent::PipelinePass Diligent::ParticlePipeline::CreateBlendPass(ParticleBlendMode mode)
{
    const bool additive = (mode == ParticleBlendMode::Additive);
    const RenderTargetBlendDesc blend = PipelineUtils::ColorBlend(BLEND_FACTOR_SRC_ALPHA, additive ? BLEND_FACTOR_ONE : BLEND_FACTOR_INV_SRC_ALPHA);

    // One per-instance vertex buffer in slot 0; the quad corners are generated from SV_VertexID.
    std::vector<LayoutElement> layout = {
        LayoutElement{0, 0, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE}, // position + rotation
        LayoutElement{1, 0, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE}, // size + frame + pad
        LayoutElement{2, 0, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE}  // color
    };

    PipelinePass pass = BuildPass(PSOBuilder(additive ? "Particle Additive PSO" : "Particle Alpha PSO")
        .Shaders("particle.hlsl", "particle.hlsl")
        .RenderTargets({ GetColorFormat() }, GetDepthFormat())
        .SampleCount(PipelineUtils::GetMSAASampleCount())
        .Topology(PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)
        .Cull(CULL_MODE_NONE)
        .Depth(true, false)
        .Blend(0, blend)
        .InputLayout(std::move(layout))
        .Variable(SHADER_TYPE_VERTEX, "CameraConstants")
        .Variable(SHADER_TYPE_VERTEX, "ParticleConstants")
        .Variable(SHADER_TYPE_PIXEL, "g_Texture")
        .Sampler(SHADER_TYPE_PIXEL, "g_Texture_sampler", PipelineUtils::LinearClampSampler()));

    PipelineUtils::BindVariable(pass.pSRB, SHADER_TYPE_VERTEX, "ParticleConstants", m_pParticleCB);
    return pass;
}

void Diligent::ParticlePipeline::RenderParticles(IDeviceContext* pContext, const RenderData& renderData)
{
    if (renderData.Particles.empty() || !m_pDevice || !m_pCameraCB || !m_pParticleCB) {
        return;
    }

    UpdateCameraConstants(pContext, renderData);

    // View matrix rows are the camera axes expressed in world space
    const glm::mat4& view = renderData.ViewMatrix;
    const glm::vec3 camRight(view[0][0], view[1][0], view[2][0]);
    const glm::vec3 camUp(view[0][1], view[1][1], view[2][1]);
    const glm::vec3 camForward(view[0][2], view[1][2], view[2][2]);

    for (const ParticleBatch& batch : renderData.Particles) {
        if (batch.Instances.empty()) continue;

        const PipelinePass& pass = (batch.Blend == ParticleBlendMode::Additive) ? m_Additive : m_Alpha;
        if (!pass) continue;

        const ParticleInstance* pSrc = batch.Instances.data();
        Uint32 count = static_cast<Uint32>(std::min<size_t>(batch.Instances.size(), kMaxInstances));

        // Alpha blending is order dependent; additive is commutative so it needs no sort
        if (batch.Blend == ParticleBlendMode::Alpha && count > 1) {
            m_SortScratch.assign(batch.Instances.begin(), batch.Instances.begin() + count);
            std::sort(m_SortScratch.begin(), m_SortScratch.end(),
                [&](const ParticleInstance& a, const ParticleInstance& b) {
                    return glm::dot(a.Position, camForward) > glm::dot(b.Position, camForward);
                });
            pSrc = m_SortScratch.data();
        }

        if (!m_Instances.Upload(m_pDevice, pContext, pSrc, count)) continue;

        ParticleConstants particleConstants;
        particleConstants.CameraRight = glm::vec4(camRight, 0.0f);
        particleConstants.CameraUp = glm::vec4(camUp, 0.0f);
        particleConstants.Grid = glm::vec4(static_cast<float>(std::max<uint32_t>(batch.Columns, 1)),
            static_cast<float>(std::max<uint32_t>(batch.Rows, 1)), 0.0f, 0.0f);
        PipelineUtils::WriteCB(pContext, m_pParticleCB, particleConstants);

        ITextureView* pTexture = batch.Texture ? batch.Texture : m_pWhiteSRV.RawPtr();
        PipelineUtils::BindVariable(pass.pSRB, SHADER_TYPE_PIXEL, "g_Texture", pTexture);

        pContext->SetPipelineState(pass.pPSO);
        pContext->CommitShaderResources(pass.pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_Instances.Bind(pContext);

        DrawAttribs drawAttrs;
        drawAttrs.NumVertices = 4;
        drawAttrs.NumInstances = count;
        drawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
        pContext->Draw(drawAttrs);
    }
}
