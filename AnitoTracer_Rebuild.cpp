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

// Diligent Platform Abstraction
#if PLATFORM_WIN32
#    include <windows.h>
#include "DiligentEngine/DiligentTools/Imgui/interface/ImGuiImplWin32.hpp"
#elif PLATFORM_LINUX
#    include "DiligentEngine/DiligentCore/Platforms/Linux/interface/LinuxNativeWindow.h"
#elif PLATFORM_MACOS
#    include "DiligentEngine/DiligentCore/Platforms/Apple/interface/MacNativeWindow.h"
#endif

// Diligent Integrated ImGui
#include "DiligentEngine/DiligentTools/Imgui/interface/ImGuiDiligentRenderer.hpp"
#include "DiligentEngine/DiligentTools/Imgui/interface/ImGuiImplDiligent.hpp"
#include "imgui.h"

#include "src/UI/GUIManager.hpp"
#include "src/Objects/CameraObj.hpp"
#include "src/Rendering/Shaders/ShaderManager.hpp"
#include "src/Rendering/BasicPipeline.hpp"
#include "src/Objects/Models/ModelManager.hpp"

using namespace Diligent;

// Global application state wrappers
RefCntAutoPtr<IRenderDevice>  g_pDevice;
RefCntAutoPtr<IDeviceContext> g_pImmediateContext;
RefCntAutoPtr<ISwapChain>     g_pSwapChain;

// Cross-platform native window handle tracker
NativeWindow g_NativeWindow;
bool g_AppRunning = true;

#if PLATFORM_WIN32
// Win32 Window message handling callback loop
LRESULT CALLBACK EngineWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
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

#if PLATFORM_WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char** argv)
#endif
{
    Uint32 windowWidth = 1280;
    Uint32 windowHeight = 720;

#if PLATFORM_WIN32
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

    IEngineFactoryVk* pFactoryVk = Diligent::LoadAndGetEngineFactoryVk(); 

    EngineVkCreateInfo engineCI;
    SwapChainDesc swapChainDesc;
    swapChainDesc.Width = windowWidth;
    swapChainDesc.Height = windowHeight;

    pFactoryVk->CreateDeviceAndContextsVk(engineCI, &g_pDevice, &g_pImmediateContext); 
    pFactoryVk->CreateSwapChainVk(g_pDevice, g_pImmediateContext, swapChainDesc, g_NativeWindow, &g_pSwapChain);

    // Create the triangle objects after initialization

    Diligent::ShaderManager::GetInstance().Initialize(g_pDevice, "Shaders");

    GUIManager& imguiManager = GUIManager::GetInstance();

    CameraObj camera;

    imguiManager.SetCamera(&camera);

    auto bPipeline = BasicPipeline();

    ModelManager::GetInstance().Initialize(g_pDevice, "Assets/");
    Model* pMyModel = ModelManager::GetInstance().LoadModel("helmet/DamagedHelmet.gltf");
        
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

        // Initialize ImGui renderer on first valid frame
        if (!imguiManager.IsInitialized() && SCDesc.Width > 0 && SCDesc.Height > 0)
        {
            imguiManager.Initialize(g_pDevice, SCDesc, g_NativeWindow);

            bPipeline.InitializePipeline(g_pDevice, g_pSwapChain);
        }

        // Skip frame if renderer not ready or swapchain invalid
        if (!imguiManager.IsInitialized() || !(SCDesc.Width > 0 && SCDesc.Height > 0))
        {
            continue;
        }

        auto transform = SCDesc.PreTransform; 
        if (transform == SURFACE_TRANSFORM_OPTIMAL)
            transform = SURFACE_TRANSFORM_IDENTITY; 

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(static_cast<float>(SCDesc.Width), static_cast<float>(SCDesc.Height));

        // Start ImGui frame
        imguiManager.NewFrame(SCDesc.Width, SCDesc.Height, transform);

        // --- Render ImGui UI Elements ---
        imguiManager.DrawUI(g_AppRunning);

        // Set up target attachments and clear color
        auto* pRTV = g_pSwapChain->GetCurrentBackBufferRTV();
        auto* pDSV = g_pSwapChain->GetDepthBufferDSV(); 
        g_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const float clearColor[] = { 0.1f, 0.15f, 0.25f, 1.0f }; 
        g_pImmediateContext->ClearRenderTarget(pRTV, clearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION); 
        g_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION); 

        // ==========================================
        // --- RENDER (BEFORE IMGUI)
        // ==========================================

        bPipeline.StartFrameRender(g_pImmediateContext, camera);
        bPipeline.RenderModel(g_pImmediateContext, pMyModel);

        // ==========================================
        // Render ImGui over the Render
        imguiManager.Render(g_pImmediateContext);

        g_pSwapChain->Present(1);
    }

    if (g_pImmediateContext) g_pImmediateContext->Flush();
    if (g_pDevice) g_pDevice->IdleGPU();

    Diligent::ShaderManager::GetInstance().Shutdown();
    // Clean up ImGui through the Singleton
    imguiManager.Shutdown();

    g_pSwapChain.Release(); 
    g_pImmediateContext.Release(); 
    g_pDevice.Release(); 

    return 0;
}