#include "GUIManager.hpp"

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Platforms/interface/NativeWindow.h"

#if PLATFORM_WIN32
#include <windows.h>
#include "Imgui/interface/ImGuiImplWin32.hpp"
#endif

#include "Imgui/interface/ImGuiDiligentRenderer.hpp"
#include "Imgui/interface/ImGuiImplDiligent.hpp"
#include "../Objects/Components/EditorCamera.hpp"
#include "../Objects/Components/GameCamera.hpp"
#include <utility>

GUIManager& Diligent::GUIManager::GetInstance()
{
    static GUIManager instance;
    return instance;
}

// Initialize ImGui context and Diligent renderer
void Diligent::GUIManager::Initialize(IRenderDevice* pDevice, const SwapChainDesc& SCDesc, NativeWindow nativeWindow)
{
    if (m_pImGuiRenderer) return; // Already initialized

    m_pDevice = pDevice;

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

    ImGuizmo::BeginFrame();
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
    m_pHierarchyPanel = hierarchyPanel.get();

    Diligent::GUIManager::GetInstance().AddPanel(std::make_unique<Diligent::InspectorPanel>(m_pHierarchyPanel, "Inspector"));
    Diligent::GUIManager::GetInstance().AddPanel(std::make_unique<Diligent::UserSettingsPanel>());
    Diligent::GUIManager::GetInstance().AddPanel(std::make_unique<Diligent::ProfilerPanel>(m_pDevice));
    Diligent::GUIManager::GetInstance().AddPanel(std::move(hierarchyPanel));

    auto fileExplorerPanel = std::make_unique<Diligent::FileExplorerPanel>("File Explorer");
    m_pFileExplorerPanel = fileExplorerPanel.get();
    for (auto& opener : m_FileOpeners)
        m_pFileExplorerPanel->RegisterOpener(std::move(opener));
    m_FileOpeners.clear();
    Diligent::GUIManager::GetInstance().AddPanel(std::move(fileExplorerPanel));
}

void Diligent::GUIManager::InitializeComponentDrawers()
{
    InspectorRegistry::GetInstance().RegisterUI<Transform, TransformUI>();
    InspectorRegistry::GetInstance().RegisterUI<CameraComponent, CameraUI>();
    InspectorRegistry::GetInstance().RegisterUI<EditorCamera, CameraUI>();
    InspectorRegistry::GetInstance().RegisterUI<GameCamera, CameraUI>();
    InspectorRegistry::GetInstance().RegisterUI<DirectionalLight, DirectionalLightUI>();
    InspectorRegistry::GetInstance().RegisterUI<PointLight, PointLightUI>();
    InspectorRegistry::GetInstance().RegisterUI<ModelComponent, ModelUI>();
}

void Diligent::GUIManager::SetSelectedObject(HierarchyObject::Ref obj)
{
    // Ensure the panel exists before trying to call its method
    if (m_pHierarchyPanel)
    {
        // Forwards the object to the existing SetSelectedObject method in HierarchyPanel
        m_pHierarchyPanel->SetSelectedObject(obj);
    }
}

void Diligent::GUIManager::DrawGizmos(CameraComponent* pActiveCamera, float x, float y, float width, float height) {
    
    auto selectedObj = GetSelectedObject();
    if (selectedObj != nullptr) {
        m_GizmoDrawer.Draw(pActiveCamera, selectedObj, x, y, width, height);
    }

}

Diligent::GUIManager::~GUIManager() = default;
void Diligent::GUIManager::RegisterFileOpener(FileExplorerPanel::Opener opener)
{
    if (m_pFileExplorerPanel)
        m_pFileExplorerPanel->RegisterOpener(std::move(opener));
    else if (opener)
        m_FileOpeners.push_back(std::move(opener));
}
