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

bool UIManager::isStartup = true;
bool UIManager::isHidingUI = false;

UIManager* UIManager::sharedInstance = nullptr;

TransformState UIManager::gizmoBeforeState = {};
bool UIManager::wasUsingGizmoLastFrame = false;
bool UIManager::gizmoWasManipulated = false;

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

}

UIManager::~UIManager()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

UIManager* UIManager::getInstance()
{
	return sharedInstance;
}

void UIManager::initialize(Vulkan::CommandPool* commandPool, const Vulkan::SwapChain* swapChain,
	const Vulkan::DepthBuffer* depthBuffer, UserSettings* userSettings)
{

	sharedInstance = new UIManager();

	const auto& device = swapChain->Device();
	//sharedInstance->device = &swapChain->Device();
	const auto& window = device.Surface().Instance().Window();
	sharedInstance->userSettings = userSettings;
	sharedInstance->swapChain = swapChain;
	sharedInstance->commandPool = commandPool;

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

	// Initialise ImGui Vulkan adapter
	ImGui_ImplVulkan_InitInfo vulkanInit = {};
	vulkanInit.Instance = device.Surface().Instance().Handle();
	vulkanInit.PhysicalDevice = device.PhysicalDevice();
	vulkanInit.Device = device.Handle();
	vulkanInit.QueueFamily = device.GraphicsFamilyIndex();
	vulkanInit.Queue = device.GraphicsQueue();
	vulkanInit.PipelineCache = nullptr;
	vulkanInit.RenderPass = sharedInstance->renderPass->Handle();
	vulkanInit.DescriptorPool = sharedInstance->descriptorPool->Handle();
	vulkanInit.MinImageCount = swapChain->MinImageCount();
	vulkanInit.ImageCount = static_cast<uint32_t>(swapChain->Images().size());
	vulkanInit.Allocator = nullptr;
	vulkanInit.CheckVkResultFn = CheckVulkanResultCallback;

	if (!ImGui_ImplVulkan_Init(&vulkanInit))
	{
		Throw(std::runtime_error("failed to initialise ImGui vulkan adapter"));
	}

	auto& io = ImGui::GetIO();

	io.IniFilename = "../../../src/imgui.ini";
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	// Loads Default UI Layout (from imgui_default_layout.ini)
	//ImGui::LoadIniSettingsFromDisk(ApplicationConfig::DEFAULT_UI_LAYOUT_PATH.c_str());

	// Window scaling and style.
	const auto scaleFactor = window.ContentScale();

	setupImGuiStyle();
	//ImGui::StyleColorsDark();
	ImGui::GetStyle().ScaleAllSizes(scaleFactor);

	// Upload ImGui fonts (use ImGuiFreeType for better font rendering, see https://github.com/ocornut/imgui/tree/master/misc/freetype).
	io.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();

	if (!io.Fonts->AddFontFromFileTTF(FileUtils::getAssetsFolderPath().generic_string().append("/fonts/Cousine-Regular.ttf").data(), 13 * scaleFactor))
	{
		Throw(std::runtime_error("failed to load Cousine font"));
	}

	auto defaultFont = io.Fonts->AddFontFromFileTTF(FileUtils::getAssetsFolderPath().generic_string().append("/fonts/" + DarkTheme.EDITOR_FONT).data(), 18 * scaleFactor);
	if (!defaultFont)
	{
		Throw(std::runtime_error("failed to load Inter font"));
	}

	io.FontDefault = defaultFont;

	static const ImWchar iconRanges[] = { ICON_MIN_MD, ICON_MAX_16_MD, 0 };
	ImFontConfig* iconFontConfig = new ImFontConfig();
	iconFontConfig->MergeMode = false;
	iconFontConfig->PixelSnapH = true;
	iconFontConfig->GlyphOffset =ImVec2(1.0f, 0.0f);

	sharedInstance->iconFont = io.Fonts->AddFontFromFileTTF(FileUtils::getAssetsFolderPath().generic_string().append("/fonts/" + DarkTheme.ICON_FONT).data(), 13 * scaleFactor, iconFontConfig, iconRanges);
	if (!sharedInstance->iconFont)
	{
		Throw(std::runtime_error("failed to load Icon font"));
	}

	Vulkan::SingleTimeCommands::Submit(*commandPool, [](VkCommandBuffer commandBuffer)
		{
			if (!ImGui_ImplVulkan_CreateFontsTexture())
			{
				Throw(std::runtime_error("failed to create ImGui font textures"));
			}
		});

	//ImGui_ImplVulkan_DestroyFontUploadObjects();

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
	this->uiList.push_back(hierarchyScreen);

	const std::shared_ptr<InspectorScreen> inspectorScreen = std::make_shared<InspectorScreen>();
	this->uiTable[UINames::INSPECTOR_SCREEN] = inspectorScreen;
	this->uiList.push_back(inspectorScreen);

	const std::shared_ptr<ConsoleScreen> consoleScreen = std::make_shared<ConsoleScreen>();
	this->uiTable[UINames::CONSOLE_SCREEN] = consoleScreen;
	this->uiList.push_back(consoleScreen);
	Debug::assignConsole(consoleScreen);

	const std::shared_ptr<ProfilerScreen> profilerScreen = std::make_shared<ProfilerScreen>();
	this->uiTable[UINames::PROFILER_SCREEN] = profilerScreen;
	this->uiList.push_back(profilerScreen);

	//std::shared_ptr<gdeng03::PlaybackScreen> playbackScreen = std::make_shared<gdeng03::PlaybackScreen>();
	//this->uiTable[uiNames.PLAYBACK_SCREEN] = playbackScreen;
	//this->uiList.push_back(playbackScreen);

	// nawt working yet lol!
	const std::shared_ptr<gdeng03::MaterialEditorScreen> materialEditorScreen = std::make_shared<gdeng03::MaterialEditorScreen>();
	this->uiTable[UINames::MATERIAL_EDITOR_SCREEN] = materialEditorScreen;
	this->uiList.push_back(materialEditorScreen);

	const std::shared_ptr<SettingsScreen> settingsScreen = std::make_shared<SettingsScreen>();
	this->uiTable[UINames::SETTINGS_SCREEN] = settingsScreen;
	this->uiList.push_back(settingsScreen);

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
	}

	// Debug::Log("Startup is " + (isStartup ? std::string("true") : std::string("false")));
	//
	if (isStartup)
	{
		// 	Debug::Log("UI first startup");
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

void UIManager::resetLayout()
{
	isResettingLayout = true;
}

void UIManager::render(VkCommandBuffer commandBuffer, const Vulkan::FrameBuffer& frameBuffer,
	const Statistics& statistics)
{
	if (isLoadingLayout)
	{
		ImGui::LoadIniSettingsFromDisk(ApplicationConfig::IMGUI_INI_PATH.c_str());
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

	// Draw the rest of your UI first.
	//UIManager::getInstance()->drawAllUI();
	drawAllUI();
	//DrawSettings();
	drawOverlay(statistics);

	//Start ImGuizmo frame.
	if (ModelManager::getInstance()->getSelectedObject() != nullptr)
	{
		static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
		bool isCTRLHeld = false;

		if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
		{
			if (ImGui::IsKeyPressed(ImGuiKey_W)) mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
			if (ImGui::IsKeyPressed(ImGuiKey_E)) mCurrentGizmoOperation = ImGuizmo::ROTATE;
			if (ImGui::IsKeyPressed(ImGuiKey_R)) mCurrentGizmoOperation = ImGuizmo::SCALE;
			if (ImGui::IsKeyPressed(ImGuiKey_LeftCtrl)) isCTRLHeld = true;
		}

		auto selectedObject = ModelManager::getInstance()->getSelectedObject();
		bool isUsingGizmoNow = ImGuizmo::IsUsing();

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
			mCurrentGizmoOperation, ImGuizmo::WORLD, glm::value_ptr(selectedObject->getObjectMatrix())))
		{
			gizmoWasManipulated = true;

			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(selectedObject->getObjectMatrix()), translation, rotation, scale);

			if (auto* parent = selectedObject->getParent())
			{
				glm::vec3 parentWorldPos = parent->getWorldPosition();
				translation[0] -= parentWorldPos.x;
				translation[1] -= parentWorldPos.y;
				translation[2] -= parentWorldPos.z;

				glm::quat parentRot = glm::quat(glm::radians(parent->getWorldRotation()));
				glm::quat localRot = glm::inverse(parentRot) * glm::quat(glm::radians(glm::vec3(rotation[0], rotation[1], rotation[2])));
				glm::vec3 eulerLocal = glm::degrees(glm::eulerAngles(localRot));
				rotation[0] = eulerLocal.x;
				rotation[1] = eulerLocal.y;
				rotation[2] = eulerLocal.z;

				glm::vec3 parentScale = parent->getWorldScale();
				scale[0] /= parentScale.x;
				scale[1] /= parentScale.y;
				scale[2] /= parentScale.z;
			}

			auto inspector = dynamic_pointer_cast<InspectorScreen>(sharedInstance->findUIByName(UINames::INSPECTOR_SCREEN));

			// Uniform Scaling
			// Check from InspectorWindow if uniform scaling is enabled
			if (inspector->IsUniformScalingEnabled() || (mCurrentGizmoOperation == ImGuizmo::SCALE && isCTRLHeld))
			{
				if (scale[0] != gizmoBeforeState.scale.x) // check which value was manipulated
				{
					float ratio = scale[0] / gizmoBeforeState.scale.x; //New / Old scale
					scale[1] = gizmoBeforeState.scale.y * ratio;
					scale[2] = gizmoBeforeState.scale.z * ratio;
				}

				if (scale[1] != gizmoBeforeState.scale.y)
				{
					float ratio = scale[1] / gizmoBeforeState.scale.y; 
					scale[0] = gizmoBeforeState.scale.x * ratio;
					scale[2] = gizmoBeforeState.scale.z * ratio;
				}

				if (scale[2] != gizmoBeforeState.scale.z)
				{
					float ratio = scale[2] / gizmoBeforeState.scale.z;
					scale[0] = gizmoBeforeState.scale.x * ratio;
					scale[1] = gizmoBeforeState.scale.y * ratio;
				}
			}

			if (!RayTracer::getInstance()->getUserSettings().IsRayTraced)
			{
				selectedObject->setLocalPosition(translation[0], translation[1], translation[2]);
				selectedObject->setLocalRotation(rotation[0], rotation[1], rotation[2]);
				selectedObject->setLocalScale(scale[0], scale[1], scale[2]);
			}
		}

		if (!isUsingGizmoNow && wasUsingGizmoLastFrame)
		{
			if (RayTracer::getInstance()->getUserSettings().IsRayTraced)
			{
				selectedObject->setLocalPosition(translation[0], translation[1], translation[2]);
				selectedObject->setLocalRotation(rotation[0], rotation[1], rotation[2]);
				selectedObject->setLocalScale(scale[0], scale[1], scale[2]);
			}
			if (gizmoWasManipulated &&
				!TransformHistory::getInstance().isUndoOrRedoInProgress() &&
				!TransformHistory::getInstance().isUndoOrRedoFinished())
			{
				TransformState afterState = {
					selectedObject->getLocalPosition(),
					selectedObject->getLocalRotation(),
					selectedObject->getLocalScale()
				};

				if (TransformHistory::isDifferent(gizmoBeforeState, afterState))
				{
					TransformHistory::getInstance().recordChange(selectedObject.get(), gizmoBeforeState, afterState);
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

void UIManager::setupImGuiStyle()
{
	// Tokyo Night Storm style from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.6000000238418579f;
	style.WindowPadding = ImVec2(15.0f, 10.10000038146973f);
	style.WindowRounding = 10.0f;
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

