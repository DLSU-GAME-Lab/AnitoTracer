#pragma once

#include <memory>

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

#include "../Objects/CameraObj.hpp"

namespace Diligent {

class GUIManager
{
public:
    // Meyers Singleton access
    static GUIManager& GetInstance()
    {
        static GUIManager instance;
        return instance;
    }

    // Initialize ImGui context and Diligent renderer
    void Initialize(IRenderDevice* pDevice, const SwapChainDesc& SCDesc, NativeWindow nativeWindow);

    // Begin a new ImGui frame
    void NewFrame(Uint32 width, Uint32 height, SURFACE_TRANSFORM transform);

    // Draw the main layout (Menu bar and Dockspace)
    void DrawUI(bool& appRunning);

    // Render ImGui draw data to the Diligent context
    void Render(IDeviceContext* pContext);

    // Cleanup resources
    void Shutdown();

    // Set the camera object for properties editing
    void SetCamera(CameraObj* pCamera) { m_pCamera = pCamera; }

    bool IsInitialized() const { return m_pImGuiRenderer != nullptr; }

private:
    GUIManager() = default;
    ~GUIManager() = default;

    // Disable copy/move
    GUIManager(const GUIManager&) = delete;
    GUIManager& operator=(const GUIManager&) = delete;

    std::unique_ptr<ImGuiImplDiligent> m_pImGuiRenderer;
    CameraObj* m_pCamera = nullptr;

    // Window states
    bool m_ShowPropertiesWindow = true;
    bool m_ShowConsoleWindow = true;
};

}