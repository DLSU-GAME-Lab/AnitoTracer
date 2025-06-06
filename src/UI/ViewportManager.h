#pragma once

#include "UIManager.h"
#include <vector>

#include "Assets/UniformBuffer.hpp"

namespace Assets {
	class Scene;
}
namespace Vulkan {
	class Viewport;
	class SwapChain;
}

class ViewportManager
{
private:
	ViewportManager();
	~ViewportManager();
	ViewportManager(const ViewportManager&);
	ViewportManager& operator=(const ViewportManager&);

private:
	std::vector<Vulkan::Viewport*> viewports;
	std::unique_ptr<Vulkan::Viewport> viewport;

public:
	static ViewportManager* getInstance();
	static void initialize();
	static void destroy();

	void renderRasterizedScenes(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void renderRayTracedScenes(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkImage outputImage);
	void drawUI();

	void createViewport(const class Vulkan::SwapChain& swapChain, const class Assets::Scene& scene);
	void deleteViewport(Vulkan::Viewport* viewport);
	void deleteAllViewports();
	void addViewport(AUIScreen* viewport);
	void setNumViewports(int count);
	std::vector<Vulkan::Viewport*> getViewports();


private:
	static ViewportManager* P_SHARED_INSTANCE;
};