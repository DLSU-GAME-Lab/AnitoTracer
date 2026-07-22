#include "AnitoTracer_Rebuild.h"

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
#include "src/Rendering/Shaders/ShaderManager.hpp"
#include "src/Rendering/Pipelines/BasicPipeline.hpp"
#include "src/Rendering/Pipelines/TexturedPipeline.hpp"
#include "src/Rendering/Pipelines/BasicLitPipeline.hpp"
#include "src/Objects/Models/ModelManager.hpp"

#include "src/Objects/HierarchyManager.hpp"
#include "src/Objects/ObjectFactory.hpp"
#include "src/UserSettings.hpp"

using namespace Diligent;

// Global application state wrappers
RefCntAutoPtr<IRenderDevice>  g_pDevice;
RefCntAutoPtr<IDeviceContext> g_pImmediateContext;
RefCntAutoPtr<ISwapChain>     g_pSwapChain;

RefCntAutoPtr<ITexture>     g_pMSAATarget;
RefCntAutoPtr<ITexture>     g_pMSAADepth;
RefCntAutoPtr<ITextureView> g_pMSAARTV;
RefCntAutoPtr<ITextureView> g_pMSAADSV;

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
    HMODULE hDXC = LoadLibraryW(L"spv_dxcompiler.dll");
    if (!hDXC) {
        DWORD err = GetLastError();
        std::cout << "Failed to load spv_dxcompiler.dll. Error Code: " << err << std::endl;
    }
    else {
        std::cout << "Successfully loaded spv_dxcompiler.dll!" << std::endl;
        FreeLibrary(hDXC);
    }
#endif

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

    engineCI.Features.RayTracing = Diligent::DEVICE_FEATURE_STATE_ENABLED;

    SwapChainDesc swapChainDesc;
    swapChainDesc.Width = windowWidth;
    swapChainDesc.Height = windowHeight;

    pFactoryVk->CreateDeviceAndContextsVk(engineCI, &g_pDevice, &g_pImmediateContext); 
    pFactoryVk->CreateSwapChainVk(g_pDevice, g_pImmediateContext, swapChainDesc, g_NativeWindow, &g_pSwapChain);

    auto CreateMSAABuffers = [&]() {
        const auto& SCDesc = g_pSwapChain->GetDesc();

        TextureDesc ColorDesc;
        ColorDesc.Name = "MSAA Color Target";
        ColorDesc.Type = RESOURCE_DIM_TEX_2D;
        ColorDesc.Width = SCDesc.Width;
        ColorDesc.Height = SCDesc.Height;
        ColorDesc.BindFlags = BIND_RENDER_TARGET;
        ColorDesc.Format = SCDesc.ColorBufferFormat;
        ColorDesc.SampleCount = 4; // Must match the PSO!

        g_pMSAATarget.Release();
        g_pDevice->CreateTexture(ColorDesc, nullptr, &g_pMSAATarget);
        g_pMSAARTV = g_pMSAATarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);

        TextureDesc DepthDesc = ColorDesc;
        DepthDesc.Name = "MSAA Depth Buffer";
        DepthDesc.BindFlags = BIND_DEPTH_STENCIL;
        DepthDesc.Format = SCDesc.DepthBufferFormat;

        g_pMSAADepth.Release();
        g_pDevice->CreateTexture(DepthDesc, nullptr, &g_pMSAADepth);
        g_pMSAADSV = g_pMSAADepth->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        };

    // Initialize them for the first time
    CreateMSAABuffers();

    // Create the triangle objects after initialization

    Diligent::ShaderManager::GetInstance().Initialize(g_pDevice, "Shaders");

    // IMPORTANT: Initialize ModelManager BEFORE loading any models
    ModelManager::GetInstance().Initialize(g_pDevice, g_pImmediateContext, "Assets/");

    GUIManager& imguiManager = GUIManager::GetInstance();

    auto bPipeline = BasicPipeline();
    auto tPipeline = TexturedPipeline();
    auto bLitPipeline = LitPipeline();

    ObjectFactory& objFactory = ObjectFactory::GetInstance();

    auto MainCam = objFactory.CreateRootCameraObject("Camera Main");
    MainCam->GetTransform()->SetPosition(glm::vec3(0, 0, -10.f));

    objFactory.CreateModelObject("Bonk", "helmet/DamagedHelmet.gltf");
    objFactory.CreateModelObject("SP", "Sponza/sponza.obj");

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

            //bPipeline.InitializePipeline(g_pDevice, g_pSwapChain);
            //tPipeline.InitializePipeline(g_pDevice, g_pSwapChain);
            bLitPipeline.InitializePipeline(g_pDevice, g_pSwapChain);
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

        // Resize MSAA buffers dynamically if the window size changed
        if (g_pMSAATarget->GetDesc().Width != SCDesc.Width ||
            g_pMSAATarget->GetDesc().Height != SCDesc.Height)
        {
            CreateMSAABuffers();
        }

        const float clearColor[] = { 0.1f, 0.15f, 0.25f, 1.0f };

        ITextureView* pRTVs[] = { g_pMSAARTV };
        g_pImmediateContext->SetRenderTargets(1, pRTVs, g_pMSAADSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        g_pImmediateContext->ClearRenderTarget(g_pMSAARTV, clearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        g_pImmediateContext->ClearDepthStencil(g_pMSAADSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // Set up target attachments and clear color
        //auto* pRTV = g_pSwapChain->GetCurrentBackBufferRTV();
        //auto* pDSV = g_pSwapChain->GetDepthBufferDSV(); 
        //g_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        //g_pImmediateContext->ClearRenderTarget(pRTV, clearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION); 
        //g_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION); 
 
        // ==========================================
        // --- RENDER (BEFORE IMGUI)
        // ==========================================

        RenderData renderData;

        HierarchyManager::GetInstance().GetMainCameraMatrices(renderData.ViewMatrix, renderData.ProjectionMatrix);
        HierarchyManager::GetInstance().GatherRenderModels(renderData.Models);
        HierarchyManager::GetInstance().GatherLightData(renderData.Lights);

        bLitPipeline.StartFrameRender(g_pImmediateContext, renderData);
        bLitPipeline.UpdateLights(g_pImmediateContext, renderData.Lights);
        bLitPipeline.UpdateShadowSettings(g_pImmediateContext, UserSettings::GetInstance().GetShadowSettings());
        bLitPipeline.RenderModels(g_pImmediateContext, renderData);

        // ==========================================
        // --- RESOLVE MSAA AND RENDER IMGUI
        // ==========================================

        auto* pBackBufferRTV = g_pSwapChain->GetCurrentBackBufferRTV();

        // Resolve the multi-sampled texture into the swap chain back buffer
        ResolveTextureSubresourceAttribs ResolveAttribs;
        ResolveAttribs.SrcTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        ResolveAttribs.DstTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

        g_pImmediateContext->ResolveTextureSubresource(
            g_pMSAATarget,
            pBackBufferRTV->GetTexture(),
            ResolveAttribs
        );
        //Bind the back buffer
        auto* pDefaultDSV = g_pSwapChain->GetDepthBufferDSV();
        g_pImmediateContext->SetRenderTargets(1, &pBackBufferRTV, pDefaultDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

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