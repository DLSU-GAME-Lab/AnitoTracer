#include "GUIManager.hpp"
#include "Panels/InspectorPanel/Components/ScriptComponentUI.hpp"

#include "../AppState.hpp"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Platforms/interface/NativeWindow.h"
#include "../Objects/HierarchyManager.hpp"

#if PLATFORM_WIN32
#include <windows.h>
#include "Imgui/interface/ImGuiImplWin32.hpp"
#endif

#include "Imgui/interface/ImGuiDiligentRenderer.hpp"
#include "Imgui/interface/ImGuiImplDiligent.hpp"
#include "../Objects/Components/EditorCamera.hpp"
#include "../Objects/Components/GameCamera.hpp"
#include <imgui.h>
#include <glm/glm.hpp>
#include <utility>
#include <algorithm>

#include "Theme.hpp"

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
    //Docking support
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //So you can drag the gizmo not the windows itself
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // Enable saving of window layout (positions and sizes)
    io.IniFilename = "imgui.ini";

    ImGui::StyleColorsDark();
    ApplyTheme();

    InitializeDefaultPanels();
    InitializeComponentDrawers();
}

// Begin a new ImGui frame
void Diligent::GUIManager::NewFrame(Uint32 width, Uint32 height, SURFACE_TRANSFORM transform)
{
    if (!m_pImGuiRenderer) return;

    // Prevent starting a frame with invalid dimensions
    if (width == 0 || height == 0) return;

    if (!m_PendingLayout.empty())
    {
        const auto path = GetLayoutsDirectory() / (m_PendingLayout + ".ini");
        m_PendingLayout.clear();
        if (std::filesystem::exists(path))
            ImGui::LoadIniSettingsFromDisk(path.string().c_str());
    }

    m_pImGuiRenderer->NewFrame(width, height, transform);
    ImGuizmo::BeginFrame();

    m_FrameStarted = true;
}

void Diligent::GUIManager::DrawUI(bool& appRunning)
{
    if (!m_pImGuiRenderer) return;

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode, nullptr);

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
    // Abort if no frame was started to prevent ImGui assertion crashes
    if (!m_pImGuiRenderer || !m_FrameStarted) return;

    m_pImGuiRenderer->Render(pContext);

    m_FrameStarted = false; // Reset for the next frame
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

    auto scriptEditorPanel = std::make_unique<Diligent::ScriptEditorPanel>("Script Editor");
    m_pScriptEditorPanel = scriptEditorPanel.get();
    m_pFileExplorerPanel->RegisterOpener(FileOpeners::CreateScriptFileOpener(
        [panel = m_pScriptEditorPanel](const std::filesystem::path& path) { panel->OpenFile(path); }));
    Diligent::GUIManager::GetInstance().AddPanel(std::move(scriptEditorPanel));
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
    InspectorRegistry::GetInstance().RegisterUI<Collider, ColliderUI>();
    InspectorRegistry::GetInstance().RegisterUI<AudioSourceComponent, AudioSourceUI>();
    InspectorRegistry::GetInstance().RegisterUI<ScriptComponent, ScriptComponentUI>();
    InspectorRegistry::GetInstance().RegisterUI<ParticleSystemComponent, ParticleSystemUI>();
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

void Diligent::GUIManager::ToggleSelectedObject(HierarchyObject::Ref obj)
{
    if (m_pHierarchyPanel)
    {
        m_pHierarchyPanel->ToggleSelectedObject(obj);
    }
}

void Diligent::GUIManager::DrawGizmos(CameraComponent* pActiveCamera, float x, float y, float width, float height) {
    
    auto selectedObj = GetSelectedObject();
    if (selectedObj == nullptr) {
        return;
    }

    const auto roots = GetSelectedRoots();
    if (roots.size() > 1) {
        m_GizmoDrawer.DrawGroup(pActiveCamera, roots, selectedObj, x, y, width, height);
    }
    else {
        m_GizmoDrawer.Draw(pActiveCamera, roots.empty() ? selectedObj : roots.front(), x, y, width, height);
    }

}

void Diligent::GUIManager::RegisterViewportPanels(std::function<ITextureView* ()> gameSrvGetter, std::function<ITextureView* ()> editorSrvGetter)
{
    auto gamePanel = std::make_unique<GamePanel>("Game Camera", std::move(gameSrvGetter));
    auto editorPanel = std::make_unique<EditorPanel>("Editor Camera", std::move(editorSrvGetter));
    m_pGamePanel = gamePanel.get();
    m_pEditorPanel = editorPanel.get();
    AddPanel(std::move(gamePanel));
    AddPanel(std::move(editorPanel));
}

std::vector<std::string> Diligent::GUIManager::GetAvailableLayouts() const
{
    std::vector<std::string> layouts;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(GetLayoutsDirectory(), ec))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".ini")
            layouts.push_back(entry.path().stem().string());
    }
    std::sort(layouts.begin(), layouts.end());
    return layouts;
}

bool Diligent::GUIManager::SaveCurrentLayout(const std::string& name)
{
    // Reject anything that isn't a plain file name
    if (name.empty() || name.find_first_of("\\/:*?\"<>|") != std::string::npos || name == "." || name == "..")
        return false;

    std::error_code ec;
    std::filesystem::create_directories(GetLayoutsDirectory(), ec);
    const auto path = GetLayoutsDirectory() / (name + ".ini");
    ImGui::SaveIniSettingsToDisk(path.string().c_str());
    return std::filesystem::exists(path);
}

Diligent::GUIManager::~GUIManager() = default;
void Diligent::GUIManager::RegisterFileOpener(FileExplorerPanel::Opener opener)
{
    if (m_pFileExplorerPanel)
        m_pFileExplorerPanel->RegisterOpener(std::move(opener));
    else if (opener)
        m_FileOpeners.push_back(std::move(opener));
}

namespace {

ImU32 ScriptColor(const glm::vec3& rgb, float alpha)
{
    const auto channel = [](float value) {
        return static_cast<int>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
    };
    return IM_COL32(channel(rgb.r), channel(rgb.g), channel(rgb.b), channel(alpha));
}

}

void Diligent::GUIManager::GetScriptViewport(ImVec2& pos, ImVec2& size) const
{
    pos = ImVec2(0.0f, 0.0f);
    size = ImGui::GetIO().DisplaySize;
    if (!AppState::isReleaseBuild) {
        pos = GetGameViewportPos();
        size = GetGameViewportSize();
    }
}

glm::vec3 Diligent::GUIManager::ScriptWorldToScreen(const glm::vec3& world) const
{
    constexpr float HiddenDepth = -1.0f;
    glm::vec3 result{0.0f, 0.0f, HiddenDepth};
    glm::mat4 view(1.0f), projection(1.0f);
    HierarchyManager& hierarchy = HierarchyManager::GetInstance();
    if (!hierarchy.GetMainCamera() || !hierarchy.GetMainCameraMatrices(view, projection)) return result;

    ImVec2 pos, size;
    GetScriptViewport(pos, size);
    if (size.x <= 1.0f || size.y <= 1.0f) return result;

    const glm::vec4 clip = projection * view * glm::vec4(world, 1.0f);
    if (clip.w <= 0.0001f) return result;
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    const bool visible = ndc.z >= 0.0f && ndc.z <= 1.0f;
    result = {pos.x + (ndc.x * 0.5f + 0.5f) * size.x,
              pos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * size.y,
              visible ? ndc.z : HiddenDepth};
    return result;
}

void Diligent::GUIManager::ScriptFillRect(const glm::vec3& min, const glm::vec3& max, const glm::vec3& rgb,
                                          float alpha) const
{
    ImVec2 pos, size;
    GetScriptViewport(pos, size);
    ImDrawList* list = ImGui::GetForegroundDrawList();
    list->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);
    list->AddRectFilled(ImVec2(min.x, min.y), ImVec2(max.x, max.y), ScriptColor(rgb, alpha));
    list->PopClipRect();
}

void Diligent::GUIManager::ScriptDrawRect(const glm::vec3& min, const glm::vec3& max, const glm::vec3& rgb,
                                          float alpha, float thickness) const
{
    ImVec2 pos, size;
    GetScriptViewport(pos, size);
    ImDrawList* list = ImGui::GetForegroundDrawList();
    list->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);
    list->AddRect(ImVec2(min.x, min.y), ImVec2(max.x, max.y), ScriptColor(rgb, alpha), 0.0f, 0, thickness);
    list->PopClipRect();
}
