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
#include "From-GDGRAP2/TransformHistory.h"
#include "From-GDGRAP2/EventBroadcaster.h"
#include "From-GDGRAP2/EventNames.h"
#include "UI/UIManager.h"
#include "UI/IBLDebugScreen.h"
#include "From-GDGRAP2/MaterialLibrary.h"
#include "From-GDGRAP2/TextureLibrary.h"
#include "Assets/Ray.hpp"

#include "Engine/CameraSystem/CameraManager.h"
#include "Utilities/FileUtils.h"

#include "RayVisualization/RayVisualizationPipeline.h"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/ImageMemoryBarrier.hpp"
#include "Vulkan/Vk_Compute/ComputeShaderRayTracer.hpp"
#include "Vulkan/Vk_Game/GameRenderer.hpp"

#include "StateManagement/CommandManager.hpp"
#include "Assets/ModelLibrary.hpp"
#include "RayPicker/RayPickerUBO.hpp"
#include "HotkeySystem/HotkeySystem.hpp"
#include "Utilities/Screenshot.hpp"

#include "imgui_impl_glfw.h"


// ── IBL debug panel helpers ───────────────────────────────────────────────────

// Returns the IBLDebugScreen from UIManager, or nullptr if not yet initialized.
static std::shared_ptr<IBLDebugScreen> FindIBLDebugScreen()
{
	auto* mgr = UIManager::getInstance();
	if (!mgr) return nullptr;
	return std::dynamic_pointer_cast<IBLDebugScreen>(
		mgr->findUIByName(UINames::IBL_DEBUG_SCREEN));
}

// Called BEFORE UIManager::ReinitializeBackends() — releases ImGui descriptor
// sets while the old Vulkan pool is still alive and can accept the free calls.
static void ReleaseIBLDescriptors()
{
	if (auto screen = FindIBLDebugScreen())
		screen->ReleaseDescriptors();
}

// Called AFTER UIManager::ReinitializeBackends() — the old pool is gone so we
// just clear stale handles (InvalidateDescriptors), then re-register against
// the freshly created pool.
static void RegisterIBLDescriptors(Vulkan::Game::GameRenderer* renderer, const UserSettings* settings = nullptr)
{
	auto screen = FindIBLDebugScreen();
	if (!screen) return;

	// Safety: zero any stale VkDescriptorSet handles that survived the reinit.
	screen->InvalidateDescriptors();
	screen->SetIBL(renderer ? renderer->GetIBLPrecompute() : nullptr);
	// Let the panel reflect the current UseColorIBL / IBLSkyColor state.
	screen->SetUserSettings(settings);
	screen->RegisterDescriptors();
}


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
	EventBroadcaster::getInstance()->addObserver(EventNames::ON_SWAP_RENDERER, this);

	HotkeySystem::getInstance()->addListener(this);

	CameraManager::initialize();
	TextureLibrary::initialize();
	MaterialLibrary::initialize();
	Assets::ModelLibrary::initialize();
	CommandManager::initialize();
}

RayTracer::~RayTracer()
{
	Assets::ModelLibrary::destroy();
	CommandManager::destroy();

	scene_.reset();
	rayScene_.reset();
	computeShaderRenderer_.reset();
	HotkeySystem::getInstance()->removeListener(this);
	EventBroadcaster::getInstance()->removeObserver(EventNames::ON_SCENE_LOADED);
	EventBroadcaster::getInstance()->removeObserver(EventNames::ON_MARK_SCENE_DIRTY);
	EventBroadcaster::getInstance()->removeObserver(EventNames::ON_SWAP_RENDERER);
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
	ubo.SamplesPerInvocation = userSettings_.SamplesPerInvocation;  // Always set, used by compute shader only
	ubo.NumberOfBounces = userSettings_.NumberOfBounces;
	ubo.RandomSeed = 1;
	ubo.MaxRays = userSettings_.MaxRays;
	// In Game renderer mode, IBL is gated by the EnableIBL checkbox.
	// When disabled, HasSky is forced to 0 so the shader falls back to
	// FallbackAmbientColor instead of sampling the IBL textures.
	const bool gameMode = (userSettings_.CurrentRendererMode == UserSettings::RendererMode::Game);
	ubo.HasSky = (gameMode ? (init.HasSky && userSettings_.Game.EnableIBL) : init.HasSky) ? 1u : 0u;
	ubo.ShowHeatmap = userSettings_.ShowHeatmap;
	ubo.HeatmapScale = userSettings_.HeatmapScale;

	// Adaptive Sampling Settings
	ubo.EnableAdaptiveSampling = userSettings_.EnableAdaptiveSampling;
	ubo.VarianceThreshold = userSettings_.VarianceThreshold;
	ubo.MinSamples = userSettings_.MinSamples;

	// Game Renderer Settings
	ubo.FallbackAmbientColor = userSettings_.Game.FallbackAmbientColor;
	ubo.Exposure             = userSettings_.Game.Exposure;

	// UseColorIBL is only meaningful in Game mode with IBL on.
	// Padding fields are zero-initialised by the {} default above.
	ubo.UseColorIBL = (gameMode && userSettings_.Game.EnableIBL && userSettings_.Game.UseColorIBL) ? 1u : 0u;
	ubo.IBLSkyColor = userSettings_.Game.IBLSkyColor;

	return ubo;
}

Assets::PushConstantModel RayTracer::GetPushConstantModel(const GameObject& gameObject) const
{
	Assets::PushConstantModel ubo = {};
	ubo.WorldMatrix = gameObject.getWorldMatrix();

	return ubo;
}

RayPickerUBO RayTracer::GetRayPickerUBO(const VkExtent2D extent) const
{
	RayPickerUBO ubo = {};
	ubo.ModelView = CameraManager::getInstance()->getActiveCamera()->ModelView();
	ubo.Projection = CameraManager::getInstance()->getActiveCamera()->GetProjection(userSettings_, extent);
	ubo.Projection[1][1] *= -1; // Inverting Y for Vulkan, https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
	ubo.ModelViewInverse = glm::inverse(ubo.ModelView);
	ubo.ProjectionInverse = glm::inverse(ubo.Projection);

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

	try {
		auto& engine = Anito::Physics::PhysicsEngine::Get();

		if (!engine.IsInitialized()) {
			if (engine.Initialize()) {
				mDefaultPhysicsWorld = engine.GetDefaultWorld();
				std::cout << "[RayTracer] Physics engine initialized successfully" << std::endl;
			} else {
				std::cerr << "[RayTracer] Failed to initialize physics engine, continuing without physics" << std::endl;
				mDefaultPhysicsWorld = nullptr;
			}
		}
	}
	catch (const std::exception& e) {
		std::cerr << "[RayTracer] Exception during physics engine initialization: " << e.what() << std::endl;
		mDefaultPhysicsWorld = nullptr;
	}
}

void RayTracer::CreateSwapChain()
{
	// Call the parent RayTracing::Application::CreateSwapChain() which will:
	// 1. Call Vulkan::Application::CreateSwapChain() to create swap chain and command buffers
	// 2. Call CreateOutputImage() to create accumulation/output images for ray tracing
	// 3. Create the ray tracing pipeline
	// CRITICAL: Must use explicit scope to call RayTracing::Application, not just Vulkan::Application
	Vulkan::RayTracing::Application::CreateSwapChain();

	rayVisualizationPipeline_.reset(new class Vulkan::RayVisualizationPipeline(SwapChain(), DepthBuffer(), UniformBuffers(), GetScene()));

	// Initialize compute shader renderer if in Compute Shader mode
	if (userSettings_.CurrentRendererMode == UserSettings::RendererMode::ComputeShader)
	{
		computeShaderRenderer_.reset(new Vulkan::Compute::ComputeShaderRayTracer(
			SwapChain(), 
			topAs_[0], 
			*accumulationImageView_, 
			*outputImageView_, 
			*outputImageViewS_, 
			UniformBuffers(), 
			GetScene(), 
			GetRayScene()
		));
		Debug::Log("Compute Shader Renderer initialized");
		computeImagesInitialized_ = false; // Reset layout initialization flag for new renderer
	}

	// Initialize game rasterization renderer if in Game mode
	if (userSettings_.CurrentRendererMode == UserSettings::RendererMode::Game)
	{
		gameRenderer_.reset(new Vulkan::Game::GameRenderer(
			SwapChain(),
			DepthBuffer(),
			UniformBuffers(),
			GetScene(),
			CommandPool()));
		Debug::Log("Game Renderer initialized");
	}

	// If UIManager hasn't been initialized yet
	if (UIManager::getInstance() == nullptr)
	{
		UIManager::initialize(&CommandPool(), &SwapChain(), &DepthBuffer(), &userSettings_, &uiConfig_);
		UIManager::getInstance()->SetProfiler(profiler_.get());

		if (!initializedUI)
		{
			UIManager::getInstance()->initializeUI();
			initializedUI = true;
		}
		// Backend freshly initialized — no old pool to release from, just register.
		RegisterIBLDescriptors(gameRenderer_.get(), &userSettings_);
	}
	else if (initializedUI)
	{
		// UIManager already exists — backend will be torn down and recreated.
		// Release IBL descriptor sets BEFORE the old pool is destroyed, then
		// re-register against the fresh pool AFTER reinit completes.
		ReleaseIBLDescriptors();

		try
		{
			UIManager::ReinitializeBackends(&SwapChain(), &DepthBuffer());
		}
		catch (const std::exception& e)
		{
			Debug::Log("WARNING: Failed to reinitialize UIManager backends: " + std::string(e.what()));
			// Continue anyway - UI might not be critical
		}
		// Old pool is gone — invalidate stale handles, then register fresh ones.
		RegisterIBLDescriptors(gameRenderer_.get(), &userSettings_);
	}

	resetAccumulation_ = true;

	CheckFramebufferSize();
}

void RayTracer::DeleteSwapChain()
{
	//userInterface_.reset();
	computeShaderRenderer_.reset();
	gameRenderer_.reset();
	rayVisualizationPipeline_.reset();
	UIManager::reset();

	// CRITICAL: Wait for all in-flight frames to complete before destroying resources
	Device().WaitIdle();

	// Call the parent RayTracing::Application::DeleteSwapChain() which will:
	// 1. Destroy ray tracing pipeline and shader binding table
	// 2. Destroy output/accumulation images and their memory
	// 3. Call Vulkan::Application::DeleteSwapChain() to destroy swap chain, command buffers, etc.
	// CRITICAL: Must use explicit scope to ensure proper parent-class cleanup
	Vulkan::RayTracing::Application::DeleteSwapChain();
}

void RayTracer::DeleteSwapChainWithoutUI()
{
	//userInterface_.reset();
	computeShaderRenderer_.reset();
	gameRenderer_.reset();
	rayVisualizationPipeline_.reset();

	// Shutdown ImGui GLFW backend without destroying UI state/layout
	// Check if GLFW backend is initialized before shutting it down
	// This is necessary because CreateSwapChain() will reinitialize it
	ImGuiIO& io = ImGui::GetIO();
	if (io.BackendPlatformUserData != nullptr)
	{
		ImGui_ImplGlfw_Shutdown();
	}

	// CRITICAL: Wait for all in-flight frames to complete before destroying resources
	// This prevents the "command buffer in use" validation error
	Device().WaitIdle();

	// Call RayTracing::Application::DeleteSwapChain() which properly cleans up
	// ray tracing resources (rayTracingPipeline, output images, etc.) before
	// calling Vulkan::Application::DeleteSwapChain()
	// NOTE: This variant does NOT reset UIManager so the UI state is preserved
	// CRITICAL: Must use explicit scope to ensure proper parent-class cleanup
	try
	{
		Vulkan::RayTracing::Application::DeleteSwapChain();
	}
	catch (const std::exception& e)
	{
		Debug::Log("ERROR in DeleteSwapChainWithoutUI: " + std::string(e.what()));
		throw;
	}
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

	// Step physics simulation if the engine has been initialized
	auto& engine = Anito::Physics::PhysicsEngine::Get();
	if (engine.IsInitialized() && mDefaultPhysicsWorld) {
		engine.StepAllWorlds(1.f / 60.f);  // Assuming 60 FPS update rate
	}

	// Check if renderer mode was switched
	if (isRenderChanged)
	{
		Debug::Log("=== Starting renderer switch ===");
		Debug::Log("Current renderer mode: " + std::to_string(static_cast<int>(userSettings_.CurrentRendererMode)));
		isRenderChanged = false;
		isSwappingRenderer_ = true;

		try
		{
			Debug::Log("Waiting for device idle before swap...");
			Device().WaitIdle();
			Debug::Log("Device idle complete");

			Debug::Log("Deleting swap chain without UI...");
			DeleteSwapChainWithoutUI();
			Debug::Log("Swap chain deletion complete");

			Debug::Log("Creating new swap chain...");
			CreateSwapChain();
			Debug::Log("New swap chain created successfully");

			resetAccumulation_ = true;
			totalNumberOfSamples_ = 0;
			lastReportedPercentage_ = 0;

			Debug::Log("=== Renderer switch complete ===");
		}
		catch (const std::exception& e)
		{
			Debug::Log("CRITICAL ERROR during renderer switch at: " + std::string(e.what()));
			isSwappingRenderer_ = false;
			throw;
		}

		isSwappingRenderer_ = false;
		return;
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
		ResetPicker();
		this->isSceneDirty = false;
		return;
	}

	//If user edited a certain model
	if (this->isSceneDirty)
	{
		this->isSceneDirty = false;

		if (userSettings_.CurrentRendererMode == UserSettings::RendererMode::Game)
		{
			// In Game mode we don't use acceleration structures and the swap chain
			// doesn't depend on scene geometry — just update the scene data and
			// recreate the renderer so it picks up the new vertex/light buffers.
			Debug::Log("Scene dirty (Game mode), reloading scene data");
			Device().WaitIdle();
			gameRenderer_.reset();
			ReloadModifiedScene();
			gameRenderer_.reset(new Vulkan::Game::GameRenderer(
				SwapChain(), DepthBuffer(), UniformBuffers(), GetScene(), CommandPool()));
			Debug::Log("Game Renderer reloaded");
			// Scene dirty reload: backends were NOT reinit'd, only the GameRenderer
			// (and its IBL images) changed. Release old, register new.
			ReleaseIBLDescriptors();
			RegisterIBLDescriptors(gameRenderer_.get(), &userSettings_);
			return;
		}

		Debug::Log("Scene dirty, reloading scene");
		Device().WaitIdle();
		DeleteSwapChainWithoutUI();
		DeleteAccelerationStructures();
		ReloadModifiedScene();
		CreateAccelerationStructures();
		CreateSwapChain();
		ResetPicker();
		return;
	}

	// Check if the accumulation buffer needs to be reset.
	if (resetAccumulation_ ||
		userSettings_.RequiresAccumulationReset(previousSettings_) ||
		!userSettings_.AccumulateRays)
	{
		totalNumberOfSamples_ = 0;
		lastReportedPercentage_ = 0;
		resetAccumulation_ = false;
	}

	previousSettings_ = userSettings_;

	// Keep track of our sample count (ray tracing modes only).
	// In Game mode there is no accumulation — skip this entirely.
	if (userSettings_.CurrentRendererMode != UserSettings::RendererMode::Game)
	{
		const uint32_t batchSize = (userSettings_.CurrentRendererMode == UserSettings::RendererMode::ComputeShader)
			? userSettings_.SamplesPerInvocation
			: userSettings_.NumberOfSamples;
		numberOfSamples_ = glm::clamp(userSettings_.MaxNumberOfSamples - totalNumberOfSamples_, 0u, batchSize);
		totalNumberOfSamples_ += numberOfSamples_;

		// Broadcast sample progress every 10%
		BroadcastSampleProgress();
	}

	rayScene_->Update(CommandPool());

	ExecuteScheduledPick();

	// Flush any deferred shadow-settings reload BEFORE the command buffer begins.
	// This is the only safe point — GameRenderer::Render() is already inside
	// commandBuffers_->Begin/End, so doing it there would invalidate in-flight resources.
	if (userSettings_.CurrentRendererMode == UserSettings::RendererMode::Game && gameRenderer_)
		gameRenderer_->FlushPendingShadowReload();

	Application::DrawFrame();

}

void RayTracer::Render(VkCommandBuffer commandBuffer, const uint32_t imageIndex)
{
	// Record delta time between calls to Render.
	const auto prevTime = time_;
	time_ = Window().GetTime();
	const auto timeDelta = time_ - prevTime;

	//Debug::Log("Rendering frame, time delta: " + std::to_string(timeDelta) + "s");

	// Update the camera position / angle.
	resetAccumulation_ = CameraManager::getInstance()->getActiveCamera()->UpdateCamera(cameraInitialSate_.ControlSpeed, timeDelta);

	// Check the current state of the benchmark, update it for the new frame.
	CheckAndUpdateBenchmarkState(prevTime);

	if (userSettings_.CurrentRendererMode == UserSettings::RendererMode::Game && gameRenderer_)
	{
		// Game rasterization renderer — draws directly to the swapchain framebuffer.
		gameRenderer_->Render(commandBuffer, imageIndex);
	}
	else if (userSettings_.IsRayTraced)
	{
		if (userSettings_.CurrentRendererMode == UserSettings::RendererMode::ComputeShader && computeShaderRenderer_)
		{
			// Use compute shader renderer
			const auto extent = SwapChain().Extent();

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = 1;

			// On first frame after compute renderer creation, transition from UNDEFINED to GENERAL
			// Otherwise, keep images in GENERAL layout (they're already there from previous frame)
			VkImageLayout accumulationCurrentLayout = computeImagesInitialized_ ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED;
			VkImageLayout outputCurrentLayout = computeImagesInitialized_ ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED;

			Vulkan::ImageMemoryBarrier::Insert(commandBuffer, accumulationImage_->Handle(), subresourceRange, 0,
				VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, accumulationCurrentLayout, VK_IMAGE_LAYOUT_GENERAL);

			Vulkan::ImageMemoryBarrier::Insert(commandBuffer, outputImage_->Handle(), subresourceRange, 0,
				VK_ACCESS_SHADER_WRITE_BIT, outputCurrentLayout, VK_IMAGE_LAYOUT_GENERAL);

			Vulkan::ImageMemoryBarrier::Insert(commandBuffer, outputImageS_->Handle(), subresourceRange, 0,
				VK_ACCESS_SHADER_WRITE_BIT, outputCurrentLayout, VK_IMAGE_LAYOUT_GENERAL);

			// Mark compute images as initialized after first layout transition
			if (!computeImagesInitialized_)
			{
				computeImagesInitialized_ = true;
			}

			// Dispatch compute shader
			computeShaderRenderer_->Dispatch(commandBuffer, imageIndex, extent);

			// Memory barrier to ensure compute shader writes are complete
			VkMemoryBarrier memBarrier = {};
			memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			memBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &memBarrier, 0, nullptr, 0, nullptr);

			// Transition output image for transfer
			Vulkan::ImageMemoryBarrier::Insert(commandBuffer, outputImage_->Handle(), subresourceRange, 
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			Vulkan::ImageMemoryBarrier::Insert(commandBuffer, SwapChain().Images()[imageIndex], subresourceRange, 0,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			// Copy output image into swap-chain image
			VkImageCopy copyRegion;
			copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.srcOffset = { 0, 0, 0 };
			copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.dstOffset = { 0, 0, 0 };
			copyRegion.extent = { extent.width, extent.height, 1 };

			vkCmdCopyImage(commandBuffer,
				outputImage_->Handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				SwapChain().Images()[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &copyRegion);

			// Transition swap chain image to present layout
			Vulkan::ImageMemoryBarrier::Insert(commandBuffer, SwapChain().Images()[imageIndex], subresourceRange, VK_ACCESS_TRANSFER_WRITE_BIT,
				0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

			// Transition output image back to GENERAL for next frame
			Vulkan::ImageMemoryBarrier::Insert(commandBuffer, outputImage_->Handle(), subresourceRange,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

			// Copy output image to host capture buffer for screenshots
			Vulkan::ImageMemoryBarrier::Insert(commandBuffer, outputImageS_->Handle(), subresourceRange,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { extent.width, extent.height, 1 };

			vkCmdCopyImageToBuffer(
				commandBuffer,
				outputImageS_->Handle(),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				hostCaptureBuffer_->Handle(),
				1,
				&region
			);

			VkBufferMemoryBarrier readbackBarrier{};
			readbackBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			readbackBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			readbackBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
			readbackBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			readbackBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			readbackBarrier.buffer = hostCaptureBuffer_->Handle();
			readbackBarrier.offset = 0;
			readbackBarrier.size = VK_WHOLE_SIZE;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_HOST_BIT,
				0,
				0, nullptr,
				1, &readbackBarrier,
				0, nullptr
			);

			// Transition capture image back to GENERAL for next frame
			Vulkan::ImageMemoryBarrier::Insert(commandBuffer, outputImageS_->Handle(), subresourceRange,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
		}
		else
		{
			// Use legacy ray tracing pipeline
			Vulkan::RayTracing::Application::Render(commandBuffer, imageIndex);
		}
	}
	else
	{
		Vulkan::Application::Render(commandBuffer, imageIndex);
	}

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
		case GLFW_KEY_F8: UIManager::getInstance()->toggleEnabled(UINames::IBL_DEBUG_SCREEN); return;

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
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		UIManager::getInstance()->onLMBPressed();
	}
	
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		UIManager::getInstance()->onLMBReleased();

		double xpos, ypos;
		glfwGetCursorPos(Window().Handle(), &xpos, &ypos);
		SchedulePick({ static_cast<float>(xpos), static_cast<float>(ypos) });
	}

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

void RayTracer::OnActionPressed(Hotkey::Action action)
{
	if (action == Hotkey::Action::ScreenShot)
	{
		TakeScreenshot("screenshot");
	}
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
	else if (eventName == EventNames::ON_SWAP_RENDERER)
	{
		int rendererMode = parameters->getIntData("RENDERER_MODE", static_cast<int>(UserSettings::RendererMode::Legacy));
		// Defer the renderer switch to the next frame to avoid issues with command buffer recording
		userSettings_.CurrentRendererMode = static_cast<UserSettings::RendererMode>(rendererMode);
		isRenderChanged = true;
		Debug::Log("Renderer switch scheduled for next frame");
	}
}

void RayTracer::LoadScene(const uint32_t sceneIndex)
{
	auto& commandPool = CommandPool();

	auto [objects, textures, lights] = std::get<1>(SceneList::AllScenes[sceneIndex])(cameraInitialSate_);

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
		lights.push_back(Assets::LightProperties(glm::vec3(2600, 20, 0), glm::vec3(0, -1, 0), glm::vec4(1.0, 1.0, 1.0, 0.02), glm::vec4(1.0, 0.4, 0.5, 1000000.0f), Assets::LightProperties::Enum::PointLight));
	}

	scene_.reset(new Assets::Scene(CommandPool(), std::move(objects), std::move(textures), std::move(lights)));
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
	std::vector<GameObject*> objects = ModelManager::getInstance()->getObjectList();
	std::vector<Assets::Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	// If there are no texture, add a dummy one. It makes the pipeline setup a lot easier.
	if (textures.empty())
	{
		textures.push_back(TextureLibrary::getInstance()->getTexture("white"));
	}
	// If there are no lights, add a dummy one. It makes the pipeline setup a lot easier.
	if (lights.empty())
	{
		lights.push_back(Assets::LightProperties(glm::vec3(1000, 500, 0), glm::vec3(0, -1, 0), glm::vec4(1.0, 1.0, 1.0, 0.02), glm::vec4(1.0, 1.0, 1.0, 1000.0f), Assets::LightProperties::Enum::PointLight));
	}

	scene_.reset(new Assets::Scene(CommandPool(), std::move(objects), std::move(textures), std::move(lights)));
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

void RayTracer::ResetPicker()
{
	rayPicker_.reset(new class RayPicker(*deviceProcedures_, SwapChain(), CommandPool(), topAs_[0], RayPickerUniformBuffers(), GetScene(), *rayTracingProperties_));
}

void RayTracer::SchedulePick(const glm::vec2& mousePos)
{
	this->isPickScheduled = true;
	this->scheduledMousePos = mousePos;
}

void RayTracer::ExecuteScheduledPick()
{
	if (UIManager::getInstance()->IsGizmoUsed() || //give prio to Gizmo
		UIManager::getInstance()->wantsToCaptureMouse()) // mouse over GUI
	{
		this->isPickScheduled = false;
		return;
	}

	if (this->isPickScheduled && rayPicker_)
	{
		this->isPickScheduled = false;

		glm::vec3 rayOrigin, rayDirection;
		ScreenToWorldRay(scheduledMousePos, rayOrigin, rayDirection);

		auto result = rayPicker_->pick(*deviceProcedures_, Device(), rayOrigin, rayDirection, currentFrame_);

		int pickedId = result.objectID;
		auto gameObject = ModelManager::getInstance()->findObjectByID(pickedId);

		if (gameObject)	ModelManager::getInstance()->setSelectedObject(gameObject);
		else ModelManager::getInstance()->setSelectedObject(nullptr);
	}
}

void RayTracer::TakeScreenshot(std::string name)
{
	// Wait for all GPU operations to complete
	Device().WaitIdle();

	// Check if accumulation is complete
	if (totalNumberOfSamples_ < userSettings_.MaxNumberOfSamples)
	{
		Debug::Log("WARNING: Screenshot taken with incomplete samples (" 
			+ std::to_string(totalNumberOfSamples_) + "/" 
			+ std::to_string(userSettings_.MaxNumberOfSamples) + ")");
	}

	auto extent = SwapChain().Extent();

	const uint32_t width = extent.width;
	const uint32_t height = extent.height;
	const uint32_t bytesPerPixel = 4; // RGBA8

	const VkDeviceSize byteSize = VkDeviceSize(width) * height * bytesPerPixel;

	void* mapped = hostCaptureBufferMemory_->Map(0, byteSize);

	Export::SavePNG(name, width, height, bytesPerPixel, mapped);

	hostCaptureBufferMemory_->Unmap();

	Debug::Log("Screenshot saved: " + name + " (Samples: " + std::to_string(totalNumberOfSamples_) + ")");
}

void RayTracer::BroadcastSampleProgress()
{
	if (userSettings_.MaxNumberOfSamples == 0)
		return;

	// Calculate current percentage (0-100)
	uint32_t currentPercentage = (totalNumberOfSamples_ * 100) / userSettings_.MaxNumberOfSamples;

	/*
	Debug::Log("Current sample progress: " + std::to_string(currentPercentage) + "% (" 
		+ std::to_string(totalNumberOfSamples_) + "/" 
		+ std::to_string(userSettings_.MaxNumberOfSamples) + ")");
	*/

	// Round down to nearest interval threshold
	uint32_t currentMilestone = (currentPercentage / sampleProgressInterval_) * sampleProgressInterval_;

	// Check if we've crossed a new interval threshold
	if (currentMilestone > lastReportedPercentage_ && currentMilestone <= 100)
	{
		lastReportedPercentage_ = currentMilestone;

		// Create parameters with progress data
		auto params = std::make_shared<Parameters>(EventNames::ON_SAMPLE_PROGRESS);
		params->encodeInt("percentage", currentMilestone);
		params->encodeInt("currentSamples", totalNumberOfSamples_);
		params->encodeInt("maxSamples", userSettings_.MaxNumberOfSamples);
		params->encodeBool("isComplete", totalNumberOfSamples_ >= userSettings_.MaxNumberOfSamples);

		// Broadcast the event
		EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SAMPLE_PROGRESS, params);

		Debug::Log("Sample Progress: " + std::to_string(currentMilestone) + "% (" 
			+ std::to_string(totalNumberOfSamples_) + "/" 
			+ std::to_string(userSettings_.MaxNumberOfSamples) + ")");
	}
}

void RayTracer::ScreenToWorldRay(const glm::vec2& mousePos,
	glm::vec3& outOrigin,
	glm::vec3& outDirection)
{
	VkExtent2D windowSize = Window().WindowSize();
	float viewportWidth = static_cast<float>(windowSize.width);
	float viewportHeight = static_cast<float>(windowSize.height);

	float x = (2.0f * mousePos.x) / viewportWidth - 1.0f;
	float y = 1.0f - (2.0f * mousePos.y) / viewportHeight;

	glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);

	auto camera = CameraManager::getInstance()->getActiveCamera();

	glm::mat4 invProjection = glm::inverse(camera->GetProjection());
	glm::mat4 invView = glm::inverse(camera->GetView());

	glm::vec4 rayEye = invProjection * rayClip;
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

	glm::vec4 rayWorld = invView * rayEye;
	outDirection = glm::normalize(glm::vec3(rayWorld));

	outOrigin = camera->getLocalPosition();
}
