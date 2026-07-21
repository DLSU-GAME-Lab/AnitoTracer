#include "GUIManager.hpp"

// Initialize ImGui context and Diligent renderer
void Diligent::GUIManager::Initialize(IRenderDevice* pDevice, const SwapChainDesc& SCDesc, NativeWindow nativeWindow)
{
    if (m_pImGuiRenderer) return; // Already initialized

    ImGuiDiligentCreateInfo imguiCI;
    imguiCI.pDevice = pDevice;
    imguiCI.BackBufferFmt = SCDesc.ColorBufferFormat;
    imguiCI.DepthBufferFmt = SCDesc.DepthBufferFormat;

#if PLATFORM_WIN32
    HWND hWnd = reinterpret_cast<HWND>(nativeWindow.hWnd);
    m_pImGuiRenderer = Diligent::ImGuiImplWin32::Create(imguiCI, hWnd);
#else
    m_pImGuiRenderer = std::make_unique<ImGuiImplDiligent>(imguiCI);
#endif

    // ImGuiImplDiligent constructor creates the context and initializes it
    ImGuiIO& io = ImGui::GetIO();
    // Enable Keyboard Controls (optional but recommended)
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Enable saving of window layout (positions and sizes)
    io.IniFilename = "imgui.ini";

    ImGui::StyleColorsDark();

    InitializeDefaultPanels();
    InitializeComponentDrawers();
}

// Begin a new ImGui frame
void Diligent::GUIManager::NewFrame(Uint32 width, Uint32 height, SURFACE_TRANSFORM transform)
{
    if (!m_pImGuiRenderer) return;
    m_pImGuiRenderer->NewFrame(width, height, transform);
}

void Diligent::GUIManager::DrawUI(bool& appRunning)
{
    if (!m_pImGuiRenderer) return;

    // Delegate menu bar rendering to the dedicated class
    m_MenuBar.Draw(appRunning, m_Panels);

    // Render all active dockable windows
    for (auto& panel : m_Panels)
    {
        if (panel->GetVisible())
        {
            panel->Draw();
        }
    }
}

// Render ImGui draw data to the Diligent context
void Diligent::GUIManager::Render(IDeviceContext* pContext)
{
    if (!m_pImGuiRenderer) return;
    m_pImGuiRenderer->Render(pContext);
}

// Cleanup resources
void Diligent::GUIManager::Shutdown()
{
    // ImGuiImplDiligent and ImGuiImplWin32 destructors handle:
    // - Win32 backend shutdown (ImGui_ImplWin32_Shutdown)
    // - Renderer device objects cleanup
    // - ImGui context destruction
    m_pImGuiRenderer.reset();
}

void Diligent::GUIManager::InitializeDefaultPanels()
{
    auto hierarchyPanel = std::make_unique<Diligent::HierarchyPanel>("Hierarchy");
    Diligent::HierarchyPanel* hierarchyPtr = hierarchyPanel.get();

    Diligent::GUIManager::GetInstance().AddPanel(std::make_unique<Diligent::InspectorPanel>(hierarchyPtr, "Inspector"));
    Diligent::GUIManager::GetInstance().AddPanel(std::move(hierarchyPanel));
}

void Diligent::GUIManager::InitializeComponentDrawers()
{
    InspectorRegistry::GetInstance().RegisterUI<Transform, TransformUI>();
    InspectorRegistry::GetInstance().RegisterUI<CameraComponent, CameraUI>();
    InspectorRegistry::GetInstance().RegisterUI<DirectionalLight, DirectionalLightUI>();
}
