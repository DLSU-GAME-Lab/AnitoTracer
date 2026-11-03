#include "DeferredHybridPipeline.hpp"
#include "../RenderData.hpp"
#include "../../UserSettings.hpp"
#include <algorithm>

namespace {
    // Mirrors cbuffer DeferredHybridConstants in Shaders/DeferredHybrid/lighting_ps.hlsl
    struct DeferredHybridConstants
    {
        glm::mat4 InvViewProj;
        glm::vec4 ViewportSize; // xy: size in pixels, zw: 1 / size
    };
    static_assert(sizeof(DeferredHybridConstants) % 16 == 0, "DeferredHybridConstants must be 16-byte aligned for HLSL");

    // G-buffer formats, must match Shaders/DeferredHybrid/gbuffer_common.hlsli
    constexpr Diligent::TEXTURE_FORMAT GBUFFER_ALBEDO_METALLIC_FORMAT = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
    constexpr Diligent::TEXTURE_FORMAT GBUFFER_NORMALS_FORMAT = Diligent::TEX_FORMAT_RGBA16_FLOAT;
    constexpr Diligent::TEXTURE_FORMAT GBUFFER_EMISSIVE_AO_FORMAT = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
    constexpr Diligent::TEXTURE_FORMAT GBUFFER_ROUGHNESS_FORMAT = Diligent::TEX_FORMAT_R8_UNORM;
    constexpr Diligent::TEXTURE_FORMAT GBUFFER_DEPTH_FORMAT = Diligent::TEX_FORMAT_D32_FLOAT;
}

void Diligent::DeferredHybridPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain)
{
    pSwapChain = _pSwapChain;
    m_pDevice = pDevice;

    if (!m_pTLAS) InitializeTLAS(pDevice);
    if (!m_pCameraCB) CreateCameraConstantBuffer(pDevice);
    if (!m_pModelCB) CreateModelConstantBuffer(pDevice);
    CreateCommonConstantBuffers(pDevice);
    if (!m_pDeferredCB) CreateDeferredConstantBuffer(pDevice);
    if (!m_pFallbackEnvMap) CreateFallbackEnvironmentMap(pDevice);

    InitializeDepthPrepassPSO(pDevice);
    InitializeGBufferPSO(pDevice);
    InitializeLightingPSO(pDevice);
}

void Diligent::DeferredHybridPipeline::CreateDeferredConstantBuffer(IRenderDevice* pDevice)
{
    BufferDesc CBDesc;
    CBDesc.Name = "Deferred Hybrid Constant Buffer";
    CBDesc.Size = sizeof(DeferredHybridConstants);
    CBDesc.Usage = USAGE_DYNAMIC;
    CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

    pDevice->CreateBuffer(CBDesc, nullptr, &m_pDeferredCB);
}

void Diligent::DeferredHybridPipeline::InitializeDepthPrepassPSO(IRenderDevice* pDevice)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Deferred Hybrid Depth Prepass PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // Depth only, into the G-buffer's own depth
    SetupDefaultGraphicsPipeline(GraphicsPipeline);
    GraphicsPipeline.NumRenderTargets = 0;
    GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_UNKNOWN;
    GraphicsPipeline.DSVFormat = GBUFFER_DEPTH_FORMAT;
    GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = True;
    GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_LESS;

    std::vector<LayoutElement> std_layout = VertexLayouts::GetStandardLayout();
    GraphicsPipeline.InputLayout.LayoutElements = std_layout.data();
    GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(std_layout.size());

    // Same vertex shader as the G-buffer pass, so both produce identical depth
    auto pVS = ShaderManager::GetInstance().GetShader("main_vs.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("DeferredHybrid/depth_prepass_ps.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    std::vector<ShaderResourceVariableDesc> Variables = GetStandardShaderVariables();
    PSODesc.ResourceLayout.Variables = Variables.data();
    PSODesc.ResourceLayout.NumVariables = static_cast<Uint32>(Variables.size());

    std::vector<ImmutableSamplerDesc> ImtblSamplers = GetStandardImmutableSamplers();
    PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers.data();
    PSODesc.ResourceLayout.NumImmutableSamplers = static_cast<Uint32>(ImtblSamplers.size());

    m_pDepthPrepassPSO.Release();
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pDepthPrepassPSO);

    m_pDepthPrepassSRB.Release();
    m_pDepthPrepassPSO->CreateShaderResourceBinding(&m_pDepthPrepassSRB, true);

    if (auto* pVar = m_pDepthPrepassSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants")) pVar->Set(m_pCameraCB);
    if (auto* pVar = m_pDepthPrepassSRB->GetVariableByName(SHADER_TYPE_VERTEX, "ModelConstants"))  pVar->Set(m_pModelCB);
    if (auto* pVar = m_pDepthPrepassSRB->GetVariableByName(SHADER_TYPE_PIXEL, "PBRMaterialConstants")) pVar->Set(m_pMaterialCB);
}

void Diligent::DeferredHybridPipeline::InitializeGBufferPSO(IRenderDevice* pDevice)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Deferred Hybrid G-Buffer PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    SetupDefaultGraphicsPipeline(GraphicsPipeline);
    GraphicsPipeline.NumRenderTargets = 4;
    GraphicsPipeline.RTVFormats[0] = GBUFFER_ALBEDO_METALLIC_FORMAT;
    GraphicsPipeline.RTVFormats[1] = GBUFFER_NORMALS_FORMAT;
    GraphicsPipeline.RTVFormats[2] = GBUFFER_EMISSIVE_AO_FORMAT;
    GraphicsPipeline.RTVFormats[3] = GBUFFER_ROUGHNESS_FORMAT;
    GraphicsPipeline.DSVFormat = GBUFFER_DEPTH_FORMAT;

    // The prepass already wrote the nearest depth: only fragments matching it pass, and nothing is rewritten.
    // LESS_EQUAL instead of EQUAL keeps this robust if the two passes' depths ever differ in the last bit.
    GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = False;
    GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_LESS_EQUAL;

    std::vector<LayoutElement> std_layout = VertexLayouts::GetStandardLayout();
    GraphicsPipeline.InputLayout.LayoutElements = std_layout.data();
    GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(std_layout.size());

    auto pVS = ShaderManager::GetInstance().GetShader("main_vs.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("DeferredHybrid/gbuffer_ps.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    std::vector<ShaderResourceVariableDesc> Variables = GetStandardShaderVariables();
    PSODesc.ResourceLayout.Variables = Variables.data();
    PSODesc.ResourceLayout.NumVariables = static_cast<Uint32>(Variables.size());

    std::vector<ImmutableSamplerDesc> ImtblSamplers = GetStandardImmutableSamplers();
    PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers.data();
    PSODesc.ResourceLayout.NumImmutableSamplers = static_cast<Uint32>(ImtblSamplers.size());

    // m_pPSO / m_pSRB so BasePipeline::RenderModels binds materials through this pass
    m_pPSO.Release();
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    m_pSRB.Release();
    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants")) pVar->Set(m_pCameraCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "ModelConstants"))  pVar->Set(m_pModelCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "PBRMaterialConstants")) pVar->Set(m_pMaterialCB);
}

void Diligent::DeferredHybridPipeline::InitializeLightingPSO(IRenderDevice* pDevice)
{
    // The lighting pass writes into the frame's target, which is multisampled when MSAA is on.
    // Shading is still per pixel (the G-buffer is single sampled), so geometry edges are not anti-aliased.
    Uint8 sampleCount = UserSettings::GetInstance().GetEnableMSAA() ? 4 : 1;

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Deferred Hybrid Lighting PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    GraphicsPipeline.NumRenderTargets = 1;
    GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;
    GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
    GraphicsPipeline.SmplDesc.Count = sampleCount;

    // The shader outputs the G-buffer depth, so later passes (particles, debug lines) are occluded by the scene
    GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = True;
    GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_ALWAYS;

    auto pVS = ShaderManager::GetInstance().GetShader("Deferred/fullscreen_vs.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("DeferredHybrid/lighting_ps.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    std::vector<ShaderResourceVariableDesc> Variables = {
        {SHADER_TYPE_PIXEL, "g_GBufferAlbedoMetallic", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_GBufferNormals", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_GBufferEmissiveAO", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_GBufferRoughness", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_GBufferDepth", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "DeferredHybridConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "LightConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "ShadowSettings", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "FogConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "g_TLAS", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        // Dynamic so the skybox can be swapped after the fallback is bound
        {SHADER_TYPE_PIXEL, "g_EnvMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    PSODesc.ResourceLayout.Variables = Variables.data();
    PSODesc.ResourceLayout.NumVariables = static_cast<Uint32>(Variables.size());

    SamplerDesc SamEnvDesc = GetLinearWrapSamplerDesc();
    SamEnvDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
    SamEnvDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
    SamEnvDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
    ImmutableSamplerDesc ImtblSamplers[] = {
        {SHADER_TYPE_PIXEL, "g_EnvMap_sampler", SamEnvDesc}
    };
    PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers;
    PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    m_pLightingPSO.Release();
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pLightingPSO);

    m_pLightingSRB.Release();
    m_pLightingPSO->CreateShaderResourceBinding(&m_pLightingSRB, true);

    if (auto* pVar = m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, "DeferredHybridConstants")) pVar->Set(m_pDeferredCB);
    if (auto* pVar = m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, "LightConstants")) pVar->Set(m_pLightCB);
    if (auto* pVar = m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, "ShadowSettings")) pVar->Set(m_pShadowCB);
    if (auto* pVar = m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, "FogConstants")) pVar->Set(m_pFogCB);

    // The TLAS is bound here, so it MUST have a valid state before rendering!
    if (auto* pVar = m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_TLAS")) pVar->Set(m_pTLAS);

    SetEnvironmentMap(m_pEnvMapSRV);

    // Re-initialization (pipeline switch, MSAA toggle) recreates the SRB, so rebind an existing G-buffer
    if (m_pGBufferDepth) BindGBufferToLightingSRB();
}

void Diligent::DeferredHybridPipeline::SetEnvironmentMap(ITextureView* pCubemapSRV)
{
    m_pEnvMapSRV = pCubemapSRV;
    if (!m_pLightingSRB || !m_pFallbackEnvMap) return;

    if (auto* pVar = m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_EnvMap")) pVar->Set(GetEnvironmentMapView());
}

void Diligent::DeferredHybridPipeline::EnsureGBufferSize(Uint32 width, Uint32 height)
{
    if (width <= m_GBufferWidth && height <= m_GBufferHeight && m_pGBufferDepth) return;

    m_GBufferWidth = (std::max)(width, m_GBufferWidth);
    m_GBufferHeight = (std::max)(height, m_GBufferHeight);

    TextureDesc TexDesc;
    TexDesc.Type = RESOURCE_DIM_TEX_2D;
    TexDesc.Width = m_GBufferWidth;
    TexDesc.Height = m_GBufferHeight;
    TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;

    auto CreateTarget = [&](const char* name, TEXTURE_FORMAT format, RefCntAutoPtr<ITexture>& pTexture) {
        pTexture.Release();
        TexDesc.Name = name;
        TexDesc.Format = format;
        m_pDevice->CreateTexture(TexDesc, nullptr, &pTexture);
    };

    CreateTarget("Deferred Hybrid G-Buffer AlbedoMetallic", GBUFFER_ALBEDO_METALLIC_FORMAT, m_pGBufferAlbedoMetallic);
    CreateTarget("Deferred Hybrid G-Buffer Normals", GBUFFER_NORMALS_FORMAT, m_pGBufferNormals);
    CreateTarget("Deferred Hybrid G-Buffer EmissiveAO", GBUFFER_EMISSIVE_AO_FORMAT, m_pGBufferEmissiveAO);
    CreateTarget("Deferred Hybrid G-Buffer Roughness", GBUFFER_ROUGHNESS_FORMAT, m_pGBufferRoughness);

    TexDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
    TexDesc.ClearValue.Format = GBUFFER_DEPTH_FORMAT;
    TexDesc.ClearValue.DepthStencil.Depth = 1.0f;
    CreateTarget("Deferred Hybrid G-Buffer Depth", GBUFFER_DEPTH_FORMAT, m_pGBufferDepth);

    BindGBufferToLightingSRB();
}

void Diligent::DeferredHybridPipeline::BindGBufferToLightingSRB()
{
    if (!m_pLightingSRB) return;

    auto Bind = [&](const char* name, ITexture* pTexture) {
        if (auto* pVar = m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, name))
            pVar->Set(pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE), SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
    };

    Bind("g_GBufferAlbedoMetallic", m_pGBufferAlbedoMetallic);
    Bind("g_GBufferNormals", m_pGBufferNormals);
    Bind("g_GBufferEmissiveAO", m_pGBufferEmissiveAO);
    Bind("g_GBufferRoughness", m_pGBufferRoughness);
    Bind("g_GBufferDepth", m_pGBufferDepth);
}

void Diligent::DeferredHybridPipeline::SetFrameTargets(ITextureView* pTargetRTV, ITextureView* pTargetDSV)
{
    m_pTargetRTV = pTargetRTV;
    m_pTargetDSV = pTargetDSV;

    const auto& TargetDesc = pTargetRTV->GetTexture()->GetDesc();
    m_ViewportWidth = TargetDesc.Width;
    m_ViewportHeight = TargetDesc.Height;

    EnsureGBufferSize(m_ViewportWidth, m_ViewportHeight);
}

void Diligent::DeferredHybridPipeline::SetGBufferViewport(IDeviceContext* pContext)
{
    // Binding the G-buffer resets the viewport to its full (possibly larger) size; draw into the frame-sized corner
    Viewport VP;
    VP.Width = static_cast<float>(m_ViewportWidth);
    VP.Height = static_cast<float>(m_ViewportHeight);
    VP.MinDepth = 0.0f;
    VP.MaxDepth = 1.0f;
    pContext->SetViewports(1, &VP, m_GBufferWidth, m_GBufferHeight);
}

void Diligent::DeferredHybridPipeline::StartFrameRender(IDeviceContext* pContext, RenderData renderData)
{
    // Camera constants + TLAS rebuild
    HybridPipeline::StartFrameRender(pContext, renderData);

    {
        MapHelper<DeferredHybridConstants> CBData(pContext, m_pDeferredCB, MAP_WRITE, MAP_FLAG_DISCARD);
        // Transposed like CameraConstants, so HLSL's mul(clipPos, g_InvViewProj) undoes mul(mul(worldPos, g_View), g_Proj)
        CBData->InvViewProj = glm::transpose(glm::inverse(renderData.ProjectionMatrix * renderData.ViewMatrix));
        CBData->ViewportSize = glm::vec4(
            static_cast<float>(m_ViewportWidth), static_cast<float>(m_ViewportHeight),
            1.0f / static_cast<float>((std::max)(m_ViewportWidth, 1u)), 1.0f / static_cast<float>((std::max)(m_ViewportHeight, 1u)));
    }

    ITextureView* pDSV = m_pGBufferDepth->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
    pContext->SetRenderTargets(0, nullptr, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void Diligent::DeferredHybridPipeline::RenderModels(IDeviceContext* pContext, RenderData renderData, bool renderOpaque)
{
    // Transparent surfaces can't live in a G-buffer (one surface per pixel); they would need a forward pass after lighting
    if (!renderOpaque) return;

    ITextureView* pDSV = m_pGBufferDepth->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);

    // 1. Depth prepass: resolve visibility before any material sampling
    pContext->SetRenderTargets(0, nullptr, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    SetGBufferViewport(pContext);
    pContext->SetPipelineState(m_pDepthPrepassPSO);
    for (size_t i = 0; i < renderData.Models.size(); i++) {
        RenderModelWithSRB(pContext, renderData.Models[i], renderOpaque, m_pDepthPrepassSRB);
    }

    // 2. G-buffer: one material write per visible pixel. Colors aren't cleared; the lighting pass skips pixels at depth 1.
    ITextureView* pRTVs[] = {
        m_pGBufferAlbedoMetallic->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET),
        m_pGBufferNormals->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET),
        m_pGBufferEmissiveAO->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET),
        m_pGBufferRoughness->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET)
    };
    pContext->SetRenderTargets(_countof(pRTVs), pRTVs, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    SetGBufferViewport(pContext);
    pContext->SetPipelineState(m_pPSO);
    BasePipeline::RenderModels(pContext, renderData, renderOpaque);
}

void Diligent::DeferredHybridPipeline::RenderLightingPass(IDeviceContext* pContext)
{
    // 3. Lighting: ray-traced shading once per visible pixel, composited over the skybox already in the target
    pContext->SetRenderTargets(1, &m_pTargetRTV, m_pTargetDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(m_pLightingPSO);
    pContext->CommitShaderResources(m_pLightingSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = 3;
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    pContext->Draw(DrawAttrs);
}
