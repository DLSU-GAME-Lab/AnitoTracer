#include <memory>
#include <iostream>

// Ensure Unicode Windows API
#define UNICODE
#define _UNICODE

// Diligent Engine Core
#include "DiligentEngine/DiligentCore/Graphics/GraphicsEngine/interface/EngineFactory.h"
#include "DiligentEngine/DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "DiligentEngine/DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "DiligentEngine/DiligentCore/Graphics/GraphicsEngine/interface/SwapChain.h"

#include "DiligentEngine/DiligentCore/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
//#include "GraphicsEngineVulkan/interface/EngineFactoryVk.h"

//#include "GraphicsEngine/interface/EngineFactory.h"
//#include "GraphicsEngine/interface/RenderDevice.h"
//#include "GraphicsEngine/interface/DeviceContext.h"
//#include "GraphicsEngine/interface/SwapChain.h"

// Diligent Platform Abstraction (Handles Windows/Linux/Mac windows natively)
#if PLATFORM_WIN32
#    include <windows.h>
#    include "Platforms/Win32/interface/Win32NativeWindow.h"
#elif PLATFORM_LINUX
#    include "Platforms/Linux/interface/LinuxNativeWindow.h"
#elif PLATFORM_MACOS
#    include "Platforms/Apple/interface/MacNativeWindow.h"
#endif

// Diligent Integrated ImGui
#include "Imgui/interface/ImGuiDiligentRenderer.hpp"
#include "Imgui/interface/ImGuiImplDiligent.hpp"
#include "imgui.h"

using namespace Diligent;

// Global application state wrappers
RefCntAutoPtr<IRenderDevice>  g_pDevice;
RefCntAutoPtr<IDeviceContext> g_pImmediateContext;
RefCntAutoPtr<ISwapChain>     g_pSwapChain;
std::unique_ptr<ImGuiImplDiligent> g_pImGuiRenderer;

// Cross-platform native window handle tracker
NativeWindow g_NativeWindow;
bool g_AppRunning = true;

#if PLATFORM_WIN32
// Win32 Window message handling callback loop
LRESULT CALLBACK EngineWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Pass events directly to ImGui's internal Win32 handler if initialized
    if (ImGui::GetCurrentContext() != nullptr)
    {
        extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
        if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
            return true;
    }

    switch (message)
    {
    case WM_SIZE:
        if (g_pSwapChain)
        {
            short width = LOWORD(lParam);
            short height = HIWORD(lParam);
            g_pSwapChain->Resize(width, height);
        }
        return 0;
    case WM_DESTROY:
        g_AppRunning = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
#endif

// Main entry point logic
#if PLATFORM_WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char** argv)
#endif
{
    Uint32 windowWidth = 1280;
    Uint32 windowHeight = 720;

#if PLATFORM_WIN32
    // 1. Create native OS Window manually on Win32 without GLFW
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = EngineWindowProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = L"DiligentVulkanImGuiWindow";
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"DiligentVulkanImGuiWindow", L"AnitoTracer - Diligent Vulkan + ImGui",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        (int)windowWidth, (int)windowHeight, nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hWnd, nCmdShow);

    g_NativeWindow.hWnd = hWnd;
#else
#error Platform window creation logic must be declared for non-Windows builds.
#endif

    // 2. Initialize Diligent Engine Vulkan Factory
    IEngineFactoryVk* pFactoryVk = Diligent::LoadAndGetEngineFactoryVk();

    EngineVkCreateInfo engineCI;
    SwapChainDesc swapChainDesc;
    swapChainDesc.Width = windowWidth;
    swapChainDesc.Height = windowHeight;

    // Create device and contexts first
    pFactoryVk->CreateDeviceAndContextsVk(engineCI, &g_pDevice, &g_pImmediateContext);

    // Create swap chain
    pFactoryVk->CreateSwapChainVk(g_pDevice, g_pImmediateContext, swapChainDesc, g_NativeWindow, &g_pSwapChain);

    // 3. Initialize Diligent's ImGui Subsystem
    ImGuiDiligentCreateInfo imguiCI;
    imguiCI.pDevice = g_pDevice;
    imguiCI.BackBufferFmt = g_pSwapChain->GetDesc().ColorBufferFormat;
    imguiCI.DepthBufferFmt = g_pSwapChain->GetDesc().DepthBufferFormat;

    // Create modern ImGui interface handle
    g_pImGuiRenderer = std::make_unique<ImGuiImplDiligent>(imguiCI);

    // 4. Main Runtime Loop
    while (g_AppRunning)
    {
#if PLATFORM_WIN32
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
#endif
        if (!g_AppRunning) break;

        const auto& SCDesc = g_pSwapChain->GetDesc();
        if (!g_pImGuiRenderer && SCDesc.Width > 0 && SCDesc.Height > 0)
        {
            ImGuiDiligentCreateInfo imguiCI;
            imguiCI.pDevice = g_pDevice;
            imguiCI.BackBufferFmt = SCDesc.ColorBufferFormat;
            imguiCI.DepthBufferFmt = SCDesc.DepthBufferFormat;
            g_pImGuiRenderer = std::make_unique<ImGuiImplDiligent>(imguiCI);
        }

        if (g_pImGuiRenderer) {
            const auto& CurrentSCDesc = g_pSwapChain->GetDesc();

            if (!(CurrentSCDesc.Width > 0 && CurrentSCDesc.Height > 0)) continue;

            auto transform = CurrentSCDesc.PreTransform;
            if (transform == SURFACE_TRANSFORM_OPTIMAL)
                transform = SURFACE_TRANSFORM_IDENTITY;

            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = ImVec2(static_cast<float>(SCDesc.Width), static_cast<float>(SCDesc.Height));

            g_pImGuiRenderer->NewFrame(SCDesc.Width, SCDesc.Height, transform);

            // --- Render ImGui Top Menu Bar ---
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("New Project", "Ctrl+N")) {}
                    if (ImGui::MenuItem("Open...", "Ctrl+O")) {}
                    ImGui::Separator();
                    if (ImGui::MenuItem("Exit", "Alt+F4")) { g_AppRunning = false; }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit"))
                {
                    if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
                    if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Help"))
                {
                    if (ImGui::MenuItem("About AnitoTracer")) {}
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            // Clear the Vulkan backbuffer surface color
            auto* pRTV = g_pSwapChain->GetCurrentBackBufferRTV();
            auto* pDSV = g_pSwapChain->GetDepthBufferDSV();
            g_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            const float clearColor[] = { 0.1f, 0.15f, 0.25f, 1.0f };
            g_pImmediateContext->ClearRenderTarget(pRTV, clearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            g_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            // Draw ImGui layers to the graphics command list context
            g_pImGuiRenderer->Render(g_pImmediateContext);

            // Swap the screen surface buffers
            g_pSwapChain->Present(1);
        }

        
    }

    // Clean up allocated ImGui resources before releasing Vulkan instances
    g_pImGuiRenderer.reset();
    g_pSwapChain.Release();
    g_pImmediateContext.Release();
    g_pDevice.Release();

    return 0;
}