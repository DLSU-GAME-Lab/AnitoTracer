#pragma once

#include <memory>
#include <variant>

// Ensure Unicode Windows API
#define UNICODE
#define _UNICODE

// Diligent Engine Core
#include "Graphics/GraphicsEngine/interface/EngineFactory.h"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"

// Diligent Platform Abstraction
#if PLATFORM_WIN32
#    include <windows.h>
#elif PLATFORM_LINUX
#    include "Platforms/Linux/interface/LinuxNativeWindow.h"
#elif PLATFORM_MACOS
#    include "Platforms/Apple/interface/MacNativeWindow.h"
#endif

// Forward declarations for pipelines and objects
#include "Rendering/Pipelines/BasicLitPipeline.hpp"
#include "Rendering/Pipelines/HybridPipeline.hpp"
#include "Objects/HierarchyObject.hpp"

class AnitoTracer_App
{
public:
    AnitoTracer_App();
    ~AnitoTracer_App();

    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    void Run();
    void Shutdown();

    // Callbacks for the native WindowProc
    void OnResize(short width, short height);
    void OnDestroy();

private:
    bool InitWindow(HINSTANCE hInstance, int nCmdShow);
    bool InitEngine();
    void InitManagers();
    void CreateMSAABuffers();

    void Update();
    void Render();
    void UpdateCameraControls();
    void HandleObjectPicking(const Diligent::SwapChainDesc& SCDesc, const struct RenderData& renderData);

private:
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  m_pDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_pImmediateContext;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain>     m_pSwapChain;

    Diligent::RefCntAutoPtr<Diligent::ITexture>     m_pMSAATarget;
    Diligent::RefCntAutoPtr<Diligent::ITexture>     m_pMSAADepth;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> m_pMSAARTV;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> m_pMSAADSV;

    Diligent::NativeWindow m_NativeWindow;
    bool m_AppRunning;
    bool m_LastMSAAState;

    Diligent::Uint32 m_WindowWidth;
    Diligent::Uint32 m_WindowHeight;

    HierarchyObject::Ref m_MainCam;
    std::variant<HybridPipeline, BasicLitPipeline> m_bLitPipeline;
};