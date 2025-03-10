#pragma once
#include "Vulkan/Vulkan.hpp"
#include <memory>



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


class UserInterface final
{
public:

	VULKAN_NON_COPIABLE(UserInterface)

	UserInterface(
		Vulkan::CommandPool& commandPool, 
		const Vulkan::SwapChain& swapChain, 
		const Vulkan::DepthBuffer& depthBuffer,
		UserSettings& userSettings);
	~UserInterface();

	void render(VkCommandBuffer commandBuffer, const Vulkan::FrameBuffer& frameBuffer, const Statistics& statistics);

	static bool wantsToCaptureKeyboard();
	static bool wantsToCaptureMouse();

	UserSettings& settings() const { return userSettings; }

private:
	static void setupImGuiStyle();
	void drawOverlay(const Statistics& statistics) const;

	std::unique_ptr<Vulkan::DescriptorPool> descriptorPool;
	std::unique_ptr<Vulkan::RenderPass> renderPass;
	UserSettings& userSettings;

	const Vulkan::SwapChain& swapChain;

	float translation[3], rotation[3], scale[3];
	bool isUsingImguizmo = false;
};
