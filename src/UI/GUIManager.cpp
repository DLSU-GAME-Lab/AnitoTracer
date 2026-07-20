#include "GUIManager.hpp"

// Initialize ImGui context and Diligent renderer
void Diligent::GUIManager::Initialize(IRenderDevice* pDevice, const SwapChainDesc& SCDesc, NativeWindow nativeWindow)
{
    if (m_pImGuiRenderer) return; // Already initialized

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

    ImGui::StyleColorsDark();
}

// Begin a new ImGui frame
void Diligent::GUIManager::NewFrame(Uint32 width, Uint32 height, SURFACE_TRANSFORM transform)
{
    if (!m_pImGuiRenderer) return;
    m_pImGuiRenderer->NewFrame(width, height, transform);
}

// Draw the main layout (Menu bar and Dockspace)
void Diligent::GUIManager::DrawUI(bool& appRunning)
{
    if (!m_pImGuiRenderer) return;


    // 2. Main Menu Bar
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
                appRunning = false;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Windows"))
        {
            ImGui::MenuItem("Properties", NULL, &m_ShowPropertiesWindow);
            ImGui::MenuItem("Console", NULL, &m_ShowConsoleWindow);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // 3. Render dockable windows based on state
    if (m_ShowPropertiesWindow)
    {
        ImGui::Begin("Properties", &m_ShowPropertiesWindow);

        if (m_pCamera)
        {
            ImGui::Text("Camera Properties");
            ImGui::Separator();

            // Position
            glm::vec3 position = m_pCamera->GetPosition();
            float posArray[3] = { position.x, position.y, position.z };
            if (ImGui::DragFloat3("Position##camera", posArray, 0.1f))
            {
                m_pCamera->SetPosition(glm::vec3(posArray[0], posArray[1], posArray[2]));
                m_pCamera->UpdateViewMatrix();
            }

            // Target
            glm::vec3 target = m_pCamera->GetTarget();
            float targetArray[3] = { target.x, target.y, target.z };
            if (ImGui::DragFloat3("Target##camera", targetArray, 0.1f))
            {
                m_pCamera->SetTarget(glm::vec3(targetArray[0], targetArray[1], targetArray[2]));
                m_pCamera->UpdateViewMatrix();
            }

            // Up Vector
            glm::vec3 up = m_pCamera->GetUp();
            float upArray[3] = { up.x, up.y, up.z };
            if (ImGui::DragFloat3("Up##camera", upArray, 0.01f))
            {
                m_pCamera->SetUp(glm::vec3(upArray[0], upArray[1], upArray[2]));
                m_pCamera->UpdateViewMatrix();
            }

            ImGui::Separator();
            ImGui::Text("Projection Settings");

            // FOV
            float fov = m_pCamera->GetFOV();
            if (ImGui::SliderFloat("FOV (degrees)", &fov, 1.0f, 180.0f))
            {
                m_pCamera->SetFOV(fov);
                m_pCamera->UpdateProjectionMatrix();
            }

            // Aspect Ratio
            float aspect = m_pCamera->GetAspect();
            if (ImGui::SliderFloat("Aspect Ratio", &aspect, 0.1f, 10.0f))
            {
                m_pCamera->SetAspect(aspect);
                m_pCamera->UpdateProjectionMatrix();
            }

            // Near Plane
            float nearZ = m_pCamera->GetNearPlane();
            if (ImGui::SliderFloat("Near Plane", &nearZ, 0.01f, 10.0f))
            {
                m_pCamera->SetNearPlane(nearZ);
                m_pCamera->UpdateProjectionMatrix();
            }

            // Far Plane
            float farZ = m_pCamera->GetFarPlane();
            if (ImGui::SliderFloat("Far Plane", &farZ, 100.0f, 10000.0f))
            {
                m_pCamera->SetFarPlane(farZ);
                m_pCamera->UpdateProjectionMatrix();
            }
        }
        else
        {
            ImGui::Text("No camera assigned");
        }

        ImGui::End();
    }

    if (m_ShowConsoleWindow)
    {
        ImGui::Begin("Console", &m_ShowConsoleWindow);
        ImGui::Text("This is a dockable console window.");
        ImGui::End();
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
