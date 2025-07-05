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

bool UIManager::isStartup = true;
bool UIManager::isHidingUI = false;

UIManager* UIManager::sharedInstance = nullptr;

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
		Throw(std::runtime_error("failed to load ImGui font"));
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

		if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
		{
			if (ImGui::IsKeyPressed(ImGuiKey_W)) mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
			if (ImGui::IsKeyPressed(ImGuiKey_E)) mCurrentGizmoOperation = ImGuizmo::ROTATE;
			if (ImGui::IsKeyPressed(ImGuiKey_R)) mCurrentGizmoOperation = ImGuizmo::SCALE;
		}

		auto selectedObject = ModelManager::getInstance()->getSelectedObject();

		ImGuizmo::BeginFrame();

		float viewportX = 0;
		float viewportY = 0;
		float viewportWidth = swapChain->Extent().width;
		float viewportHeight = swapChain->Extent().height;
		ImGuizmo::SetRect(viewportX, viewportY, viewportWidth, viewportHeight);

		glm::mat4 viewMatrix = CameraManager::getInstance()->getActiveCamera()->GetView();
		glm::mat4 projMatrix = CameraManager::getInstance()->getActiveCamera()->GetProjection();

		if (ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), glm::value_ptr(projMatrix),
			mCurrentGizmoOperation, ImGuizmo::WORLD, glm::value_ptr(selectedObject->getObjectMatrix())))
		{
			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(selectedObject->getObjectMatrix()), translation, rotation, scale);

			if (mCurrentGizmoOperation == ImGuizmo::TRANSLATE)
			{
				isUsingImguizmo = true;

			}
			else if (mCurrentGizmoOperation == ImGuizmo::ROTATE)
			{
				isUsingImguizmo = true;
			}
			else if (mCurrentGizmoOperation == ImGuizmo::SCALE)
			{
				isUsingImguizmo = true;
			}
		}

		if ((isUsingImguizmo && !RayTracer::getInstance()->getUserSettings().IsRayTraced) || (isUsingImguizmo && RayTracer::getInstance()->getUserSettings().IsRayTraced && !ImGuizmo::IsUsingAny()))
		{
			if (selectedObject->getParent())
			{
				glm::vec3 parentWorldPos = selectedObject->getParent()->getWorldPosition();
				translation[0] -= parentWorldPos.x;
				translation[1] -= parentWorldPos.y;
				translation[2] -= parentWorldPos.z;
			}
			selectedObject->setLocalPosition(translation[0], translation[1], translation[2]);

			if (selectedObject->getParent())
			{
				glm::quat parentRot = glm::quat(glm::radians(selectedObject->getParent()->getWorldRotation()));
				glm::quat localRot = glm::inverse(parentRot) * glm::quat(glm::radians(glm::vec3(rotation[0], rotation[1], rotation[2])));
				glm::vec3 eulerLocal = glm::degrees(glm::eulerAngles(localRot));

				rotation[0] = eulerLocal.x;
				rotation[1] = eulerLocal.y;
				rotation[2] = eulerLocal.z;
			}
			selectedObject->setLocalRotation(rotation[0], rotation[1], rotation[2]);

			if (selectedObject->getParent())
			{
				glm::vec3 parentScale = selectedObject->getParent()->getWorldScale();
				scale[0] /= parentScale.x;
				scale[1] /= parentScale.y;
				scale[2] /= parentScale.z;
			}
			selectedObject->setLocalScale(scale[0], scale[1], scale[2]);


			isUsingImguizmo = false;
		}
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

	style.Colors[ImGuiCol_Text] = ImVec4(0.7529411911964417f, 0.7921568751335144f, 0.9607843160629272f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.2549019753932953f, 0.2823529541492462f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1411764770746231f, 0.1568627506494522f, 0.2313725501298904f, 1.0f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.1415455490350723f, 0.1563405692577362f, 0.2313725501298904f, 0.7058823704719543f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.1415455490350723f, 0.1563405692577362f, 0.2313725501298904f, 0.7058823704719543f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.6614994406700134f, 0.6949518918991089f, 0.8392156958580017f, 0.501960813999176f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.2543022036552429f, 0.2832040190696716f, 0.407843142747879f, 0.501960813999176f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2549019753932953f, 0.2823529541492462f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4786158800125122f, 0.6400314569473267f, 0.9686274528503418f, 0.4000000059604645f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4786158800125122f, 0.6400314569473267f, 0.9686274528503418f, 0.6705882549285889f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1411764770746231f, 0.1568627506494522f, 0.2313725501298904f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2549019753932953f, 0.2823529541492462f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.5099999904632568f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.5299999713897705f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.7333333492279053f, 0.6039215922355652f, 0.9686274528503418f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.7333333492279053f, 0.6039215922355652f, 0.9686274528503418f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.7942906618118286f, 0.6929580569267273f, 0.9785407781600952f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.2549019753932953f, 0.2823529541492462f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.47843137383461f, 0.6392157077789307f, 0.9686274528503418f, 0.5137255191802979f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.47843137383461f, 0.6352941393852234f, 0.9686274528503418f, 0.6980392336845398f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.2549019753932953f, 0.2823529541492462f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.47843137383461f, 0.6392157077789307f, 0.9686274528503418f, 0.5137255191802979f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.47843137383461f, 0.6352941393852234f, 0.9686274528503418f, 0.6980392336845398f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 0.7799999713897705f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.6700000166893005f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.949999988079071f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.2549019753932953f, 0.2823529541492462f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.47843137383461f, 0.6352941393852234f, 0.9686274528503418f, 0.7682403326034546f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.47843137383461f, 0.6352941393852234f, 0.9686274528503418f, 0.6995707750320435f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1411764770746231f, 0.1568627506494522f, 0.2313725501298904f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2549019753932953f, 0.2823529541492462f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(1.0f, 0.6196078658103943f, 0.3921568691730499f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.9686274528503418f, 0.4627451002597809f, 0.5568627715110779f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8784313797950745f, 0.686274528503418f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6196078658103943f, 0.3921568691730499f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1019607856869698f, 0.105882354080677f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3499999940395355f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
}

