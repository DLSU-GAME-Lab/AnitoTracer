#pragma once

#include <memory>
#include <vector>
#include <string>
#include <compare>

#include "Panels/BasePanel.hpp" 
#include "MenuBar.hpp"
#include "Gizmos/GizmoDrawer.hpp"

#include "Panels/HierarchyPanel.hpp"
#include "Panels/UserSettingsPanel.hpp"
#include "Panels/ProfilerPanel.hpp"
#include "Panels/FileExplorerPanel.hpp"
#include "Panels/InspectorPanel/InspectorPanel.hpp"

#include "Panels/InspectorPanel/Components/TransformUI.hpp"
#include "Panels/InspectorPanel/Components/CameraUI.hpp"
#include "Panels/InspectorPanel/Components/DirectionalLightUI.hpp"
#include "Panels/InspectorPanel/Components/PointLightUI.hpp"
#include "Panels/InspectorPanel/Components/ModelUI.hpp"

namespace Diligent {

    class ImGuiImplDiligent;

    class GUIManager
    {
    public:
        static GUIManager& GetInstance();

        void Initialize(IRenderDevice* pDevice, const SwapChainDesc& SCDesc, NativeWindow nativeWindow);
        void NewFrame(Uint32 width, Uint32 height, SURFACE_TRANSFORM transform);
        void DrawUI(bool& appRunning);
        void Render(IDeviceContext* pContext);
        void Shutdown();

        void InitializeDefaultPanels();
        void InitializeComponentDrawers();

        // Register a new panel to the manager
        void AddPanel(std::unique_ptr<BasePanel> panel)
        {
            m_Panels.push_back(std::move(panel));
        }

        bool IsInitialized() const { return m_pImGuiRenderer != nullptr; }

        //Bridge function to set the selected object
        void SetSelectedObject(HierarchyObject::Ref obj);
        //Bridge to get selected object
        HierarchyObject::Ref GetSelectedObject() const {
            return m_pHierarchyPanel ? m_pHierarchyPanel->GetSelectedObject() : nullptr;
        }

        GizmoDrawer& GetGizmoDrawer() { return m_GizmoDrawer; }
        void DrawGizmos(CameraComponent* pActiveCamera, float x, float y, float width, float height);

        void RegisterFileOpener(FileExplorerPanel::Opener opener);

    private:
        GUIManager() = default;
        ~GUIManager();

        GUIManager(const GUIManager&) = delete;
        GUIManager& operator=(const GUIManager&) = delete;

        std::unique_ptr<ImGuiImplDiligent> m_pImGuiRenderer;

        //Cache ref for laters
        HierarchyPanel* m_pHierarchyPanel = nullptr;
        FileExplorerPanel* m_pFileExplorerPanel = nullptr;
        std::vector<FileExplorerPanel::Opener> m_FileOpeners;

        // Manage all UI windows dynamically
        std::vector<std::unique_ptr<BasePanel>> m_Panels;

        // Hold a reference to the engine's device
        RefCntAutoPtr<IRenderDevice> m_pDevice;

        // The dedicated menu bar instance
        MenuBar m_MenuBar;

        //Gizmo Drawer instance
        GizmoDrawer m_GizmoDrawer;
    };

}