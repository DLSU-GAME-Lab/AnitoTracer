#include "UIManager.h"

#include <glm/gtc/type_ptr.hpp>

#include "AssetExplorerScreen.h"
#include "ConsoleScreen.h"
#include "From-GDGRAP2/RTConfig.h"
#include "MenuScreen.h"
#include "From-GDGRAP2/Debug.h"
#include "HierarchyScreen.h"
#include "InspectorScreen.h"
#include "MaterialEditorScreen.h"
#include "PlaybackScreen.h"
#include "ProfilerScreen.h"
#include "RayTracer.hpp"
#include "SettingsScreen.h"
#include "ProjectScreen.h"
#include "From-GDGRAP2/TransformHistory.h"
#include "ViewportScreen.h"
#include "Engine/CameraSystem/CameraManager.h"
#include "From-GDGRAP2/ModelManager.h"
#include "ImGuizmo.h"
#include "imgui_freetype.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "Utilities/Exception.hpp"
#include "Utilities/FileUtils.h"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/FrameBuffer.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Vulkan/SingleTimeCommands.hpp"
#include "Vulkan/Surface.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/Window.hpp"
#include "IconsMaterialDesign.h"
#include "EditorTheme.hpp"
#include "Utilities/DragAndDrop/DragAndDropUtils.h"

// For DragDrop from Windows
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#include <shellapi.h> // Required for DragAcceptFiles
#include "StateManagement/CommandManager.hpp"
#include "StateManagement/ConcreteCommands/InspectorCommands.hpp"
#include "HotkeySystem/HotkeySystem.hpp"
#include "StateManagement/ConcreteCommands/GUICommands.hpp"
#include "StateManagement/ConcreteCommands/HierarchyCommands.hpp"

#include "glm/fwd.hpp"
#include "Assets/Model.hpp"
#include "From-GDGRAP2/MaterialLibrary.h"
#include <glm/gtx/matrix_decompose.hpp>


bool UIManager::isStartup = true;
bool UIManager::isHidingUI = false;

UIManager* UIManager::sharedInstance = nullptr;

TransformState UIManager::gizmoBeforeState = {};

// For WndProc subclassing
LRESULT CALLBACK DragDropWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
WNDPROC imguiWndproc;

namespace
{
	void CheckVulkanResultCallback(const VkResult err)
	{
		if (err != VK_SUCCESS)
		{
			Throw(std::runtime_error(std::string("ImGui Vulkan error (") + Vulkan::ToString(err) + ")"));
		}
	}
}

UIManager::UIManager()
{
	HotkeySystem::getInstance()->addListener(this);
}

UIManager::~UIManager()
{
	uiConfig->currentGizmoOperation = m_currentGizmoOperation;
	HotkeySystem::getInstance()->removeListener(this);

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

UIManager* UIManager::getInstance()
{
	return sharedInstance;
}

void UIManager::initialize(Vulkan::CommandPool* commandPool, const Vulkan::SwapChain* swapChain,
	const Vulkan::DepthBuffer* depthBuffer, UserSettings* userSettings, UIConfig* uiConfig)
{

	sharedInstance = new UIManager();

	const auto& device = swapChain->Device();
	//sharedInstance->device = &swapChain->Device();
	const auto& window = device.Surface().Instance().Window();
	sharedInstance->userSettings = userSettings;
	sharedInstance->swapChain = swapChain;
	sharedInstance->commandPool = commandPool;
	sharedInstance->uiConfig = uiConfig;
	sharedInstance->m_currentGizmoOperation = uiConfig->currentGizmoOperation;

	// Initialise descriptor pool and render pass for ImGui.
	const std::vector<Vulkan::DescriptorBinding> descriptorBindings =
	{
		{0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0},
	};
	sharedInstance->descriptorPool.reset(new Vulkan::DescriptorPool(device, descriptorBindings, 10));
	sharedInstance->renderPass.reset(
		new Vulkan::RenderPass(
			*swapChain,
			*depthBuffer,
			VK_ATTACHMENT_LOAD_OP_LOAD,
			VK_ATTACHMENT_LOAD_OP_LOAD));

	// Initialise ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Initialise ImGui GLFW adapter
	if (!ImGui_ImplGlfw_InitForVulkan(window.Handle(), true))
	{
		Throw(std::runtime_error("failed to initialise ImGui GLFW adapter"));
	}

	HWND hWnd = glfwGetWin32Window(window.Handle());
	imguiWndproc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
	SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)DragDropWndProc);
	DragAcceptFiles(hWnd, TRUE);

	// Initialise ImGui Vulkan adapter
	ImGui_ImplVulkan_InitInfo vulkanInit = {};
	vulkanInit.Instance = device.Surface().Instance().Handle();
	vulkanInit.PhysicalDevice = device.PhysicalDevice();
	vulkanInit.Device = device.Handle();
	vulkanInit.QueueFamily = device.GraphicsFamilyIndex();
	vulkanInit.Queue = device.GraphicsQueue();
	vulkanInit.PipelineCache = nullptr;
	vulkanInit.PipelineInfoMain.RenderPass = sharedInstance->renderPass->Handle();
	vulkanInit.DescriptorPool = sharedInstance->descriptorPool->Handle();
	vulkanInit.MinImageCount = swapChain->MinImageCount();
	vulkanInit.ImageCount = static_cast<uint32_t>(swapChain->Images().size());
	vulkanInit.Allocator = nullptr;
	vulkanInit.CheckVkResultFn = CheckVulkanResultCallback;

	if (!ImGui_ImplVulkan_Init(&vulkanInit))
	{
		Throw(std::runtime_error("failed to initialise ImGui vulkan adapter"));
	}

	sharedInstance->currentLayoutPath = ApplicationConfig::IMGUI_INI_PATH;
	auto& io = ImGui::GetIO();

	io.IniFilename = sharedInstance->currentLayoutPath.c_str();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	// Loads Default UI Layout (from imgui_default_layout.ini)
	//ImGui::LoadIniSettingsFromDisk(ApplicationConfig::DEFAULT_UI_LAYOUT_PATH.c_str());

	// Window scaling and style.
	const auto scaleFactor = window.ContentScale();

	setupImGuiStyle();
	//ImGui::StyleColorsDark();
	ImGui::GetStyle().ScaleAllSizes(scaleFactor);

	// Upload ImGui fonts (use ImGuiFreeType for better font rendering, see https://github.com/ocornut/imgui/tree/master/misc/freetype).
	io.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());

	if (!io.Fonts->AddFontFromFileTTF(FileUtils::getAssetsFolderPath().generic_string().append("/fonts/Cousine-Regular.ttf").data(), 13 * scaleFactor))
	{
		Throw(std::runtime_error("failed to load Cousine font"));
	}

	/* 1 */
	auto defaultFont = io.Fonts->AddFontFromFileTTF(FileUtils::getAssetsFolderPath().generic_string().append("/fonts/" + DarkTheme.EDITOR_FONT).data(), 18 * scaleFactor);
	if (!defaultFont)
	{
		Throw(std::runtime_error("failed to load Inter font"));
	}

	io.FontDefault = defaultFont;

	/* 2 */
	io.Fonts->AddFontFromFileTTF(FileUtils::getAssetsFolderPath().generic_string().append("/fonts/" + DarkTheme.EDITOR_FONT).data(), 14 * scaleFactor);

	static const ImWchar iconRanges[] = { ICON_MIN_MD, ICON_MAX_16_MD, 0 };

	ImFontConfig* iconFontConfig = new ImFontConfig();
	iconFontConfig->MergeMode = false;
	iconFontConfig->PixelSnapH = true;
	iconFontConfig->GlyphOffset =ImVec2(1.0f, 0.0f);

	/* 3 */
	io.Fonts->AddFontFromFileTTF(FileUtils::getAssetsFolderPath().generic_string().append("/fonts/" + DarkTheme.ICON_FONT).data(), 20 * scaleFactor, iconFontConfig, iconRanges);

	ImFontConfig* iconFontConfig2 = new ImFontConfig();
	iconFontConfig2->MergeMode = false;
	iconFontConfig2->PixelSnapH = true;
	iconFontConfig2->GlyphOffset = ImVec2(1.0f, 3.0f);

	/* 4 */
	io.Fonts->AddFontFromFileTTF(FileUtils::getAssetsFolderPath().generic_string().append("/fonts/" + DarkTheme.ICON_FONT).data(), 14 * scaleFactor, iconFontConfig2, iconRanges);

	ImFontConfig* iconFontConfig3 = new ImFontConfig();
	iconFontConfig3->MergeMode = false;
	iconFontConfig3->PixelSnapH = true;
	iconFontConfig3->GlyphOffset = ImVec2(0.0f, 0.0f);

	/* 5 */
	sharedInstance->iconFont = io.Fonts->AddFontFromFileTTF(FileUtils::getAssetsFolderPath().generic_string().append("/fonts/" + DarkTheme.ICON_FONT).data(), 14 * scaleFactor, iconFontConfig3, iconRanges);

	Vulkan::SingleTimeCommands::Submit(*commandPool, [](VkCommandBuffer commandBuffer)
		{
			// IMGUI_IMPL_VULKAN NOW SUPPORTS DYNAMIC FONTS OUT OF BOX 
		});

	sharedInstance->initializeUI();

}

void UIManager::initializeUI()
{
	//loadLayout();q

	//populate UI table
	//UIs that will show during runtime
	const std::shared_ptr<MenuScreen> menuScreen = std::make_shared<MenuScreen>();
	this->uiTable[UINames::MENU_SCREEN] = menuScreen;
	this->uiList.push_back(menuScreen);

	const std::shared_ptr<HierarchyScreen> hierarchyScreen = std::make_shared<HierarchyScreen>();
	this->uiTable[UINames::HIERARCHY_SCREEN] = hierarchyScreen;
	hierarchyScreen->setEnabled(this->uiConfig->isHierarchyEnabled);
	this->uiList.push_back(hierarchyScreen);

	//PROJECTS/FILE EXPLORER WINDOW
	//=============================================================================================
	/*
	const std::shared_ptr<> menuScreen = std::make_shared<MenuScreen>();
	this->uiTable[UINames::MENU_SCREEN] = menuScreen;
	this->uiList.push_back(menuScreen);
	*/
	//=============================================================================================

	const std::shared_ptr<InspectorScreen> inspectorScreen = std::make_shared<InspectorScreen>();
	this->uiTable[UINames::INSPECTOR_SCREEN] = inspectorScreen;
	inspectorScreen->setEnabled(this->uiConfig->isInspectorEnabled);
	inspectorScreen->setUniformScalingEnabled(this->uiConfig->inspectorUniformScaling);
	this->uiList.push_back(inspectorScreen);

	const std::shared_ptr<ProjectScreen> projectScreen = std::make_shared<ProjectScreen>();
	this->uiTable[UINames::PROJECT_SCREEN] = projectScreen;
	projectScreen->setEnabled(this->uiConfig->isProjectEnabled);
	this->uiList.push_back(projectScreen);

	const std::shared_ptr<ConsoleScreen> consoleScreen = std::make_shared<ConsoleScreen>();
	this->uiTable[UINames::CONSOLE_SCREEN] = consoleScreen;
	consoleScreen->setEnabled(this->uiConfig->isConsoleEnabled);
	consoleScreen->setText(this->uiConfig->consoleTextLog, this->uiConfig->consoleLogCount);
	this->uiList.push_back(consoleScreen);
	Debug::assignConsole(consoleScreen);

	const std::shared_ptr<ProfilerScreen> profilerScreen = std::make_shared<ProfilerScreen>();
	this->uiTable[UINames::PROFILER_SCREEN] = profilerScreen;
	this->uiList.push_back(profilerScreen);
	profilerScreen->setEnabled(this->uiConfig->isProfilerEnabled);
	sharedInstance->profilerActive = this->uiConfig->isProfilerEnabled;

	//std::shared_ptr<gdeng03::PlaybackScreen> playbackScreen = std::make_shared<gdeng03::PlaybackScreen>();
	//this->uiTable[uiNames.PLAYBACK_SCREEN] = playbackScreen;
	//this->uiList.push_back(playbackScreen);

	// nawt working yet lol!
	const std::shared_ptr<gdeng03::MaterialEditorScreen> materialEditorScreen = std::make_shared<gdeng03::MaterialEditorScreen>();
	this->uiTable[UINames::MATERIAL_EDITOR_SCREEN] = materialEditorScreen;
	materialEditorScreen->setEnabled(this->uiConfig->isMaterialEditorEnabled);
	this->uiList.push_back(materialEditorScreen);

	const std::shared_ptr<SettingsScreen> settingsScreen = std::make_shared<SettingsScreen>();
	this->uiTable[UINames::SETTINGS_SCREEN] = settingsScreen;
	this->uiList.push_back(settingsScreen);
	settingsScreen->setEnabled(this->uiConfig->isSettingsEnabled);
	sharedInstance->settingsActive = this->uiConfig->isSettingsEnabled;

	// std::shared_ptr<AssetExplorerScreen> assetExplorerScreen = std::make_shared<AssetExplorerScreen>();
	// this->uiTable[uiNames.ASSET_EXPLORER_SCREEN] = assetExplorerScreen;
	// this->uiList.push_back(assetExplorerScreen);

	// nawt working yet lol!
	// std::shared_ptr<ViewportScreen> viewportScreen = std::make_shared<ViewportScreen>();
	// this->uiTable[uiNames.VIEWPORT_SCREEN] = viewportScreen;
	// this->uiList.push_back(viewportScreen);

	// MaterialScreen* materialScreen = new MaterialScreen();
	// this->uiTable[uiNames.MATERIAL_SCREEN] = materialScreen;
	// this->uiList.push_back(materialScreen);
	// materialScreen->SetEnabled(false);

	// save and load the current layout to avoid resetting randomly

	for (const auto& i : this->uiList)
	{
		if (i->name != UINames::MENU_SCREEN)
			i->setEnabled(!isHidingUI);

		if (i->name == UINames::SETTINGS_SCREEN)
			i->setEnabled(settingsActive);
		
		if (i->name == UINames::PROFILER_SCREEN)
			i->setEnabled(profilerActive);

	}

	// Debug::Log("Startup is " + (isStartup ? std::string("true") : std::string("false")));
	//
	if (isStartup)
	{
		// 	Debug::Log("UI first startup");
		String filename = ApplicationConfig::IMGUI_INI_SAVE_PATH + "Default.ini";
		sharedInstance->currentLayoutPath = filename;
		loadLayout();
	}
	else
		loadDynamicLayout();

	Debug::Log("Initialized UIs!");
}

void UIManager::saveLayout()
{
	ImGui::SaveIniSettingsToDisk(ApplicationConfig::IMGUI_INI_PATH.c_str());
}

void UIManager::saveLayout(String name)
{
	String filename = ApplicationConfig::IMGUI_INI_SAVE_PATH + name + ".ini";

	//COMMENTED FOR NOW WHILE SETTING UP PRESETS
	//if (name != "Default" || name != "Tall" || name != "Wide")
	ImGui::SaveIniSettingsToDisk(filename.c_str());
}

void UIManager::saveDefaultLayout()
{
	ImGui::SaveIniSettingsToDisk(ApplicationConfig::DEFAULT_UI_LAYOUT_PATH.c_str());
}

void UIManager::saveDynamicLayout()
{
	Debug::Log("Saving dynamic layout to " + ApplicationConfig::IMGUI_DYNAMIC_INI_PATH);
	ImGui::SaveIniSettingsToDisk(ApplicationConfig::IMGUI_DYNAMIC_INI_PATH.c_str());
}

void UIManager::loadDynamicLayout()
{
	isLoadingDynamicLayout = true;
}

void UIManager::loadLayout()
{
	isLoadingLayout = true;
}

void UIManager::loadLayoutFromFile()
{
	String filename;
	FileUtils::getLayoutFilePath(sharedInstance->currentLayoutPath, filename);
	isLoadingLayout = true;
}

void UIManager::loadPresetLayout(int index)
{
	String filename;
	switch (index) {
		case 0:
			filename = ApplicationConfig::IMGUI_INI_SAVE_PATH + "Default.ini";
			sharedInstance->currentLayoutPath = filename;
			isLoadingLayout = true;
			break;
		case 1:
			filename = ApplicationConfig::IMGUI_INI_SAVE_PATH + "Tall.ini";
			sharedInstance->currentLayoutPath = filename;
			isLoadingLayout = true;
			break;
		case 2:
			filename = ApplicationConfig::IMGUI_INI_SAVE_PATH + "Wide.ini";
			sharedInstance->currentLayoutPath = filename;
			isLoadingLayout = true;
			break;
		default:
			break;
	}
}

void UIManager::resetLayout()
{
	isResettingLayout = true;
}

void UIManager::render(VkCommandBuffer commandBuffer, const Vulkan::FrameBuffer& frameBuffer,
	const Statistics& statistics)
{
	if (isLoadingLayout)
	{
		ImGui::LoadIniSettingsFromDisk(sharedInstance->currentLayoutPath.c_str());
		isLoadingLayout = false;
		isStartup = false;
	}
	else if (isResettingLayout)
	{
		ImGui::LoadIniSettingsFromDisk(ApplicationConfig::DEFAULT_UI_LAYOUT_PATH.c_str());
		isResettingLayout = false;
	}
	else if (isLoadingDynamicLayout)
	{
		Debug::Log("Loading dynamic layout");
		ImGui::LoadIniSettingsFromDisk(ApplicationConfig::IMGUI_DYNAMIC_INI_PATH.c_str());
		isLoadingDynamicLayout = false;
	}

	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplVulkan_NewFrame();
	ImGui::NewFrame();

	//ImGui::ShowMetricsWindow();
	//ImGui::ShowDemoWindow();

	//Allow docking inside the main viewport 
	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
	
	// Draw the rest of your UI first.
	//UIManager::getInstance()->drawAllUI();
	drawAllUI();
	//DrawSettings();
	drawOverlay(statistics);

	DragAndDropUtils::attachModelInstantiateTargetToViewport(ImGui::GetMainViewport());

	//Start ImGuizmo frame.
	if (ModelManager::getInstance()->getSelectedObject() != nullptr)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_LeftCtrl)) isCTRLHeld = true;

		auto selectedObject = ModelManager::getInstance()->getSelectedObject();
		bool isUsingGizmoNow = ImGuizmo::IsUsing();

		if (!wasUsingGizmoLastFrame) // set gizmo origin
		{
			this->gizmoModelMatrix = selectedObject->getWorldMatrix();
		}

		// Store the 'before' state when manipulation starts
		if (isUsingGizmoNow && !wasUsingGizmoLastFrame)
		{
			gizmoWasManipulated = false;

			gizmoBeforeState = {
				selectedObject->getLocalPosition(),
				selectedObject->getLocalRotation(),
				selectedObject->getLocalScale()
			};
		}

		ImGuizmo::BeginFrame();
		ImGuizmo::SetRect(0, 0, swapChain->Extent().width, swapChain->Extent().height);

		glm::mat4 viewMatrix = CameraManager::getInstance()->getActiveCamera()->GetView();
		glm::mat4 projMatrix = CameraManager::getInstance()->getActiveCamera()->GetProjection();

		if (ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), glm::value_ptr(projMatrix),
			m_currentGizmoOperation, ImGuizmo::WORLD, glm::value_ptr(gizmoModelMatrix)))
		{
			gizmoWasManipulated = true;

			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(gizmoModelMatrix), translation, rotation, scale);

			auto inspector = dynamic_pointer_cast<InspectorScreen>(sharedInstance->findUIByName(UINames::INSPECTOR_SCREEN));

			// Uniform Scaling
			// Check from InspectorWindow if uniform scaling is enabled
			if (inspector->IsUniformScalingEnabled() || (m_currentGizmoOperation == ImGuizmo::SCALE && isCTRLHeld))
			{
				if (scale[0] != gizmoBeforeState.scale.x) // check which value was manipulated
				{
					float ratio = scale[0] / gizmoBeforeState.scale.x; //New / Old scale
					scale[1] = gizmoBeforeState.scale.y * ratio;
					scale[2] = gizmoBeforeState.scale.z * ratio;
				}

				else if (scale[1] != gizmoBeforeState.scale.y)
				{
					float ratio = scale[1] / gizmoBeforeState.scale.y; 
					scale[0] = gizmoBeforeState.scale.x * ratio;
					scale[2] = gizmoBeforeState.scale.z * ratio;
				}

				else if (scale[2] != gizmoBeforeState.scale.z)
				{
					float ratio = scale[2] / gizmoBeforeState.scale.z;
					scale[0] = gizmoBeforeState.scale.x * ratio;
					scale[1] = gizmoBeforeState.scale.y * ratio;
				}
			}

			if (!RayTracer::getInstance()->getUserSettings().IsRayTraced) // For Rasterized Mode
			{
				selectedObject->setLocalPosition(translation[0], translation[1], translation[2]);
				selectedObject->setLocalRotation(rotation[0], rotation[1], rotation[2]);
				selectedObject->setLocalScale(scale[0], scale[1], scale[2]);
			}
		}

		if (!isUsingGizmoNow && wasUsingGizmoLastFrame) // Stop Manipulate
		{
			if (RayTracer::getInstance()->getUserSettings().IsRayTraced)
			{
				glm::mat4 newLocalMatrix;

				if (selectedObject->getParent())
				{
					glm::mat4 parentWorldInverse = glm::inverse(selectedObject->getParent()->getWorldMatrix()); 
					newLocalMatrix = parentWorldInverse * gizmoModelMatrix; // gizmo is in new world space
				}
				else
				{
					newLocalMatrix = gizmoModelMatrix;
				}

				// Decompose to update local position, rotation, scale
				glm::vec3 skew;
				glm::vec4 perspective;
				glm::quat rotationQuat;
				glm::vec3 newLocalScale;
				glm::vec3 newLocalPosition;
				glm::vec3 newLocalRotation;
				glm::decompose(newLocalMatrix, newLocalScale, rotationQuat,newLocalPosition, skew, perspective);
				newLocalRotation = glm::degrees(glm::eulerAngles(rotationQuat));

				TransformState afterState = {
				newLocalPosition,
				newLocalRotation,
				newLocalScale
				};

				if (gizmoBeforeState != afterState)
				{
					/* Also Records Actions */
					CommandManager::getInstance()->executeCommand(new TransformObjectCommand(
						selectedObject, 
						gizmoBeforeState.position, gizmoBeforeState.rotation, gizmoBeforeState.scale,
						afterState.position, afterState.rotation, afterState.scale
					));
				}
			}
		}

		wasUsingGizmoLastFrame = isUsingGizmoNow;
	}

	ImGui::Render();

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = sharedInstance->renderPass->Handle();
	renderPassInfo.framebuffer = frameBuffer.Handle();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = sharedInstance->renderPass->SwapChain().Extent();
	//renderPassInfo.renderArea.extent = VkExtent2D(1920, 1080);
	renderPassInfo.clearValueCount = 0;
	renderPassInfo.pClearValues = nullptr;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	vkCmdEndRenderPass(commandBuffer);

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
	//ImGui::UpdatePlatformWindows();
	//ImGui::RenderPlatformWindowsDefault();

	TransformHistory::getInstance().resetUndoRedoFlag();

	this->detectAndRecordLayoutChanges(); // detect layout changes for recording after docking is set
}

void UIManager::drawOverlay(const Statistics& statistics) const
{
	if (!settings()->ShowOverlay)
	{
		return;
	}

	//ImGui::SetNextWindowBgAlpha(0.3f); // Transparent background

	if (ImGui::Begin("Statistics", &settings()->ShowOverlay, UISettings::GlobalWindowFlags))
	{
		ImGui::Text("Statistics (%dx%d):", statistics.framebufferSize.width, statistics.framebufferSize.height);
		ImGui::Separator();
		ImGui::Text("Frame rate: %.1f fps", statistics.frameRate);
		ImGui::Text("Primary ray rate: %.2f Gr/s", statistics.rayRate);
		ImGui::Text("Accumulated samples:  %u", statistics.totalSamples);
	}
	ImGui::End();
}

void UIManager::reset()
{
	saveDynamicLayout();
	//ImGui::SaveIniSettingsToDisk(ApplicationConfig::DEFAULT_UI_LAYOUT_PATH.c_str());
	delete sharedInstance;
}

void UIManager::drawAllUI() const
{
	for (const auto& i : this->uiList)
	{
		if (i->enabled)
			i->drawUI();
	}
}

bool UIManager::getEnabled(const std::string& name)
{
	if (name == "Statistics")
	{
		return userSettings->ShowOverlay;
	}

	if (!this->uiTable[name])
		return false;

	return this->uiTable[name]->enabled;
}

void UIManager::setEnabled(const String& uiName, const bool flag)
{
	if (uiName == "Statistics")
	{
		userSettings->ShowOverlay = flag;
		return;
	}

	if (this->uiTable[uiName] != nullptr)
	{
		this->uiTable[uiName]->setEnabled(flag);
	}
}

void UIManager::toggleEnabled(const String& uiName)
{
	if (uiName == "Statistics")
	{
		userSettings->ShowOverlay = !userSettings->ShowOverlay;
		return;
	}

	if (this->uiTable[uiName] != nullptr)
	{
		this->uiTable[uiName]->toggleEnabled();
		this->m_windowWasToggled = true;
	}
}

std::shared_ptr<AUIScreen> UIManager::findUIByName(const String& uiName)
{
	if (this->uiTable[uiName] != nullptr)
	{
		return this->uiTable[uiName];
	}

	return nullptr;
}

void UIManager::toggleAllUI()
{
	for (const auto& i : this->uiList)
	{
		if (i->name != UINames::MENU_SCREEN)
			i->setEnabled(isHidingUI);
	}

	isHidingUI = !isHidingUI;
	userSettings->ShowOverlay = !userSettings->ShowOverlay;
}

void UIManager::hideAllUI() const
{
	for (const auto& i : this->uiList)
	{
		if (i->name != UINames::MENU_SCREEN)
			i->setEnabled(false);
	}
}

void UIManager::showAllUI() const
{
	for (const auto& i : this->uiList)
	{
		if (i->name != UINames::MENU_SCREEN)
			i->setEnabled(true);
	}
}

void UIManager::FreeDescriptor(VkDescriptorSet& descriptorset)
{
	const auto& device = swapChain->Device();

	// Initialise descriptor pool and render pass for ImGui.
	const std::vector<Vulkan::DescriptorBinding> descriptorBindings =
	{
		{0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0},
	};
	sharedInstance->descriptorPool.reset(new Vulkan::DescriptorPool(device, descriptorBindings, 10));

}

bool UIManager::wantsToCaptureKeyboard()
{
	return ImGui::GetIO().WantCaptureKeyboard;
}

bool UIManager::wantsToCaptureMouse()
{
	return ImGui::GetIO().WantCaptureMouse;
}

ImFont* UIManager::GetIconFont()
{
	return this->iconFont;
}

void UIManager::OnActionPressed(Hotkey::Action action)
{
	if (ModelManager::getInstance()->getSelectedObject() == nullptr) return;
	if (CameraManager::getInstance()->getActiveCamera()->getCurrentMoveMode() != Camera::CameraMoveMode::NONE) return;

	using Action = Hotkey::Action;

	if (action == Action::SceneTool_Move) m_currentGizmoOperation = ImGuizmo::TRANSLATE;
	if (action == Action::SceneTool_Rotate) m_currentGizmoOperation = ImGuizmo::ROTATE;
	if (action == Action::SceneTool_Scale) m_currentGizmoOperation = ImGuizmo::SCALE;
	if (action == Action::SceneTool_Transform) m_currentGizmoOperation = ImGuizmo::UNIVERSAL;
	if (action == Action::SceneTool_Cycle)
	{
		switch (m_currentGizmoOperation)
		{
		case ImGuizmo::TRANSLATE: m_currentGizmoOperation = ImGuizmo::ROTATE; break;
		case ImGuizmo::ROTATE: m_currentGizmoOperation = ImGuizmo::SCALE; break;
		case ImGuizmo::SCALE: m_currentGizmoOperation = ImGuizmo::UNIVERSAL; break;
		case ImGuizmo::UNIVERSAL:
		default: m_currentGizmoOperation = ImGuizmo::TRANSLATE; break;
		}
	}
}

void UIManager::detectAndRecordLayoutChanges()
{
	/* we do this so we don't record layout changes due to other docking spaces stretch */
	if (m_windowWasToggled)
	{
		this->m_windowWasToggled = false;
		this->m_scheduleRecording = false;
		return;
	}

	if (this->m_scheduleNextFrame) // need to record next frame to save docking changes
	{
		this->m_scheduleNextFrame = false;
		return;
	}

	if (this->m_scheduleRecording)
	{
		this->m_currentLayoutSnapshot = GetIniDump();
		if (this->compareStrippedIni(this->m_currentLayoutSnapshot))
		{
			CommandManager::getInstance()->executeCommand(new ModifyLayoutCommand(this->m_lastLayoutSnapshot, this->m_currentLayoutSnapshot));
		}
		this->m_scheduleRecording = false;
	}
}

void UIManager::onLMBPressed()
{
	this->m_lastLayoutSnapshot = GetIniDump();
}

void UIManager::onLMBReleased()
{
	this->m_scheduleRecording = this->m_scheduleNextFrame = true;
}

std::string UIManager::GetIniDump() const
{
	size_t size = 0;
	const char* data = ImGui::SaveIniSettingsToMemory(&size);
	return std::string(data, size);
}

std::string UIManager::stripIni(std::string iniString)
{
	std::string filtered;
	filtered.reserve(iniString.size());

	size_t start = 0;

	for (size_t i = 0; i <= iniString.size(); i++)
	{
		if (i == iniString.size() || iniString[i] == '\n')
		{
			size_t len = i - start;
			std::string line = iniString.substr(start, len);

			// skip collapsed line
			if (line.find("Collapsed=") == std::string::npos)
			{
				filtered += line + "\n";
			}

			start = i + 1;
		}
	}

	return filtered;
}

bool UIManager::compareStrippedIni(std::string currentIniState)
{
	auto newState = this->stripIni(currentIniState);
	auto oldState = this->stripIni(this->m_lastLayoutSnapshot);

	return oldState != newState;
}

void UIManager::setupImGuiStyle()
{
	// Tokyo Night Storm style from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.6000000238418579f;
	style.WindowPadding = ImVec2(15.0f, 10.10000038146973f);
	style.WindowRounding = 0.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(32.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;
	style.ChildRounding = 5.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 5.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(4.0f, 3.0f);
	style.FrameRounding = 4.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(8.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 12.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 10.0f;
	style.TabRounding = 4.0f;
	style.TabBorderSize = 0.0f;
	//style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Left;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
	style.WindowMenuButtonPosition = ImGuiDir_None;

	style.Colors[ImGuiCol_Text] = DarkTheme.TEXT;
	style.Colors[ImGuiCol_TextDisabled] = DarkTheme.TEXT_DISABLED;
	
	style.Colors[ImGuiCol_WindowBg] = DarkTheme.WINDOW_BG;
	style.Colors[ImGuiCol_ChildBg] = DarkTheme.CHILD_BG;
	style.Colors[ImGuiCol_PopupBg] = DarkTheme.POPUP_BG;
	
	style.Colors[ImGuiCol_Border] = DarkTheme.BORDER;
	style.Colors[ImGuiCol_BorderShadow] = DarkTheme.BORDER_SHADOW;

	style.Colors[ImGuiCol_FrameBg] = DarkTheme.FRAME_BG;
	style.Colors[ImGuiCol_FrameBgHovered] = DarkTheme.FRAME_BG_HOVERED;
	style.Colors[ImGuiCol_FrameBgActive] = DarkTheme.FRAME_BG_ACTIVE;
	
	style.Colors[ImGuiCol_TitleBg] = DarkTheme.TITLE_BG;
	style.Colors[ImGuiCol_TitleBgActive] = DarkTheme.TITLE_BG_ACTIVE;
	style.Colors[ImGuiCol_TitleBgCollapsed] = DarkTheme.TITLE_BG_COLLAPSED;
	
	style.Colors[ImGuiCol_MenuBarBg] = DarkTheme.MENU_BAR_BG;
	
	style.Colors[ImGuiCol_ScrollbarBg] = DarkTheme.SCROLLBAR_BG;
	style.Colors[ImGuiCol_ScrollbarGrab] = DarkTheme.SCROLLBAR_GRAB;
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = DarkTheme.SCROLLBAR_GRAB_HOVERED;
	style.Colors[ImGuiCol_ScrollbarGrabActive] = DarkTheme.SCROLLBAR_GRAB_ACTIVE;
	
	style.Colors[ImGuiCol_CheckMark] = DarkTheme.CHECKMARK;
	
	style.Colors[ImGuiCol_SliderGrab] = DarkTheme.SLIDER_GRAB;
	style.Colors[ImGuiCol_SliderGrabActive] = DarkTheme.SLIDER_GRAB_ACTIVE;
	
	style.Colors[ImGuiCol_Button] = DarkTheme.BUTTON;
	style.Colors[ImGuiCol_ButtonHovered] = DarkTheme.BUTTON_HOVERED;
	style.Colors[ImGuiCol_ButtonActive] = DarkTheme.BUTTON_ACTIVE;
	
	style.Colors[ImGuiCol_Header] = DarkTheme.HEADER;
	style.Colors[ImGuiCol_HeaderHovered] = DarkTheme.HEADER_HOVERED;
	style.Colors[ImGuiCol_HeaderActive] = DarkTheme.HEADER_ACTIVE;
	
	style.Colors[ImGuiCol_Separator] = DarkTheme.SEPARATOR;
	style.Colors[ImGuiCol_SeparatorHovered] = DarkTheme.SEPARATOR_HOVERED;
	style.Colors[ImGuiCol_SeparatorActive] = DarkTheme.SEPARATOR_ACTIVE;

	style.Colors[ImGuiCol_ResizeGrip] = DarkTheme.RESIZE_GRIP;
	style.Colors[ImGuiCol_ResizeGripHovered] = DarkTheme.RESIZE_GRIP_HOVERED;
	style.Colors[ImGuiCol_ResizeGripActive] = DarkTheme.RESIZE_GRIP_ACTIVE;
	
	style.Colors[ImGuiCol_Tab] = DarkTheme.TAB;
	style.Colors[ImGuiCol_TabHovered] = DarkTheme.TAB_HOVERED;
	style.Colors[ImGuiCol_TabActive] = DarkTheme.TAB_ACTIVE;
	style.Colors[ImGuiCol_TabUnfocused] = DarkTheme.TAB_UNFOCUSED;
	style.Colors[ImGuiCol_TabUnfocusedActive] = DarkTheme.TAB_UNFOCUSED_ACTIVE;
	
	style.Colors[ImGuiCol_PlotLines] = DarkTheme.PLOT_LINES;
	style.Colors[ImGuiCol_PlotLinesHovered] = DarkTheme.PLOT_LINES_HOVERED;
	style.Colors[ImGuiCol_PlotHistogram] = DarkTheme.PLOT_HISTOGRAM;
	style.Colors[ImGuiCol_PlotHistogramHovered] = DarkTheme.PLOT_HISTOGRAM_HOVERED;
	
	style.Colors[ImGuiCol_TableHeaderBg] = DarkTheme.TABLE_HEADER_BG;
	style.Colors[ImGuiCol_TableBorderStrong] = DarkTheme.TABLE_BORDER_STRONG;
	style.Colors[ImGuiCol_TableBorderLight] = DarkTheme.TABLE_BORDER_LIGHT;
	style.Colors[ImGuiCol_TableRowBg] = DarkTheme.TABLE_ROW_BG;
	style.Colors[ImGuiCol_TableRowBgAlt] = DarkTheme.TABLE_ROW_BG_ALT;
	
	style.Colors[ImGuiCol_TextSelectedBg] = DarkTheme.TEXT_SELECTED_BG;
	style.Colors[ImGuiCol_DragDropTarget] = DarkTheme.DRAG_DROP_TARGET;
	
	style.Colors[ImGuiCol_NavHighlight] = DarkTheme.NAV_HIGHLIGHT;
	style.Colors[ImGuiCol_NavWindowingHighlight] = DarkTheme.NAV_WINDOWING_HIGHLIGHT;
	style.Colors[ImGuiCol_NavWindowingDimBg] = DarkTheme.NAV_WINDOWING_DIM_BG;
	style.Colors[ImGuiCol_ModalWindowDimBg] = DarkTheme.MODAL_WINDOW_DIM_BG;
}

LRESULT CALLBACK DragDropWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_DROPFILES: {
			HDROP hDrop = (HDROP)wParam; // Cast wParam to HDROP handle
			UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0); // Get number of dropped files

			std::vector<std::wstring> files;
			for (UINT i = 0; i < fileCount; ++i) {
				// Determine the required buffer size for the file path
				UINT bufferSize = DragQueryFile(hDrop, i, NULL, 0) + 1;
				std::wstring filePath(bufferSize, L'\0');

				// Get the actual file path
				DragQueryFile(hDrop, i, &filePath[0], bufferSize);
				filePath.pop_back(); // Remove the extra null terminator
				files.push_back(filePath);
			}

			if (!files.empty()) {
				std::wstring listedFilenames = L"";
				for (std::wstring importedFile : files) {
					std::wstring sourcePathW(importedFile.c_str());
					listedFilenames += sourcePathW + L"\n";
				}

				std::wstring importPromptMessage(L"Import the following:\n" + listedFilenames + L"to project?");
				int response = MessageBox(hWnd, importPromptMessage.c_str(), L"Dropped File", MB_YESNO | MB_ICONQUESTION);

				if (response == IDYES) {
					for (std::wstring importedFile : files) {
						std::wstring sourcePathW(importedFile.c_str());
						std::string sourcePath(sourcePathW.begin(), sourcePathW.end());
						DragAndDropUtils::copyFileToAssetsRoot(sourcePath);
					}
				}
			}

			DragFinish(hDrop); // Free the memory allocated for the dropped files

			return true;
		}
	}

	return CallWindowProc(imguiWndproc, hWnd, message, wParam, lParam);
}