#pragma once

#include <memory>
#include <vector>
#include <string>

#include "Panels/BasePanel.hpp" 
#include "MenuBar.hpp" // Added include for our new class

#include "Panels/HierarchyPanel.hpp"
#include "Panels/UserSettingsPanel.hpp"
#include "Panels/ProfilerPanel.hpp"
#include "Panels/InspectorPanel/InspectorPanel.hpp"

#include "Panels/InspectorPanel/Components/TransformUI.hpp"
#include "Panels/InspectorPanel/Components/CameraUI.hpp"
#include "Panels/InspectorPanel/Components/DirectionalLightUI.hpp"
#include "Panels/InspectorPanel/Components/PointLightUI.hpp"

namespace Diligent {

    class ImGuiImplDiligent;

    class GUIManager
    {
    public:
        static GUIManager& GetInstance()
        {
            static GUIManager instance;
            return instance;
        }

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

    private:
        GUIManager() = default;
        ~GUIManager() = default;

        GUIManager(const GUIManager&) = delete;
        GUIManager& operator=(const GUIManager&) = delete;

        std::unique_ptr<ImGuiImplDiligent> m_pImGuiRenderer;

        // Manage all UI windows dynamically
        std::vector<std::unique_ptr<BasePanel>> m_Panels;

        // Hold a reference to the engine's device
        RefCntAutoPtr<IRenderDevice> m_pDevice;

        // The dedicated menu bar instance
        MenuBar m_MenuBar;
    };

}