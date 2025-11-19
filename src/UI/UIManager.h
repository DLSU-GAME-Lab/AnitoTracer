#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include "imgui.h"
#include "ImGuizmo.h"
#include "AUIScreen.h"
#include "UserSettings.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"

#include "Vulkan/Vulkan.hpp"
#include <memory>

#include "Engine/Profiler/Profiler.h"

#include "From-GDGRAP2/TransformHistory.h"
#include "HotkeySystem/HotkeyListener.hpp"


typedef std::string String;

namespace Vulkan
{
	class CommandPool;
	class DepthBuffer;
	class DescriptorPool;
	class FrameBuffer;
	class RenderPass;
	class SwapChain;
	
	class TransformState;
}

struct UserSettings;

struct Statistics final
{
	VkExtent2D framebufferSize;
	float frameRate;
	float rayRate;
	uint32_t totalSamples;
};

class Viewport;
class UIManager : HotkeyListener
{
public:
	typedef std::string String;
	typedef std::vector<std::shared_ptr<AUIScreen>> UIList;
	typedef std::unordered_map<String, std::shared_ptr<AUIScreen>> UITable;
	typedef std::vector<String> SavedLayouts;

	VULKAN_NON_COPIABLE(UIManager)

	UIManager();
	~UIManager();

	static UIManager* getInstance();
	static void initialize(Vulkan::CommandPool* commandPool, const Vulkan::SwapChain* swapChain, const Vulkan::DepthBuffer* depthBuffer, UserSettings* userSettings);
	static void reset();

	void initializeUI();
	static void saveLayout();
	static void saveLayout(String name);
	static void saveDefaultLayout();
	static void saveDynamicLayout();
	void loadDynamicLayout();
	void loadLayout();
	void loadLayoutFromFile();
	void loadPresetLayout(int index);
	void resetLayout();

	bool getEnabled(const std::string& name);
	void setEnabled(const String& uiName, bool flag);
	void toggleEnabled(const String& uiName);
	std::shared_ptr<AUIScreen> findUIByName(const String& uiName);

	UserSettings* settings() const { return userSettings; }

	void toggleAllUI();
	void hideAllUI() const;
	void showAllUI() const;

	GpuCpuProfiler* profiler;
	Vulkan::CommandPool* commandPool;
	std::unique_ptr<Vulkan::DescriptorPool> descriptorPool;
	void SetProfiler(GpuCpuProfiler* profiler) { this->profiler = profiler; }
	void FreeDescriptor(VkDescriptorSet& descriptorset);

	// fucky test code below vvv
	//std::vector<VkImage>* images = nullptr;
	//Vulkan::Device* device = nullptr;
	// Vulkan::Sampler* sampler = nullptr;
	// Vulkan::ImageView* imageView = nullptr;
	// VkDescriptorSet m_Dset;
	//Vulkan::Image* image = nullptr;
	// fucky test code above ^^^

	void render(VkCommandBuffer commandBuffer, const Vulkan::FrameBuffer& frameBuffer, const Statistics& statistics);

	static bool wantsToCaptureKeyboard();
	static bool wantsToCaptureMouse();

	ImFont* GetIconFont();
	void OnActionPressed(Hotkey::Action action) override;

	bool settingsActive = false;
	bool profilerActive = false;
private:
	// UserInterface(
	// 	Vulkan::CommandPool& commandPool,
	// 	const Vulkan::SwapChain& swapChain,
	// 	const Vulkan::DepthBuffer& depthBuffer,
	// 	UserSettings& userSettings);
	// ~UserInterface();

	static void setupImGuiStyle();

	void drawAllUI() const;
	void drawOverlay(const Statistics& statistics) const;


	std::unique_ptr<Vulkan::RenderPass> renderPass;


	float translation[3] = {}, rotation[3] = {}, scale[3] = {};
	bool isUsingImguizmo = false;

	static UIManager* sharedInstance;

	static bool isStartup; // ui manager already created ?
	static bool isHidingUI;
	bool isLoadingLayout = false;
	bool isLoadingDynamicLayout = false;
	bool isResettingLayout = false;
	UserSettings* userSettings = nullptr;
	const Vulkan::SwapChain* swapChain = nullptr;

	static bool wasUsingGizmoLastFrame;
	static TransformState gizmoBeforeState;
	
	static bool gizmoWasManipulated;

	ImFont* iconFont = nullptr;
	bool isCTRLHeld = false;
	ImGuizmo::OPERATION m_currentGizmoOperation = ImGuizmo::TRANSLATE;

	UIList uiList;
	UITable uiTable;

	glm::mat4 gizmoModelMatrix;

	String currentLayoutPath;
};

