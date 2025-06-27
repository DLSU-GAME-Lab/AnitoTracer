#include "RayTracer.hpp"
//#include "UserInterface.hpp"
#include "UserSettings.hpp"
#include "Assets/Model.hpp"
#include "Assets/Scene.hpp"
#include "Assets/Texture.hpp"
#include "Assets/UniformBuffer.hpp"
#include "Assets/CubeMapTexture.hpp"
#include "Utilities/Exception.hpp"
#include "Utilities/Glm.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/Window.hpp"
#include <iostream>
#include <sstream>

#include "From-GDGRAP2/Debug.h"
#include "From-GDGRAP2/GlobalConfig.h"
#include "From-GDGRAP2/ModelManager.h"
#include "UI/UIManager.h"
#include "From-GDGRAP2/MaterialLibrary.h"
#include "From-GDGRAP2/TextureLibrary.h"
#include "imgui_impl_vulkan.h"
#include "Assets/Ray.hpp"

#include "Engine/CameraSystem/CameraManager.h"
#include "From-GDGRAP2/RTConfig.h"
#include "Utilities/FileUtils.h"

#include "RayVisualization/RayVisualizationPipeline.h"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/ImageMemoryBarrier.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Vulkan/PipelineLayout.hpp"

namespace
{
	const bool EnableValidationLayers =
#ifdef NDEBUG
		false;
#else
		true;
#endif
}

RayTracer* RayTracer::sharedInstance = nullptr;
RayTracer::RayTracer(const UserSettings& userSettings, const Vulkan::WindowConfig& windowConfig, const VkPresentModeKHR presentMode) :
	Application(windowConfig, presentMode, EnableValidationLayers),
	userSettings_(userSettings)
{
	CheckFramebufferSize();

	EventBroadcaster::getInstance()->addObserver(EventNames::ON_SCENE_LOADED, this);
	EventBroadcaster::getInstance()->addObserver(EventNames::ON_MARK_SCENE_DIRTY, this);

	CameraManager::initialize();
	TextureLibrary::initialize();
	MaterialLibrary::initialize();
}

RayTracer::~RayTracer()
{
	scene_.reset();
	rayScene_.reset();
	EventBroadcaster::getInstance()->removeObserver(EventNames::ON_SCENE_LOADED);
	EventBroadcaster::getInstance()->removeObserver(EventNames::ON_MARK_SCENE_DIRTY);
}

void RayTracer::initialize(const UserSettings& userSettings, const Vulkan::WindowConfig& windowConfig,
	VkPresentModeKHR presentMode)
{
	sharedInstance = new RayTracer(userSettings, windowConfig, presentMode);
}

RayTracer* RayTracer::getInstance()
{
	return sharedInstance;
}

Assets::UniformBufferObject RayTracer::GetUniformBufferObject(const VkExtent2D extent) const
{
	const auto& init = cameraInitialSate_;

	Assets::UniformBufferObject ubo = {};
	//ubo.ModelView = modelViewController_.ModelView();
	ubo.ModelView = CameraManager::getInstance()->getActiveCamera()->ModelView();
	ubo.Projection = CameraManager::getInstance()->getActiveCamera()->GetProjection(userSettings_, extent);
	ubo.Projection[1][1] *= -1; // Inverting Y for Vulkan, https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
	ubo.ModelViewInverse = glm::inverse(ubo.ModelView);
	ubo.ProjectionInverse = glm::inverse(ubo.Projection);
	ubo.Aperture = userSettings_.Aperture;
	ubo.FocusDistance = userSettings_.FocusDistance;
	ubo.TotalNumberOfSamples = totalNumberOfSamples_;
	ubo.NumberOfSamples = numberOfSamples_;
	ubo.NumberOfBounces = userSettings_.NumberOfBounces;
	ubo.RandomSeed = 1;
	ubo.MaxRays = userSettings_.MaxRays;
	ubo.HasSky = init.HasSky;
	ubo.ShowHeatmap = userSettings_.ShowHeatmap;
	ubo.HeatmapScale = userSettings_.HeatmapScale;

	return ubo;
}

Assets::PushConstantModel RayTracer::GetPushConstantModel(const Assets::Model& model) const
{
	Assets::PushConstantModel ubo = {};
	ubo.WorldMatrix = model.GetWorldMatrix();

	return ubo;
}

void RayTracer::SetPhysicalDevice(
	VkPhysicalDevice physicalDevice,
	std::vector<const char*>& requiredExtensions,
	VkPhysicalDeviceFeatures& deviceFeatures,
	void* nextDeviceFeatures)
{
	// Required extensions.
	requiredExtensions.insert(requiredExtensions.end(),
		{
			// VK_KHR_SHADER_CLOCK is required for heatmap
			VK_KHR_SHADER_CLOCK_EXTENSION_NAME
		});

	// Opt-in into mandatory device features.
	VkPhysicalDeviceShaderClockFeaturesKHR shaderClockFeatures = {};
	shaderClockFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR;
	shaderClockFeatures.pNext = nextDeviceFeatures;
	shaderClockFeatures.shaderSubgroupClock = true;

	deviceFeatures.fillModeNonSolid = true;
	deviceFeatures.samplerAnisotropy = true;
	deviceFeatures.shaderInt64 = true;


	Application::SetPhysicalDevice(physicalDevice, requiredExtensions, deviceFeatures, &shaderClockFeatures);
}

void RayTracer::OnDeviceSet()
{
	Application::OnDeviceSet();

	LoadScene(userSettings_.SceneIndex);
	CreateAccelerationStructures();
}

void RayTracer::CreateSwapChain()
{
	Application::CreateSwapChain();

	rayVisualizationPipeline_.reset(new class Vulkan::RayVisualizationPipeline(SwapChain(), DepthBuffer(), UniformBuffers(), GetScene()));
	//userInterface_.reset(new UserInterface(CommandPool(), SwapChain(), DepthBuffer(), userSettings_));
	//UIManager::reset();
	UIManager::initialize(&CommandPool(), &SwapChain(), &DepthBuffer(), &userSettings_);
	UIManager::getInstance()->SetProfiler(profiler_.get());

	if (!initializedUI)
	{
		UIManager::getInstance()->initializeUI();
		// UIManager::getInstance()->device = &Device();
		// UIManager::getInstance()->sampler = new Vulkan::Sampler(Device(), Vulkan::SamplerConfig());

		initializedUI = true;
	}

	resetAccumulation_ = true;

	CheckFramebufferSize();
}

void RayTracer::DeleteSwapChain()
{
	//userInterface_.reset();
	rayVisualizationPipeline_.reset();
	UIManager::reset();

	Application::DeleteSwapChain();
}

void RayTracer::DrawFrame()
{
	if (userSettings_.MultiSampling) {
		if (isMoving || mousePressed)
		{
			userSettings_.NumberOfSamples = 2;
		}
		else
		{
			userSettings_.NumberOfSamples = 2 * userSettings_.aaValue;
		}

		//if (userSettings_.NumberOfSamples > 24) 
		//{
		//	userSettings_.NumberOfSamples = 24;
		//}
	}
	// Check if the scene has been changed by the user via select new scene
	if (sceneIndex_ != static_cast<uint32_t>(userSettings_.SceneIndex))
	{
		Debug::Log("Scene changed, loading scene " + std::to_string(userSettings_.SceneIndex));
		Device().WaitIdle();
		DeleteSwapChain();
		DeleteAccelerationStructures();
		LoadScene(userSettings_.SceneIndex);
		CreateAccelerationStructures();
		CreateSwapChain();
		this->isSceneDirty = false;
		return;
	}

	//If user edited a certain model
	if (this->isSceneDirty)
	{
		Debug::Log("Scene dirty, reloading scene");
		this->isSceneDirty = false;
		Device().WaitIdle();
		DeleteSwapChain();
		DeleteAccelerationStructures();
		ReloadModifiedScene();
		CreateAccelerationStructures();
		CreateSwapChain();
		return;
	}

	// Check if the accumulation buffer needs to be reset.
	if (resetAccumulation_ ||
		userSettings_.RequiresAccumulationReset(previousSettings_) ||
		!userSettings_.AccumulateRays)
	{
		totalNumberOfSamples_ = 0;
		resetAccumulation_ = false;
	}

	previousSettings_ = userSettings_;

	// Keep track of our sample count.
	numberOfSamples_ = glm::clamp(userSettings_.MaxNumberOfSamples - totalNumberOfSamples_, 0u, userSettings_.NumberOfSamples);
	totalNumberOfSamples_ += numberOfSamples_;

	rayScene_->Update(CommandPool());

	Application::DrawFrame();
}

void RayTracer::Render(VkCommandBuffer commandBuffer, const uint32_t imageIndex)
{
	// Record delta time between calls to Render.
	const auto prevTime = time_;
	time_ = Window().GetTime();
	const auto timeDelta = time_ - prevTime;

	// Update the camera position / angle.
	resetAccumulation_ = CameraManager::getInstance()->getActiveCamera()->UpdateCamera(cameraInitialSate_.ControlSpeed, timeDelta);

	// Check the current state of the benchmark, update it for the new frame.
	CheckAndUpdateBenchmarkState(prevTime);

	// Render the scene
	userSettings_.IsRayTraced
		? Vulkan::RayTracing::Application::Render(commandBuffer, imageIndex)
		: Vulkan::Application::Render(commandBuffer, imageIndex);

	// Render ray visualization
	if (isVisualizeRays_)
	{
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = rayVisualizationPipeline_->RenderPass().Handle();
		renderPassInfo.framebuffer = SwapChainFrameBuffer(imageIndex).Handle();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = SwapChain().Extent();
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			const auto& rayScene = GetRayScene();

			VkDescriptorSet descriptorSets[] = { rayVisualizationPipeline_->DescriptorSet(imageIndex) };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rayVisualizationPipeline_->Handle());
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rayVisualizationPipeline_->PipelineLayout().Handle(), 0, 1, descriptorSets, 0, nullptr);

			for (const auto& rays : rayScene.Rays())
			{
				VkBuffer vertexBuffer = rays->VertexBuffer().Handle();

				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
				vkCmdSetLineWidth(commandBuffer, 5);

				vkCmdDraw(commandBuffer, rays->NumberOfVertices(), 1, 0, 0);
			}
		}

		vkCmdEndRenderPass(commandBuffer);
	}

	// Render the UI
	Statistics stats = {};
	stats.framebufferSize = Window().FramebufferSize();
	stats.frameRate = static_cast<float>(1 / timeDelta);

	if (userSettings_.IsRayTraced)
	{
		const auto extent = SwapChain().Extent();

		stats.rayRate = static_cast<float>(
			static_cast<double>(extent.width * extent.height) * numberOfSamples_
			/ (timeDelta * 1000000000));

		stats.totalSamples = totalNumberOfSamples_;
	}

	if (renderUI_)
		UIManager::getInstance()->render(commandBuffer, SwapChainFrameBuffer(imageIndex), stats);

	if (isTakingScreenshot)
	{
		saveScreenshot(commandBuffer, imageIndex);
		isTakingScreenshot = false;
	}
}

void RayTracer::OnKey(int key, int scancode, int action, int mods)
{
	// Settings (toggle switches)
	if (action == GLFW_PRESS)
	{
		isMoving = true;
		switch (key)
		{
		case GLFW_KEY_F1: UIManager::getInstance()->toggleEnabled(UINames::SETTINGS_SCREEN); return;
		case GLFW_KEY_F2: userSettings_.ShowOverlay = !userSettings_.ShowOverlay; return;
		case GLFW_KEY_F3: UIManager::getInstance()->toggleAllUI(); return;
		case GLFW_KEY_F4: userSettings_.IsRayTraced = !userSettings_.IsRayTraced; EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY); return;
		case GLFW_KEY_F5: EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY); return;
		case GLFW_KEY_F6: isVisualizeRays_ = !isVisualizeRays_; EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY); return;
		case GLFW_KEY_F11: isTakingScreenshot = true; return;
			// case GLFW_KEY_H: userSettings_.ShowHeatmap = !userSettings_.ShowHeatmap; return;
			// case GLFW_KEY_O: isWireFrame_ = !isWireFrame_; EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY); return;
			// case GLFW_KEY_P: isWireFrame_ = !isWireFrame_; EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY); return;
			//case GLFW_KEY_U: renderUI_ = !renderUI_; EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY); return;


		default: break;
		}
	}

	if (UIManager::wantsToCaptureKeyboard())
	{
		return;
	}

	// Camera motions
	if (!userSettings_.Benchmark)
	{
		resetAccumulation_ |= CameraManager::getInstance()->getActiveCamera()->OnKey(key, scancode, action, mods);

	}

	if (action == GLFW_RELEASE) {
		isMoving = false;
	}
}

void RayTracer::OnCursorPosition(const double xpos, const double ypos)
{
	if (!HasSwapChain() ||
		userSettings_.Benchmark ||
		UIManager::wantsToCaptureKeyboard() ||
		UIManager::wantsToCaptureMouse())
	{
		return;
	}

	// Camera motions
	resetAccumulation_ |= CameraManager::getInstance()->getActiveCamera()->OnCursorPosition(xpos, ypos);

}

void RayTracer::OnMouseButton(const int button, const int action, const int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		isMoving = true;
		mousePressed = true;
	}

	if (!HasSwapChain() ||
		userSettings_.Benchmark ||
		UIManager::wantsToCaptureMouse())
	{
		return;
	}

	// Camera motions

	resetAccumulation_ |= CameraManager::getInstance()->getActiveCamera()->OnMouseButton(button, action, mods);

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
	{
		isMoving = false;
		mousePressed = false;
	}
}

void RayTracer::OnScroll(const double xoffset, const double yoffset)
{
	if (!HasSwapChain() ||
		userSettings_.Benchmark ||
		UIManager::wantsToCaptureMouse())
	{
		return;
	}

	const auto prevFov = userSettings_.FieldOfView;
	userSettings_.FieldOfView = std::clamp(
		static_cast<float>(prevFov - yoffset),
		UserSettings::FieldOfViewMinValue,
		UserSettings::FieldOfViewMaxValue);

	resetAccumulation_ = prevFov != userSettings_.FieldOfView;
}

void RayTracer::onTriggeredEvent(String eventName, std::shared_ptr<Parameters> parameters)
{
	// {"Cube And Spheres", CubeAndSpheres},
	// { "Ray Tracing In One Weekend", RayTracingInOneWeekend },
	// { "Planets In One Weekend", PlanetsInOneWeekend },
	// { "Lucy In One Weekend", LucyInOneWeekend },
	// { "Cornell Box", CornellBox },
	// { "Cornell Box & Lucy", CornellBoxLucy },

	if (eventName == EventNames::ON_SCENE_LOADED)
	{
		int sceneIndex = parameters->getIntData("SCENE_INDEX", 0);
		userSettings_.SceneIndex = sceneIndex;
		GlobalConfig::getInstance()->encodeBool(ConfigKeys::DO_NOT_RESET_CAMERA, false);
	}
	else if (eventName == EventNames::ON_MARK_SCENE_DIRTY)
	{
		this->isSceneDirty = true;
		GlobalConfig::getInstance()->encodeBool(ConfigKeys::DO_NOT_RESET_CAMERA, true);
		//Debug::Log("Scene marked as dirty! \n");
	}
}

void RayTracer::LoadScene(const uint32_t sceneIndex)
{
	auto& commandPool = CommandPool();

	auto [models, textures, lights] = std::get<1>(SceneList::AllScenes[sceneIndex])(cameraInitialSate_);

	Assets::CubeMapTexture skyboxCubeMap;

	skyboxCubeMap.faces[0] = FileUtils::getAssetsFolderPath().generic_string() + "/textures/sky_right.png";
	skyboxCubeMap.faces[1] = FileUtils::getAssetsFolderPath().generic_string() + "/textures/sky_left.png";
	skyboxCubeMap.faces[2] = FileUtils::getAssetsFolderPath().generic_string() + "/textures/sky_top.png";
	skyboxCubeMap.faces[3] = FileUtils::getAssetsFolderPath().generic_string() + "/textures/sky_bottom.png";
	skyboxCubeMap.faces[4] = FileUtils::getAssetsFolderPath().generic_string() + "/textures/sky_front.png";
	skyboxCubeMap.faces[5] = FileUtils::getAssetsFolderPath().generic_string() + "/textures/sky_back.png";

	skyboxTextureImage_ = std::make_unique<Assets::TextureImage>(commandPool, skyboxCubeMap);
	/*std::cout << "TextureImage ImageView handle: " << skyboxTextureImage_->ImageView().Handle() << std::endl;
	std::cout << "TextureImage Sampler handle: " << skyboxTextureImage_->Sampler().Handle() << std::endl;*/

	// If there are no texture, add a dummy one. It makes the pipeline setup a lot easier.
	if (textures.empty())
	{
		textures.push_back(TextureLibrary::getInstance()->getTexture("white"));
	}
	// If there are no lights, add a dummy one. It makes the pipeline setup a lot easier.
	if (lights.empty())
	{
		lights.push_back(Assets::LightProperties(glm::vec3(2600, 20, 0), glm::vec4(1.0, 1.0, 1.0, 0.02), glm::vec4(1.0, 0.4, 0.5, 1000000.0f), Assets::LightProperties::Enum::PointLight));
	}

	scene_.reset(new Assets::Scene(CommandPool(), std::move(models), std::move(textures), std::move(lights)));
	scene_->SetSkybox(
		skyboxTextureImage_->ImageView().Handle(),
		skyboxTextureImage_->Sampler().Handle()
	);
	//std::cout << "Skybox ImageView: " << scene_->SkyboxImageView() << std::endl;
	//std::cout << "Skybox Sampler: " << scene_->SkyboxSampler() << std::endl;

	rayScene_.reset(new Assets::RayScene(CommandPool(), userSettings_));
	sceneIndex_ = sceneIndex;

	userSettings_.FieldOfView = cameraInitialSate_.FieldOfView;
	userSettings_.Aperture = cameraInitialSate_.Aperture;
	userSettings_.FocusDistance = cameraInitialSate_.FocusDistance;

	CameraManager::getInstance()->getActiveCamera()->Reset(cameraInitialSate_.ModelView);

	periodTotalFrames_ = 0;
	resetAccumulation_ = true;
}

/**
 * \brief Loads the modified scene but using the model manager as reference.
 * \param sceneIndex
 */
void RayTracer::ReloadModifiedScene()
{
	std::vector<Assets::Model> models = ModelManager::getInstance()->getAllObjectModels();
	std::vector<Assets::Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	for (auto& model : models)
	{
		model.ResetVertices();
	}

	// If there are no texture, add a dummy one. It makes the pipeline setup a lot easier.
	if (textures.empty())
	{
		textures.push_back(TextureLibrary::getInstance()->getTexture("white"));
	}
	// If there are no lights, add a dummy one. It makes the pipeline setup a lot easier.
	if (lights.empty())
	{
		lights.push_back(Assets::LightProperties(glm::vec3(1000, 500, 0), glm::vec4(1.0, 1.0, 1.0, 0.02), glm::vec4(1.0, 1.0, 1.0, 1000.0f), Assets::LightProperties::Enum::PointLight));
	}

	scene_.reset(new Assets::Scene(CommandPool(), std::move(models), std::move(textures), std::move(lights)));
	scene_->SetSkybox(
		skyboxTextureImage_->ImageView().Handle(),
		skyboxTextureImage_->Sampler().Handle()
	);
	rayScene_.reset(new Assets::RayScene(CommandPool(), userSettings_));

	// userSettings_.FieldOfView = cameraInitialSate_.FieldOfView;
	// userSettings_.Aperture = cameraInitialSate_.Aperture;
	// userSettings_.FocusDistance = cameraInitialSate_.FocusDistance;
	//
	// modelViewController_.Reset(cameraInitialSate_.ModelView);

	periodTotalFrames_ = 0;
	resetAccumulation_ = true;
}

void RayTracer::CheckAndUpdateBenchmarkState(double prevTime)
{
	if (!userSettings_.Benchmark)
	{
		return;
	}

	// Initialise scene benchmark timers
	if (periodTotalFrames_ == 0)
	{
		std::cout << std::endl;
		std::cout << "Benchmark: Start scene #" << sceneIndex_ << " '" << std::get<0>(SceneList::AllScenes[sceneIndex_]) << "'" << std::endl;
		sceneInitialTime_ = time_;
		periodInitialTime_ = time_;
	}

	// Print out the frame rate at regular intervals.
	{
		const double period = 5;
		const double prevTotalTime = prevTime - periodInitialTime_;
		const double totalTime = time_ - periodInitialTime_;

		if (periodTotalFrames_ != 0 && static_cast<uint64_t>(prevTotalTime / period) != static_cast<uint64_t>(totalTime / period))
		{
			std::cout << "Benchmark: " << periodTotalFrames_ / totalTime << " fps" << std::endl;
			periodInitialTime_ = time_;
			periodTotalFrames_ = 0;
		}

		periodTotalFrames_++;
	}

	// If in benchmark mode, bail out from the scene if we've reached the time or sample limit.
	{
		const bool timeLimitReached = periodTotalFrames_ != 0 && Window().GetTime() - sceneInitialTime_ > userSettings_.BenchmarkMaxTime;
		const bool sampleLimitReached = numberOfSamples_ == 0;

		if (timeLimitReached || sampleLimitReached)
		{
			if (!userSettings_.BenchmarkNextScenes || static_cast<size_t>(userSettings_.SceneIndex) == SceneList::AllScenes.size() - 1)
			{
				Window().Close();
			}

			std::cout << std::endl;
			userSettings_.SceneIndex += 1;
		}
	}
}

void RayTracer::CheckFramebufferSize() const
{
	// Check the framebuffer size when requesting a fullscreen window, as it's not guaranteed to match.
	const auto& cfg = Window().Config();
	const auto fbSize = Window().FramebufferSize();

	if (userSettings_.Benchmark && cfg.Fullscreen && (fbSize.width != cfg.Width || fbSize.height != cfg.Height))
	{
		std::ostringstream out;
		out << "framebuffer fullscreen size mismatch (requested: ";
		out << cfg.Width << "x" << cfg.Height;
		out << ", got: ";
		out << fbSize.width << "x" << fbSize.height << ")";

		Throw(std::runtime_error(out.str()));
	}
}

void RayTracer::saveScreenshot(VkCommandBuffer commandBuffer, const uint32_t imageIndex)
{
	Debug::Log("Saving screenshot to " + ApplicationConfig::SCREENSHOT_PATH);

	bool supportsBlit = true;

	// Check blit support for source and destination
	VkFormatProperties formatProps;

	// Check if the device supports blitting from optimal images (the swapchain images are in optimal format)
	vkGetPhysicalDeviceFormatProperties(Device().PhysicalDevice(), SwapChain().Format(), &formatProps);
	if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
		std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	// Check if the device supports blitting to linear images
	vkGetPhysicalDeviceFormatProperties(Device().PhysicalDevice(), VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
	if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
		std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	// Source for the copy is the last rendered swapchain image
	VkImage srcImage = SwapChain().Images()[imageIndex];

	screenshotImage_.reset(new Vulkan::Image(
		Device(),
		SwapChain().Extent(),
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_LINEAR,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT
	));

	screenshotImageMemory_.reset(
		new Vulkan::DeviceMemory(screenshotImage_->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)));

	VkImageSubresourceRange subresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	Vulkan::ImageMemoryBarrier::Insert(commandBuffer, screenshotImage_->Handle(), subresourceRange,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	Vulkan::ImageMemoryBarrier::Insert(commandBuffer, srcImage, subresourceRange,
		VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	// If source and destination support blit we'll blit as this also does automatic format conversion (e.g. from BGR to RGB)
	if (supportsBlit)
	{
		// Define the region to blit (we will blit the whole swap chain image)
		VkOffset3D blitSize;
		blitSize.x = SwapChain().Extent().width;
		blitSize.y = SwapChain().Extent().height;
		blitSize.z = 1;
		VkImageBlit imageBlitRegion{};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = blitSize;
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = blitSize;

		// Issue the blit command
		vkCmdBlitImage(
			commandBuffer,
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			screenshotImage_->Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlitRegion,
			VK_FILTER_NEAREST);
	}
	else
	{
		// Otherwise use image copy (requires us to manually flip components)
		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = SwapChain().Extent().width;
		imageCopyRegion.extent.height = SwapChain().Extent().height;
		imageCopyRegion.extent.depth = 1;

		// Issue the copy command
		vkCmdCopyImage(
			commandBuffer,
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			screenshotImage_->Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopyRegion);
	}

	// Transition destination image to general layout, which is the required layout for mapping the image memory later on
	Vulkan::ImageMemoryBarrier::Insert(commandBuffer, screenshotImage_->Handle(), subresourceRange,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

	// Transition back the swap chain image after the blit is done
	Vulkan::ImageMemoryBarrier::Insert(commandBuffer, srcImage, subresourceRange,
		VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// Get layout of the image (including row pitch)
	VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
	VkSubresourceLayout subResourceLayout;
	vkGetImageSubresourceLayout(Device().Handle(), screenshotImage_->Handle(), &subResource, &subResourceLayout);

	// Map image memory so we can start copying from it
	const char* data;
	vkMapMemory(Device().Handle(), screenshotImageMemory_->Handle(), 0, VK_WHOLE_SIZE, 0, (void**)&data);
	data += subResourceLayout.offset;

	std::ofstream file(ApplicationConfig::SCREENSHOT_PATH + "/FUCK.ppm", std::ios::out | std::ios::binary | std::ios::trunc);

	// ppm header
	file << "P6\n" << (int)SwapChain().Extent().width << "\n" << (int)SwapChain().Extent().height << "\n" << 255 << "\n";

	// If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
	bool colorSwizzle = false;
	// Check if source is BGR
	// Note: Not complete, only contains most common and basic BGR surface formats for demonstration purposes
	if (!supportsBlit)
	{
		std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
		colorSwizzle = (std::ranges::find(formatsBGR, SwapChain().Format()) != formatsBGR.end());
	}

	// ppm binary pixel data
	for (uint32_t y = 0; y < (int)SwapChain().Extent().height; y++)
	{
		unsigned int* row = (unsigned int*)data;
		for (uint32_t x = 0; x < (int)SwapChain().Extent().width; x++)
		{
			if (colorSwizzle)
			{
				file.write(reinterpret_cast<char*>(row) + 2, 1);
				file.write(reinterpret_cast<char*>(row) + 1, 1);
				file.write(reinterpret_cast<char*>(row), 1);
			}
			else
			{
				file.write(reinterpret_cast<char*>(row), 3);
			}
			row++;
		}
		data += subResourceLayout.rowPitch;
	}
	file.close();

	std::cout << "Screenshot saved to disk" << std::endl;

	// // Clean up resources
	// vkUnmapMemory(Device().Handle(), dstImageMemory);
	// vkFreeMemory(Device().Handle(), dstImageMemory, nullptr);
	// vkDestroyImage(Device().Handle(), dstImage, nullptr);
	//
	// screenshotSaved = true;
}
