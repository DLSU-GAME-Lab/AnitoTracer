#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include "imgui.h"
#include "AUIScreen.h"
#include "UserSettings.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"

#include "Vulkan/Vulkan.hpp"
#include <memory>

#include "Engine/Profiler/Profiler.h"

typedef std::string String;

namespace Vulkan
{
	class CommandPool;
	class DepthBuffer;
	class DescriptorPool;
	class FrameBuffer;
	class RenderPass;
	class SwapChain;
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
class UIManager
{
public:
	typedef std::string String;
	typedef std::vector<std::shared_ptr<AUIScreen>> UIList;
	typedef std::unordered_map<String, std::shared_ptr<AUIScreen>> UITable;

	VULKAN_NON_COPIABLE(UIManager)

	UIManager();
	~UIManager();

	static UIManager* getInstance();
	static void initialize(Vulkan::CommandPool* commandPool, const Vulkan::SwapChain* swapChain, const Vulkan::DepthBuffer* depthBuffer, UserSettings* userSettings);
	static void reset();

	void initializeUI();
	static void saveLayout();
	void loadLayout();
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
	void SetProfiler(GpuCpuProfiler* profiler) { this->profiler = profiler; }

	// fucky test code below vvv
	//std::vector<VkImage>* images = nullptr;
	// const Vulkan::Device* device = nullptr;
	// Vulkan::Sampler* sampler = nullptr;
	// Vulkan::ImageView* imageView = nullptr;
	// VkDescriptorSet m_Dset;
	//Vulkan::Image* image = nullptr;
	// fucky test code above ^^^

	void render(VkCommandBuffer commandBuffer, const Vulkan::FrameBuffer& frameBuffer, const Statistics& statistics);

	static bool wantsToCaptureKeyboard();
	static bool wantsToCaptureMouse();

private:
	// UserInterface(
	// 	Vulkan::CommandPool& commandPool,
	// 	const Vulkan::SwapChain& swapChain,
	// 	const Vulkan::DepthBuffer& depthBuffer,
	// 	UserSettings& userSettings);
	// ~UserInterface();

	static void setupImGuiStyle();

	void drawDockspace() const;
	void drawAllUI() const;
	void drawOverlay(const Statistics& statistics) const;

	std::unique_ptr<Vulkan::DescriptorPool> descriptorPool;
	std::unique_ptr<Vulkan::RenderPass> renderPass;


	float translation[3] = {}, rotation[3] = {}, scale[3] = {};
	bool isUsingImguizmo = false;

	static UIManager* sharedInstance;

	bool isHidingUI = false;
	bool isLoadingLayout = false;
	bool isResettingLayout = false;
	UserSettings* userSettings = nullptr;
	const Vulkan::SwapChain* swapChain = nullptr;

	UIList uiList;
	UITable uiTable;
};

