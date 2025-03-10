#include "RayTracer.hpp"
#include "UserInterface.hpp"
#include "UserSettings.hpp"
#include "Assets/Model.hpp"
#include "Assets/Scene.hpp"
#include "Assets/Texture.hpp"
#include "Assets/UniformBuffer.hpp"
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
#include "ImGui/imgui_impl_vulkan.h"

#include "Engine/CameraSystem/CameraManager.h"
#include "Utilities/FileUtils.h"

namespace
{
	const bool EnableValidationLayers =
#ifdef NDEBUG
		false;
#else
		true;
#endif
}

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
	EventBroadcaster::getInstance()->removeObserver(EventNames::ON_SCENE_LOADED);
	EventBroadcaster::getInstance()->removeObserver(EventNames::ON_MARK_SCENE_DIRTY);
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
	ubo.HasSky = init.HasSky;
	ubo.ShowHeatmap = userSettings_.ShowHeatmap;
	ubo.HeatmapScale = userSettings_.HeatmapScale;

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

	userInterface_.reset(new UserInterface(CommandPool(), SwapChain(), DepthBuffer(), userSettings_));

	if (!initializedUI)
	{
		UIManager::initialize(&userSettings_);
		UIManager::getInstance()->device = &Device();
		UIManager::getInstance()->sampler = new Vulkan::Sampler(Device(), Vulkan::SamplerConfig());

		initializedUI = true;
	}

	resetAccumulation_ = true;

	CheckFramebufferSize();
}

void RayTracer::DeleteSwapChain()
{
	userInterface_.reset();

	Application::DeleteSwapChain();
}

void RayTracer::DrawFrame()
{
	// Check if the scene has been changed by the user via select new scene
	if (sceneIndex_ != static_cast<uint32_t>(userSettings_.SceneIndex))
	{
		Device().WaitIdle();
		DeleteSwapChain();
		DeleteAccelerationStructures();
		LoadScene(userSettings_.SceneIndex);
		CreateAccelerationStructures();
		CreateSwapChain();
		return;
	}
	//If user edited a certain model
	if(this->isSceneDirty)
	{
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

	// Render the UI
	Statistics stats = {};
	stats.framebufferSize = Window().FramebufferSize();
	stats.frameRate = static_cast<float>(1 / timeDelta);

	if (userSettings_.IsRayTraced)
	{
		const auto extent = SwapChain().Extent();

		stats.rayRate = static_cast<float>(
			static_cast<double>(extent.width * extent.height)*numberOfSamples_
			/ (timeDelta * 1000000000));

		stats.totalSamples = totalNumberOfSamples_;
	}

	userInterface_->render(commandBuffer, SwapChainFrameBuffer(imageIndex), stats);
}

void RayTracer::OnKey(int key, int scancode, int action, int mods)
{
	if (userInterface_->wantsToCaptureKeyboard())
	{
		return;
	}

	if (action == GLFW_PRESS)
	{		
		switch (key)
		{
		case GLFW_KEY_ESCAPE: Window().Close(); break;
		default: break;
		}

		// Settings (toggle switches)
		if (!userSettings_.Benchmark)
		{
			switch (key)
			{
			case GLFW_KEY_F1: UIManager::getInstance()->toggleEnabled(UINames::SETTINGS_SCREEN); break;
			case GLFW_KEY_F2: userSettings_.ShowOverlay = !userSettings_.ShowOverlay; break;
			case GLFW_KEY_F3: UIManager::getInstance()->toggleAllUI(); break;
			case GLFW_KEY_F5: EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY); break;
			case GLFW_KEY_1: CameraManager::getInstance()->setSceneCameraProjection(0); break;
			case GLFW_KEY_2: CameraManager::getInstance()->setSceneCameraProjection(1); break;
			case GLFW_KEY_T: userSettings_.IsRayTraced = !userSettings_.IsRayTraced; break;
			case GLFW_KEY_H: userSettings_.ShowHeatmap = !userSettings_.ShowHeatmap; break;
			case GLFW_KEY_P: isWireFrame_ = !isWireFrame_; break;
			default: break;
			}
		}
	}

	// Camera motions
	if (!userSettings_.Benchmark)
	{
		resetAccumulation_ |= CameraManager::getInstance()->getActiveCamera()->OnKey(key, scancode, action, mods);
	}
}

void RayTracer::OnCursorPosition(const double xpos, const double ypos)
{
	if (!HasSwapChain() ||
		userSettings_.Benchmark ||
		userInterface_->wantsToCaptureKeyboard() || 
		userInterface_->wantsToCaptureMouse())
	{
		return;
	}

	// Camera motions
	resetAccumulation_ |= CameraManager::getInstance()->getActiveCamera()->OnCursorPosition(xpos, ypos);
}

void RayTracer::OnMouseButton(const int button, const int action, const int mods)
{
	if (!HasSwapChain() || 
		userSettings_.Benchmark ||
		userInterface_->wantsToCaptureMouse())
	{
		return;
	}

	// Camera motions
	resetAccumulation_ |= CameraManager::getInstance()->getActiveCamera()->OnMouseButton(button, action, mods);
}

void RayTracer::OnScroll(const double xoffset, const double yoffset)
{
	if (!HasSwapChain() ||
		userSettings_.Benchmark ||
		userInterface_->wantsToCaptureMouse())
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
	else if(eventName == EventNames::ON_MARK_SCENE_DIRTY)
	{
		this->isSceneDirty = true;
		GlobalConfig::getInstance()->encodeBool(ConfigKeys::DO_NOT_RESET_CAMERA, true);
		//Debug::Log("Scene marked as dirty! \n");
	}
}

void RayTracer::LoadScene(const uint32_t sceneIndex)
{
	auto [models, textures, lights] = std::get<1>(SceneList::AllScenes[sceneIndex])(cameraInitialSate_);

	// If there are no texture, add a dummy one. It makes the pipeline setup a lot easier.
	if (textures.empty())
	{
		textures.push_back(TextureLibrary::getInstance()->getTexture("white"));
	}
	// If there are no lights, add a dummy one. It makes the pipeline setup a lot easier.
	if (lights.empty())
	{
		lights.push_back(Assets::LightProperties(glm::vec3(2600, 20, 0), glm::vec4(1.0,1.0,1.0,0.02), glm::vec4(1.0,0.4,0.5,1000000.0f), Assets::LightProperties::Enum::PointLight));
	}

	scene_.reset(new Assets::Scene(CommandPool(), std::move(models), std::move(textures), std::move(lights)));
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
