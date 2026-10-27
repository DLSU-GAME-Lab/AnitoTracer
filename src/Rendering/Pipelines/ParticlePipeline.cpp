#include "ParticlePipeline.hpp"
#include "../../UserSettings.hpp"
#include "../RenderData.hpp"

#include <algorithm>

namespace {
    constexpr Diligent::Uint32 kMaxParticleInstances = 1u << 20;
}

void Diligent::ParticlePipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain)
{
    m_Alpha = {};
    m_Additive = {};
    m_pCameraCB.Release();
    m_pParticleCB.Release();
    m_pInstanceBuffer.Release();
    m_InstanceCapacity = 0;
    m_pWhiteSRV.Release();

    pSwapChain = _pSwapChain;
    m_pDevice = pDevice;

    CreateCameraConstantBuffer(pDevice);

    BufferDesc CBDesc;
    CBDesc.Name = "Particle Constant Buffer";
    CBDesc.Size = sizeof(ParticleConstants);
    CBDesc.Usage = USAGE_DYNAMIC;
    CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    pDevice->CreateBuffer(CBDesc, nullptr, &m_pParticleCB);

    CreateWhiteTexture(pDevice);

    CreateBlendState(pDevice, m_Alpha, ParticleBlendMode::Alpha);
    CreateBlendState(pDevice, m_Additive, ParticleBlendMode::Additive);
}

void Diligent::ParticlePipeline::CreateWhiteTexture(IRenderDevice* pDevice)
{
    TextureDesc desc;
    desc.Name = "Particle White Texture";
    desc.Type = RESOURCE_DIM_TEX_2D;
    desc.Width = 1;
    desc.Height = 1;
    desc.Format = TEX_FORMAT_RGBA8_UNORM;
    desc.BindFlags = BIND_SHADER_RESOURCE;
    desc.Usage = USAGE_IMMUTABLE;

    Uint32 white = 0xFFFFFFFFu;
    TextureSubResData subResData{ &white, 4 };
    TextureData initData{ &subResData, 1 };

    RefCntAutoPtr<ITexture> pTexture;
    pDevice->CreateTexture(desc, &initData, &pTexture);
    if (pTexture) {
        m_pWhiteSRV = pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    }
}

void Diligent::ParticlePipeline::CreateBlendState(IRenderDevice* pDevice, BlendState& state, ParticleBlendMode mode)
{
    const bool additive = (mode == ParticleBlendMode::Additive);

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = additive ? "Particle Additive PSO" : "Particle Alpha PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    SetupDefaultGraphicsPipeline(GraphicsPipeline);
    GraphicsPipeline.SmplDesc.Count = UserSettings::GetInstance().GetEnableMSAA() ? 4 : 1;
    GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = False;

    auto& rt = GraphicsPipeline.BlendDesc.RenderTargets[0];
    rt.BlendEnable = True;
    rt.SrcBlend = BLEND_FACTOR_SRC_ALPHA;
    rt.DestBlend = additive ? BLEND_FACTOR_ONE : BLEND_FACTOR_INV_SRC_ALPHA;
    rt.BlendOp = BLEND_OPERATION_ADD;
    rt.SrcBlendAlpha = BLEND_FACTOR_ZERO;
    rt.DestBlendAlpha = BLEND_FACTOR_ONE;
    rt.BlendOpAlpha = BLEND_OPERATION_ADD;
    rt.RenderTargetWriteMask = COLOR_MASK_ALL;

    // One per-instance vertex buffer in slot 0; the quad corners are generated from SV_VertexID.
    LayoutElement layout[] = {
        LayoutElement{0, 0, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE}, // position + rotation
        LayoutElement{1, 0, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE}, // size + frame + pad
        LayoutElement{2, 0, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE}  // color
    };
    GraphicsPipeline.InputLayout.LayoutElements = layout;
    GraphicsPipeline.InputLayout.NumElements = _countof(layout);

    auto pVS = ShaderManager::GetInstance().GetShader("particle.hlsl", SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("particle.hlsl", SHADER_TYPE_PIXEL, "main_ps");
    if (!pVS || !pPS)
    {
        LOG_ERROR_AND_THROW("particle.hlsl failed to load (VS: ", pVS ? "ok" : "null",
            ", PS: ", pPS ? "ok" : "null", ")");
    }
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    ShaderResourceVariableDesc Variables[] =
    {
        {SHADER_TYPE_VERTEX, "CameraConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_VERTEX, "ParticleConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    PSODesc.ResourceLayout.Variables = Variables;
    PSODesc.ResourceLayout.NumVariables = _countof(Variables);

    SamplerDesc clampSampler;
    clampSampler.MinFilter = FILTER_TYPE_LINEAR;
    clampSampler.MagFilter = FILTER_TYPE_LINEAR;
    clampSampler.MipFilter = FILTER_TYPE_LINEAR;
    clampSampler.AddressU = TEXTURE_ADDRESS_CLAMP;
    clampSampler.AddressV = TEXTURE_ADDRESS_CLAMP;
    clampSampler.AddressW = TEXTURE_ADDRESS_CLAMP;
    ImmutableSamplerDesc ImmutableSamplers[] =
    {
        {SHADER_TYPE_PIXEL, "g_Texture_sampler", clampSampler}
    };
    PSODesc.ResourceLayout.ImmutableSamplers = ImmutableSamplers;
    PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImmutableSamplers);

    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &state.PSO);
    if (!state.PSO) return;

    state.PSO->CreateShaderResourceBinding(&state.SRB, true);

    if (auto* var = state.SRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants")) {
        var->Set(m_pCameraCB);
    }
    if (auto* var = state.SRB->GetVariableByName(SHADER_TYPE_VERTEX, "ParticleConstants")) {
        var->Set(m_pParticleCB);
    }
}

void Diligent::ParticlePipeline::EnsureInstanceBufferCapacity(Uint32 requiredInstances)
{
    if (m_pInstanceBuffer && requiredInstances <= m_InstanceCapacity) {
        return;
    }

    Uint32 newCapacity = std::max<Uint32>(requiredInstances, m_InstanceCapacity == 0 ? 1024 : m_InstanceCapacity * 2);
    newCapacity = std::min<Uint32>(newCapacity, kMaxParticleInstances);

    BufferDesc desc;
    desc.Name = "Particle Instance Buffer";
    desc.Usage = USAGE_DYNAMIC;
    desc.BindFlags = BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = CPU_ACCESS_WRITE;
    desc.Size = static_cast<Uint64>(newCapacity) * sizeof(ParticleInstance);

    m_pInstanceBuffer.Release();
    m_pDevice->CreateBuffer(desc, nullptr, &m_pInstanceBuffer);
    m_InstanceCapacity = newCapacity;
}

void Diligent::ParticlePipeline::RenderParticles(IDeviceContext* pContext, const RenderData& renderData)
{
    if (renderData.Particles.empty() || !m_pDevice || !m_pCameraCB || !m_pParticleCB) {
        return;
    }

    {
        MapHelper<CameraConstants> CBData(pContext, m_pCameraCB, MAP_WRITE, MAP_FLAG_DISCARD);
        CBData->View = glm::transpose(renderData.ViewMatrix);
        CBData->Proj = glm::transpose(renderData.ProjectionMatrix);
    }

    // View matrix rows are the camera axes expressed in world space
    const glm::mat4& view = renderData.ViewMatrix;
    const glm::vec3 camRight(view[0][0], view[1][0], view[2][0]);
    const glm::vec3 camUp(view[0][1], view[1][1], view[2][1]);
    const glm::vec3 camForward(view[0][2], view[1][2], view[2][2]);

    for (const ParticleBatch& batch : renderData.Particles) {
        if (batch.Instances.empty()) continue;

        BlendState& state = (batch.Blend == ParticleBlendMode::Additive) ? m_Additive : m_Alpha;
        if (!state.PSO || !state.SRB) continue;

        const ParticleInstance* pSrc = batch.Instances.data();
        Uint32 count = static_cast<Uint32>(std::min<size_t>(batch.Instances.size(), kMaxParticleInstances));

        // Alpha blending is order dependent; additive is commutative so it needs no sort
        if (batch.Blend == ParticleBlendMode::Alpha && count > 1) {
            m_SortScratch.assign(batch.Instances.begin(), batch.Instances.begin() + count);
            std::sort(m_SortScratch.begin(), m_SortScratch.end(),
                [&](const ParticleInstance& a, const ParticleInstance& b) {
                    return glm::dot(a.Position, camForward) > glm::dot(b.Position, camForward);
                });
            pSrc = m_SortScratch.data();
        }

        EnsureInstanceBufferCapacity(count);
        if (!m_pInstanceBuffer) continue;

        {
            MapHelper<ParticleInstance> VBData(pContext, m_pInstanceBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
            ParticleInstance* pDst = VBData;
            if (!pDst) continue;
            memcpy(pDst, pSrc, count * sizeof(ParticleInstance));
        }

        {
            MapHelper<ParticleConstants> CBData(pContext, m_pParticleCB, MAP_WRITE, MAP_FLAG_DISCARD);
            CBData->CameraRight = glm::vec4(camRight, 0.0f);
            CBData->CameraUp = glm::vec4(camUp, 0.0f);
            CBData->Grid = glm::vec4(static_cast<float>(std::max<uint32_t>(batch.Columns, 1)),
                static_cast<float>(std::max<uint32_t>(batch.Rows, 1)), 0.0f, 0.0f);
        }

        ITextureView* pTexture = batch.Texture ? batch.Texture : m_pWhiteSRV.RawPtr();
        if (auto* pTexVar = state.SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")) {
            pTexVar->Set(pTexture);
        }

        pContext->SetPipelineState(state.PSO);
        pContext->CommitShaderResources(state.SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        IBuffer* pBuffs[] = { m_pInstanceBuffer };
        pContext->SetVertexBuffers(0, 1, pBuffs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

        DrawAttribs drawAttrs;
        drawAttrs.NumVertices = 4;
        drawAttrs.NumInstances = count;
        drawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
        pContext->Draw(drawAttrs);
    }
}
