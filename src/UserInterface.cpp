#include "UserInterface.hpp"
#include "SceneList.hpp"
#include "UserSettings.hpp"
#include "Utilities/Exception.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/FrameBuffer.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Vulkan/SingleTimeCommands.hpp"
#include "Vulkan/Surface.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/Window.hpp"

#include "imgui.h"
#include "ImGui/imgui_freetype.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_vulkan.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <glm/gtx/string_cast.hpp>

#include "Engine/CameraSystem/CameraManager.h"
#include "From-GDGRAP2/EventBroadcaster.h"
#include "From-GDGRAP2/EventNames.h"
#include "From-GDGRAP2/ModelManager.h"
#include "UI/UIManager.h"
#include "From-GDGRAP2/RTConfig.h"
#include "ImGui/ImGuizmo.h"
#include "Utilities/FileUtils.h"


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

UserInterface::UserInterface(
	Vulkan::CommandPool& commandPool,
	const Vulkan::SwapChain& swapChain,
	const Vulkan::DepthBuffer& depthBuffer,
	UserSettings& userSettings) :
	userSettings_(userSettings), swapChain(swapChain)
{
	const auto& device = swapChain.Device();
	const auto& window = device.Surface().Instance().Window();

	// Initialise descriptor pool and render pass for ImGui.
	const std::vector<Vulkan::DescriptorBinding> descriptorBindings =
	{
		{0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0},
	};
	descriptorPool_.reset(new Vulkan::DescriptorPool(device, descriptorBindings, 1));
	renderPass_.reset(new Vulkan::RenderPass(swapChain, depthBuffer, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_LOAD));

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
	vulkanInit.DescriptorPool = descriptorPool_->Handle();
	vulkanInit.MinImageCount = swapChain.MinImageCount();
	vulkanInit.ImageCount = static_cast<uint32_t>(swapChain.Images().size());
	vulkanInit.Allocator = nullptr;
	vulkanInit.CheckVkResultFn = CheckVulkanResultCallback;

	if (!ImGui_ImplVulkan_Init(&vulkanInit, renderPass_->Handle()))
	{
		Throw(std::runtime_error("failed to initialise ImGui vulkan adapter"));
	}

	auto& io = ImGui::GetIO();

	io.IniFilename = "../../../src/imgui.ini";
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	// Loads Default UI Layout (from imgui_default_layout.ini)
	ImGui::LoadIniSettingsFromDisk(ApplicationConfig::DEFAULT_UI_LAYOUT_PATH.c_str());

	// Window scaling and style.
	const auto scaleFactor = window.ContentScale();

	ImGui::StyleColorsDark();
	ImGui::GetStyle().ScaleAllSizes(scaleFactor);

	// Upload ImGui fonts (use ImGuiFreeType for better font rendering, see https://github.com/ocornut/imgui/tree/master/misc/freetype).
	io.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
	if (!io.Fonts->AddFontFromFileTTF(FileUtils::getAssetsFolderPath().generic_string().append("/fonts/Cousine-Regular.ttf").data(), 13 * scaleFactor))
	{
		Throw(std::runtime_error("failed to load ImGui font"));
	}

	Vulkan::SingleTimeCommands::Submit(commandPool, [](VkCommandBuffer commandBuffer)
		{
			if (!ImGui_ImplVulkan_CreateFontsTexture(commandBuffer))
			{
				Throw(std::runtime_error("failed to create ImGui font textures"));
			}
		});


	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

UserInterface::~UserInterface()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void UserInterface::Render(VkCommandBuffer commandBuffer, const Vulkan::FrameBuffer& frameBuffer, const Statistics& statistics)
{
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplVulkan_NewFrame();
	ImGui::NewFrame();

	// DrawSettings();
	// DrawOverlay(statistics);
	//ImGui::ShowStyleEditor();
	// Draw the rest of your UI first.
	UIManager::getInstance()->drawAllUI();


	//Start ImGuizmo frame.
	if (ModelManager::getInstance()->getSelectedObject() != nullptr)
	{
		static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);

		if (ImGui::IsKeyPressed(ImGuiKey_W)) mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
		if (ImGui::IsKeyPressed(ImGuiKey_E)) mCurrentGizmoOperation = ImGuizmo::ROTATE;
		if (ImGui::IsKeyPressed(ImGuiKey_R)) mCurrentGizmoOperation = ImGuizmo::SCALE;

		auto selectedObject = ModelManager::getInstance()->getSelectedObject();

		ImGuizmo::BeginFrame();

		float viewportX = 0;
		float viewportY = 0;
		float viewportWidth = swapChain.Extent().width;
		float viewportHeight = swapChain.Extent().height;
		ImGuizmo::SetRect(viewportX, viewportY, viewportWidth, viewportHeight);

		glm::mat4 viewMatrix = CameraManager::getInstance()->getActiveCamera()->GetView();
		glm::mat4 projMatrix = CameraManager::getInstance()->getActiveCamera()->GetProjection();

		if (ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), glm::value_ptr(projMatrix),
			mCurrentGizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(selectedObject->getObjectMatrix())))
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

		if (isUsingImguizmo && !ImGuizmo::IsUsingAny())
		{
			selectedObject->setLocalPosition(glm::vec3(translation[0], translation[1], translation[2]));

			glm::quat newRotationQuat = glm::quat(glm::radians(glm::vec3(rotation[0], rotation[1], rotation[2])));
			selectedObject->setLocalRotation(glm::degrees(glm::eulerAngles(newRotationQuat)));

			glm::vec3 newScale(scale[0], scale[1], scale[2]);
			selectedObject->setLocalScale(newScale);

			isUsingImguizmo = false;
		}
	}

	ImGui::Render();

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass_->Handle();
	renderPassInfo.framebuffer = frameBuffer.Handle();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = renderPass_->SwapChain().Extent();
	renderPassInfo.clearValueCount = 0;
	renderPassInfo.pClearValues = nullptr;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	vkCmdEndRenderPass(commandBuffer);
}

bool UserInterface::WantsToCaptureKeyboard() const
{
	return ImGui::GetIO().WantCaptureKeyboard;
}

bool UserInterface::WantsToCaptureMouse() const
{
	return ImGui::GetIO().WantCaptureMouse;
}

void UserInterface::DrawSettings()
{
	if (!Settings().ShowSettings)
	{
		return;
	}

	const float distance = 10.0f;
	const ImVec2 pos = ImVec2(distance, distance);
	const ImVec2 posPivot = ImVec2(0.0f, 0.0f);
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always, posPivot);

	const auto flags =
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings;

	if (ImGui::Begin("Settings", &Settings().ShowSettings, flags))
	{
		std::vector<const char*> scenes;
		for (const auto& scene : SceneList::AllScenes)
		{
			scenes.push_back(std::get<0>(scene).c_str());
		}

		const auto& window = descriptorPool_->Device().Surface().Instance().Window();

		ImGui::Text("Help");
		ImGui::Separator();
		ImGui::BulletText("F1: toggle Settings.");
		ImGui::BulletText("F2: toggle Statistics.");
		ImGui::BulletText(
			"%c%c%c%c/SHIFT/CTRL: move camera.",
			std::toupper(window.GetKeyName(GLFW_KEY_W, 0)[0]),
			std::toupper(window.GetKeyName(GLFW_KEY_A, 0)[0]),
			std::toupper(window.GetKeyName(GLFW_KEY_S, 0)[0]),
			std::toupper(window.GetKeyName(GLFW_KEY_D, 0)[0]));
		ImGui::BulletText("L/R Mouse: rotate camera/scene.");
		ImGui::NewLine();

		ImGui::Text("Scene");
		ImGui::Separator();
		ImGui::PushItemWidth(-1);
		ImGui::Combo("##SceneList", &Settings().SceneIndex, scenes.data(), static_cast<int>(scenes.size()));
		ImGui::PopItemWidth();
		ImGui::NewLine();

		ImGui::Text("Ray Tracing");
		ImGui::Separator();
		ImGui::Checkbox("Enable ray tracing", &Settings().IsRayTraced);
		ImGui::Checkbox("Accumulate rays between frames", &Settings().AccumulateRays);
		uint32_t min = 1, max = 128;
		ImGui::SliderScalar("Samples", ImGuiDataType_U32, &Settings().NumberOfSamples, &min, &max);
		min = 1, max = 32;
		ImGui::SliderScalar("Bounces", ImGuiDataType_U32, &Settings().NumberOfBounces, &min, &max);
		ImGui::NewLine();

		ImGui::Text("Camera");
		ImGui::Separator();
		ImGui::SliderFloat("FoV", &Settings().FieldOfView, UserSettings::FieldOfViewMinValue, UserSettings::FieldOfViewMaxValue, "%.0f");
		ImGui::SliderFloat("Aperture", &Settings().Aperture, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("Focus", &Settings().FocusDistance, 0.1f, 20.0f, "%.1f");
		ImGui::NewLine();

		ImGui::Text("Profiler");
		ImGui::Separator();
		ImGui::Checkbox("Show heatmap", &Settings().ShowHeatmap);
		ImGui::SliderFloat("Scaling", &Settings().HeatmapScale, 0.10f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		ImGui::NewLine();
	}
	ImGui::End();
}

void UserInterface::DrawOverlay(const Statistics& statistics)
{
	if (!Settings().ShowOverlay)
	{
		return;
	}

	const auto& io = ImGui::GetIO();
	const float distance = 10.0f;
	const ImVec2 pos = ImVec2(io.DisplaySize.x - distance, distance);
	const ImVec2 posPivot = ImVec2(1.0f, 0.0f);
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always, posPivot);
	ImGui::SetNextWindowBgAlpha(0.3f); // Transparent background

	const auto flags =
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoSavedSettings;

	if (ImGui::Begin("Statistics", &Settings().ShowOverlay, flags))
	{
		ImGui::Text("Statistics (%dx%d):", statistics.FramebufferSize.width, statistics.FramebufferSize.height);
		ImGui::Separator();
		ImGui::Text("Frame rate: %.1f fps", statistics.FrameRate);
		ImGui::Text("Primary ray rate: %.2f Gr/s", statistics.RayRate);
		ImGui::Text("Accumulated samples:  %u", statistics.TotalSamples);
	}
	ImGui::End();
}
