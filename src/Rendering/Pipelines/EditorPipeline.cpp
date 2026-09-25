#include "EditorPipeline.hpp"

void Diligent::EditorPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    // Initialize the base deferred pipeline components
    DeferredPipeline::InitializePipeline(pDevice, pSwapChain);

    CreateEditorCB(pDevice);
    // Initialize the supplementary wireframe PSO for editor rendering
    InitializeWireframePSO(pDevice);
}

void Diligent::EditorPipeline::CreateEditorCB(IRenderDevice* pDevice)
{
    BufferDesc CBDesc;
    CBDesc.Name = "Editor Constants Buffer";
    CBDesc.Size = sizeof(EditorConstants);
    CBDesc.Usage = USAGE_DYNAMIC;
    CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

    pDevice->CreateBuffer(CBDesc, nullptr, &m_pEditorCB);
}

void Diligent::EditorPipeline::InitializeWireframePSO(IRenderDevice* pDevice)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Editor G-Buffer Wireframe PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    SetupDefaultGraphicsPipeline(GraphicsPipeline);

    GraphicsPipeline.RasterizerDesc.FillMode = FILL_MODE_WIREFRAME;
    GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;

    // Apply depth bias to pull the wireframe slightly forward toward the camera,
    // preventing z-fighting when rendering simultaneously with solid surfaces.
    GraphicsPipeline.RasterizerDesc.DepthBias = -50000;
    GraphicsPipeline.RasterizerDesc.SlopeScaledDepthBias = -2.0f;

    GraphicsPipeline.NumRenderTargets = 3;
    GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA8_UNORM;
    GraphicsPipeline.RTVFormats[1] = TEX_FORMAT_RGBA16_FLOAT;
    GraphicsPipeline.RTVFormats[2] = TEX_FORMAT_RGBA32_FLOAT;
    GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;

    std::vector<LayoutElement> std_layout = VertexLayouts::GetStandardLayout();
    GraphicsPipeline.InputLayout.LayoutElements = std_layout.data();
    GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(std_layout.size());

    auto pVS = ShaderManager::GetInstance().GetShader("main_vs.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");

    // Load our brand new dedicated wireframe shader!
    // Make sure the path matches where Master saved the hlsl file!
    auto pPS = ShaderManager::GetInstance().GetShader("Deferred/editor_wireframe_ps.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    // Only define the specific variables our tiny custom shader actually needs
    std::vector<ShaderResourceVariableDesc> Variables = {
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "CameraConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "ModelConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_PIXEL, "EditorConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC }
    };
    PSODesc.ResourceLayout.Variables = Variables.data();
    PSODesc.ResourceLayout.NumVariables = static_cast<Uint32>(Variables.size());

    // No samplers needed for our solid color lines!
    PSODesc.ResourceLayout.NumImmutableSamplers = 0;
    PSODesc.ResourceLayout.ImmutableSamplers = nullptr;

    m_pWireframePSO.Release();
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pWireframePSO);

    // Create and bind variables to our dedicated wireframe SRB
    m_pWireframeSRB.Release();
    m_pWireframePSO->CreateShaderResourceBinding(&m_pWireframeSRB, true);

    if (auto* pVar = m_pWireframeSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants")) pVar->Set(m_pCameraCB);
    if (auto* pVar = m_pWireframeSRB->GetVariableByName(SHADER_TYPE_VERTEX, "ModelConstants"))  pVar->Set(m_pModelCB);
    if (auto* pVar = m_pWireframeSRB->GetVariableByName(SHADER_TYPE_PIXEL, "EditorConstants")) pVar->Set(m_pEditorCB);
}

void Diligent::EditorPipeline::RenderModels(IDeviceContext* pContext, RenderData renderData, bool renderOpaque)
{
    // Draw solid surfaces using the base PSO
    if (m_RenderSurfaces) {
        pContext->SetPipelineState(m_pPSO);
        DeferredPipeline::RenderModels(pContext, renderData, renderOpaque);
    }

    // Draw wireframes using the custom wireframe PSO
    if (m_RenderWireframe) {
        // 1. Update the custom buffer with the current property color
        {
            MapHelper<EditorConstants> CBData(pContext, m_pEditorCB, MAP_WRITE, MAP_FLAG_DISCARD);
            CBData->WireframeColor = m_WireframeColor;
        }

        // 2. Set the wireframe PSO
        pContext->SetPipelineState(m_pWireframePSO);

        // 3. Temporarily hijack the base class's SRB pointer so the base render loop binds our custom layout!
        auto pOriginalSRB = m_pSRB;
        m_pSRB = m_pWireframeSRB;

        DeferredPipeline::RenderModels(pContext, renderData, renderOpaque);

        // 4. Safely restore the original SRB for the next pass
        m_pSRB = pOriginalSRB;
    }
}