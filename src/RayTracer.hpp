#pragma once
#include "SceneList.hpp"
#include "UserSettings.hpp"
#include "Vulkan/RayTracing/Application.hpp"
#include "From-GDGRAP2/EventBroadcaster.h"
#include "Assets/TextureImage.hpp"
#include "Assets/RayScene.hpp"
#include "UI/UIConfig.hpp"
#include "RayPicker/RayPicker.hpp"
#include "HotkeySystem/HotkeyListener.hpp"

namespace Vulkan {
	class RayVisualizationPipeline;
	class SwapChain;
}
class RayTracer final : public Vulkan::RayTracing::Application, public Observer, public HotkeyListener
{

public:

	VULKAN_NON_COPIABLE(RayTracer)

	RayTracer(const UserSettings& userSettings, const Vulkan::WindowConfig& windowConfig, VkPresentModeKHR presentMode);
	~RayTracer();

	static void initialize(const UserSettings& userSettings, const Vulkan::WindowConfig& windowConfig, VkPresentModeKHR presentMode);
	static RayTracer* getInstance();
	void TakeScreenshot(std::string path);

	bool IsAccumulationComplete() const { return totalNumberOfSamples_ >= userSettings_.MaxNumberOfSamples; }
	uint32_t GetTotalSamples() const { return totalNumberOfSamples_; }
	uint32_t GetMaxSamples() const { return userSettings_.MaxNumberOfSamples; }

	UserSettings getUserSettings() const { return userSettings_; }

protected:

	const Assets::Scene& GetScene() const override { return *scene_; }
	const Assets::RayScene& GetRayScene() const override { return *rayScene_; }
	Assets::UniformBufferObject GetUniformBufferObject(VkExtent2D extent) const override;
	Assets::PushConstantModel GetPushConstantModel(const GameObject& model) const override;
	RayPickerUBO GetRayPickerUBO(const VkExtent2D extent) const override;

	void SetPhysicalDevice(
		VkPhysicalDevice physicalDevice, 
		std::vector<const char*>& requiredExtensions, 
		VkPhysicalDeviceFeatures& deviceFeatures, 
		void* nextDeviceFeatures) override;

	void OnDeviceSet() override;
	void CreateSwapChain() override;
	void DeleteSwapChain() override;
	void DeleteSwapChainWithoutUI();
	void DrawFrame() override;
	void Render(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;

	void OnKey(int key, int scancode, int action, int mods) override;
	void OnCursorPosition(double xpos, double ypos) override;
	void OnMouseButton(int button, int action, int mods) override;
	void OnScroll(double xoffset, double yoffset) override;

	void OnActionPressed(Hotkey::Action action) override;
	void onTriggeredEvent(String eventName, std::shared_ptr<Parameters> parameters) override;

private:
	void LoadScene(uint32_t sceneIndex);
	void ReloadModifiedScene();
	void CheckAndUpdateBenchmarkState(double prevTime);
	void CheckFramebufferSize() const;
	void ResetPicker();
	void SchedulePick(const glm::vec2& mousePos);
	void ExecuteScheduledPick();
	void BroadcastSampleProgress();


	void ScreenToWorldRay(const glm::vec2& mousePos, glm::vec3& outOrigin, glm::vec3& outDirection);

	uint32_t sceneIndex_{};
	UserSettings userSettings_{};
	UserSettings previousSettings_{};
	SceneList::CameraInitialState cameraInitialSate_{};
	UIConfig uiConfig_{};

	std::unique_ptr<Assets::Scene> scene_;
	std::unique_ptr<Assets::TextureImage> skyboxTextureImage_;
	std::unique_ptr<Assets::RayScene> rayScene_;
	//std::unique_ptr<class UserInterface> userInterface_;

	double time_{};

	uint32_t totalNumberOfSamples_{};
	uint32_t numberOfSamples_{};
	bool resetAccumulation_{};

	// Sample progress tracking
	uint32_t lastReportedPercentage_{};

	// Benchmark stats
	double sceneInitialTime_{};
	double periodInitialTime_{};
	uint32_t periodTotalFrames_{};

	bool isSceneDirty = false;

	bool initializedUI = false;

	bool isRenderChanged = false;

	bool renderUI_ = true;
	bool isVisualizeRays_ = false;
	bool isMoving = false;
	bool mousePressed = false;

	bool isPickScheduled = false;

	static RayTracer* sharedInstance;

	std::unique_ptr<class Vulkan::RayVisualizationPipeline> rayVisualizationPipeline_;
	std::unique_ptr<class RayPicker> rayPicker_;
	glm::vec2 scheduledMousePos;
};
