#include "AnitoTracer_App.hpp"

#include <iostream>

#include "Imgui/interface/ImGuiImplWin32.hpp"
#include "Imgui/interface/ImGuiDiligentRenderer.hpp"
#include "Imgui/interface/ImGuiImplDiligent.hpp"
#include "imgui.h"

#include "UI/GUIManager.hpp"
#include "Rendering/Shaders/ShaderManager.hpp"
#include "Rendering/Models/ModelManager.hpp"
#include "Objects/HierarchyManager.hpp"
#include "Objects/ObjectFactory.hpp"
#include "UserSettings.hpp"
#include "UI/ObjectPicker.hpp"
#include "Asset/AssetPipeline.hpp"

#include "ObjectSystems/Event/Example/Print_OnSceneLoad.hpp"

#include "Physics/PhysicsEngine.hpp"

#include ANITO_EVENT_INCLUDES

using namespace Diligent;

// Global pointer required for the static WindowProc to route messages back to the class instance.
static AnitoTracer_App* g_pAppInstance = nullptr;

#if PLATFORM_WIN32
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
        if (g_pAppInstance)
        {
            short width = LOWORD(lParam);
            short height = HIWORD(lParam);
            g_pAppInstance->OnResize(width, height);
        }
        return 0;
    case WM_DESTROY:
        if (g_pAppInstance)
        {
            g_pAppInstance->OnDestroy();
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
#endif

AnitoTracer_App::AnitoTracer_App()
    : m_AppRunning(true)
    , m_LastMSAAState(false)
    , m_WindowWidth(1280)
    , m_WindowHeight(720)
{
    g_pAppInstance = this;
}

AnitoTracer_App::~AnitoTracer_App()
{
    g_pAppInstance = nullptr;
}

bool AnitoTracer_App::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    SubscribeToStandardEvents();

    if (!InitWindow(hInstance, nCmdShow)) return false;
    if (!InitEngine()) return false;

    InitManagers();

    m_LastMSAAState = UserSettings::GetInstance().GetEnableMSAA();

    //Dispatch with empty EventArgs
    EventSystem::DispatchTo(EVENT_ON_APP_INITIALIZE, std::make_unique<EventArgs>());

    return true;
}

bool AnitoTracer_App::InitWindow(HINSTANCE hInstance, int nCmdShow)
{
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

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = EngineWindowProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = L"DiligentVulkanImGuiWindow";
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"DiligentVulkanImGuiWindow", L"AnitoTracer - Diligent Vulkan + ImGui",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        (int)m_WindowWidth, (int)m_WindowHeight, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return false;

    ShowWindow(hWnd, nCmdShow);
    m_NativeWindow.hWnd = hWnd;
    return true;
#else
#error Platform window creation logic must be declared for non-Windows builds.
    return false;
#endif
}

bool AnitoTracer_App::InitEngine()
{
    IEngineFactoryVk* pFactoryVk = Diligent::LoadAndGetEngineFactoryVk();
    EngineVkCreateInfo engineCI;
    engineCI.Features.RayTracing = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;

#if defined(_DEBUG) || defined(DEBUG)
    // Enable the Vulkan validation layer in debug builds so driver-specific
    // usage errors (invalid buffer alignment, resource state, RT feature
    // misuse, etc.) surface as explicit messages instead of silently
    // producing incorrect rendering on some GPUs/drivers.
    engineCI.EnableValidation = true;
#endif

    SwapChainDesc swapChainDesc;
    swapChainDesc.Width = m_WindowWidth;
    swapChainDesc.Height = m_WindowHeight;

    pFactoryVk->CreateDeviceAndContextsVk(engineCI, &m_pDevice, &m_pImmediateContext);
    pFactoryVk->CreateSwapChainVk(m_pDevice, m_pImmediateContext, swapChainDesc, m_NativeWindow, &m_pSwapChain);

    const GraphicsAdapterInfo& AdapterInfo = m_pDevice->GetAdapterInfo();
    std::cout << "[Info] GPU Adapter: " << AdapterInfo.Description
        << " (VendorId=0x" << std::hex << AdapterInfo.VendorId << std::dec << ")" << std::endl;

    bool bSupportsRayTracing = (m_pDevice->GetDeviceInfo().Features.RayTracing == Diligent::DEVICE_FEATURE_STATE_ENABLED);

    RendererManager::GetInstance().Initialize(m_pDevice, m_pImmediateContext, m_pSwapChain, bSupportsRayTracing);

    return true;
}

void AnitoTracer_App::SubscribeToStandardEvents()
{
    m_OnInitializeSub = ScopedSubscription::Create<EventArgs>(
        EVENT_ON_APP_INITIALIZE,
        &AnitoTracer_App::HandleInitializeEvent,
        this
    );

    m_OnRenderStartSub = ScopedSubscription::Create<EventArgs>(
        EVENT_RENDER_START,
        &AnitoTracer_App::HandleRenderStartEvent,
        this
    );

    m_OnRenderEndSub = ScopedSubscription::Create<EventArgs>(
        EVENT_RENDER_END,
        &AnitoTracer_App::HandleRenderEndEvent,
        this
    );

    m_OnWindowResizeSub = gbe::ScopedSubscription::Create<WindowResizeArgs>(
        "EVENT_ONWINDOWRESIZE", //For testing
        &AnitoTracer_App::HandleWindowResizeEvent,
        this
    );
}

void AnitoTracer_App::InitManagers()
{
    Diligent::ShaderManager::GetInstance().Initialize(m_pDevice, "Shaders");
    ModelManager::GetInstance().Initialize(m_pDevice, m_pImmediateContext);
    
    AssetPipeline::IncludeFolder("Assets");

    ObjectFactory& objFactory = ObjectFactory::GetInstance();
    m_MainCam = objFactory.CreateRootCameraObject("Main Camera");
    m_MainCam.GetPtr()->GetTransform()->SetPosition(glm::vec3(0, 0, -10.f));
}

void AnitoTracer_App::OnResize(short width, short height)
{
    gbe::EventSystem::DispatchTo(
        "EVENT_ONWINDOWRESIZE", //For testing
        std::make_unique<WindowResizeArgs>(width, height)
    );
}

void AnitoTracer_App::OnDestroy()
{
    m_AppRunning = false;
}

void AnitoTracer_App::Run()
{
    //LifeCycle objects
    Print_OnSceneLoad print_OnSceneLoad; //test

    while (m_AppRunning)
    {
        Update();
        if (!m_AppRunning) break;
        Render();
    }
}

void AnitoTracer_App::Update()
{
#if PLATFORM_WIN32
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif

    if (!m_AppRunning) return;

    const auto& SCDesc = m_pSwapChain->GetDesc();
    GUIManager& imguiManager = GUIManager::GetInstance();

    if (!imguiManager.IsInitialized() && SCDesc.Width > 0 && SCDesc.Height > 0)
    {
        imguiManager.Initialize(m_pDevice, SCDesc, m_NativeWindow);
        RendererManager::GetInstance().InitializePipelines();
    }

    if (!imguiManager.IsInitialized() || !(SCDesc.Width > 0 && SCDesc.Height > 0))
    {
        return;
    }

    auto transform = SCDesc.PreTransform;
    if (transform == SURFACE_TRANSFORM_OPTIMAL)
        transform = SURFACE_TRANSFORM_IDENTITY;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(SCDesc.Width), static_cast<float>(SCDesc.Height));

    imguiManager.NewFrame(SCDesc.Width, SCDesc.Height, transform);
    UpdateCameraControls();
    imguiManager.DrawUI(m_AppRunning);

    //Draw Gizmos
    if (m_MainCam.GetPtr()) {
        imguiManager.DrawGizmos(
            m_MainCam.GetPtr()->GetComponent<CameraComponent>(),
            0.0f, 0.0f,
            static_cast<float>(SCDesc.Width), static_cast<float>(SCDesc.Height)
        );
    }

    //===============//EVENTS//===============//

	static double s_LastTime = ImGui::GetTime();
	double currentTime = ImGui::GetTime();
	float deltaTime = static_cast<float>(currentTime - s_LastTime);
	s_LastTime = currentTime;

	PhysicsEngine::GetInstance().Get().Step(deltaTime);
    HierarchyManager::GetInstance().DispatchEvent<FixedUpdateTrigger>(deltaTime);
    HierarchyManager::GetInstance().DispatchEvent<EditorUpdateTrigger>(0.016f); //test delta frame
}

void AnitoTracer_App::Render()
{
    EventSystem::DispatchTo(EVENT_RENDER_START, std::make_unique<EventArgs>());

    RenderData renderData;
    HierarchyManager::GetInstance().GetMainCameraMatrices(renderData.ViewMatrix, renderData.ProjectionMatrix);
    HierarchyManager::GetInstance().GatherRenderModels(renderData.Models);
    HierarchyManager::GetInstance().GatherLightData(renderData.Lights);

    // Pass the render data to the new manager to handle frame execution
    RendererManager::GetInstance().RenderFrame(renderData);

    const auto& SCDesc = m_pSwapChain->GetDesc();
    HandleObjectPicking(SCDesc, renderData);

    GUIManager::GetInstance().Render(m_pImmediateContext);
    m_pSwapChain->Present(1);

    EventSystem::DispatchTo(EVENT_RENDER_END, std::make_unique<EventArgs>());
}

void AnitoTracer_App::UpdateCameraControls()
{
    ImGuiIO& io = ImGui::GetIO();
    static double s_LastTime = ImGui::GetTime();
    double currentTime = ImGui::GetTime();
    float deltaTime = static_cast<float>(currentTime - s_LastTime);
    s_LastTime = currentTime;

    if (!io.WantCaptureKeyboard && m_MainCam.GetPtr())
    {
        float mod = 1.f;
        float mov_mod = 4.f;
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
        {
            mod = 4.f;
            mov_mod = 10.f;
        }

        float moveSpeed = 10.0f * deltaTime * mov_mod;
        float rotSpeed = 8.0f * deltaTime * mod;

        auto* camTransform = m_MainCam.GetPtr()->GetTransform();
        glm::vec3 pos = camTransform->GetPosition();
        glm::vec3 rot = camTransform->GetEulerAnglesDegrees();

        glm::vec3 rotRad = glm::radians(rot);
        glm::quat orientation = glm::quat(rotRad);

        glm::vec3 forward = orientation * glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 right = orientation * glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 up = orientation * glm::vec3(0.0f, 1.0f, 0.0f);

        if (ImGui::IsKeyDown(ImGuiKey_W)) pos += forward * moveSpeed;
        if (ImGui::IsKeyDown(ImGuiKey_S)) pos -= forward * moveSpeed;
        if (ImGui::IsKeyDown(ImGuiKey_A)) pos -= right * moveSpeed;
        if (ImGui::IsKeyDown(ImGuiKey_D)) pos += right * moveSpeed;
        if (ImGui::IsKeyDown(ImGuiKey_Q)) pos -= up * moveSpeed;
        if (ImGui::IsKeyDown(ImGuiKey_E)) pos += up * moveSpeed;

        if (ImGui::IsKeyDown(ImGuiKey_I)) rot.x -= rotSpeed;
        if (ImGui::IsKeyDown(ImGuiKey_K)) rot.x += rotSpeed;
        if (ImGui::IsKeyDown(ImGuiKey_J)) rot.y -= rotSpeed;
        if (ImGui::IsKeyDown(ImGuiKey_L)) rot.y += rotSpeed;

        camTransform->SetPosition(pos);
        camTransform->SetEulerAnglesDegrees(rot);
    }
}

void AnitoTracer_App::HandleObjectPicking(const SwapChainDesc& SCDesc, const RenderData& renderData)
{
    uint32_t pickedID = ObjectPicker::ProcessObjectPicking(renderData, SCDesc.Width, SCDesc.Height);

    if (pickedID != 0) {
        HierarchyObject* selectedObj = HierarchyObject::getById(pickedID);
        if (selectedObj) {
            std::cout << "Clicked on Model owned by: " << selectedObj->GetName() << std::endl;
            GUIManager::GetInstance().SetSelectedObject(selectedObj);
        }
    }
}

void AnitoTracer_App::HandleInitializeEvent(const gbe::EventArgs* args)
{
    std::cout << "Engine Initialized" << std::endl;
}

void AnitoTracer_App::HandleRenderStartEvent(const gbe::EventArgs * args)
{
    //Avoid Spam- uncomment if necessary desu
    //std::cout << "Engine Render Start" << std::endl;
}

void AnitoTracer_App::HandleRenderEndEvent(const gbe::EventArgs * args)
{
    //Avoid Spam- uncomment if necessary desu
    //std::cout << "Engine Render End" << std::endl;
}

void AnitoTracer_App::HandleWindowResizeEvent(const WindowResizeArgs* args)
{
    std::cout << "EVENT_ONWINDOWRESIZE: SwapChain resized to "
        << args->width << "x" << args->height << "!\n";

    RendererManager::GetInstance().OnResize(args->width, args->height);
}

void AnitoTracer_App::Shutdown()
{
	HierarchyManager::GetInstance().Clear();

    if (m_pImmediateContext) m_pImmediateContext->Flush();
    if (m_pDevice) m_pDevice->IdleGPU();

    Diligent::ShaderManager::GetInstance().Shutdown();
    GUIManager::GetInstance().Shutdown();
    RendererManager::GetInstance().Shutdown(); // Shutdown the manager

    m_pSwapChain.Release();
    m_pImmediateContext.Release();
    m_pDevice.Release();
}