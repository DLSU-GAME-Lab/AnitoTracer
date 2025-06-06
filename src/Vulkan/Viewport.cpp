#include "Viewport.h"
#include "Buffer.hpp"
#include "CommandPool.hpp"
#include "CommandBuffers.hpp"
#include "DebugUtilsMessenger.hpp"
#include "DepthBuffer.hpp"
#include "Device.hpp"
#include "Fence.hpp"
#include "FrameBuffer.hpp"
#include "GraphicsPipeline.hpp"
#include "Instance.hpp"
#include "PipelineLayout.hpp"
#include "RenderPass.hpp"
#include "Semaphore.hpp"
#include "Image.hpp"
#include "ImageMemoryBarrier.hpp"
#include "ImageView.hpp"
#include "SwapChain.hpp"

#include "Assets/Model.hpp"
#include "Assets/Scene.hpp"
#include "Assets/UniformBuffer.hpp"

#include "From-GDGRAP2/ModelManager.h"

#include <imgui.h>

#include <array>
#include <iostream>

#include "imgui_impl_vulkan.h"
#include "Engine/CameraSystem/CameraManager.h"
#include "From-GDGRAP2/Debug.h"

namespace Vulkan {
	Viewport::Viewport(const class SwapChain& swapChain, const class Assets::Scene& scene) :
		AUIScreen("Viewport"),
		swapChain_(swapChain),
		scene_(scene)
	{
		const auto& device = SwapChain().Device();

		sampler_.reset(new Vulkan::Sampler(device, Vulkan::SamplerConfig()));
		{ // Create Viewport Images
			outputImages_.resize(swapChain_.Images().size());
			outputImageMemory_.resize(swapChain_.Images().size());
			outputImageViews_.resize(swapChain_.Images().size());

			const auto extent = SwapChain().Extent();
			const auto format = SwapChain().Format();
			const auto tiling = VK_IMAGE_TILING_OPTIMAL;

			for (int i = 0; i < SwapChain().Images().size(); i++)
			{
				VkImageCreateInfo imageInfo = {};
				imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				imageInfo.imageType = VK_IMAGE_TYPE_2D;
				imageInfo.extent.width = extent.width;
				imageInfo.extent.height = extent.height;
				imageInfo.extent.depth = 1;
				imageInfo.mipLevels = 1;
				imageInfo.arrayLayers = 1;
				imageInfo.format = format;
				imageInfo.tiling = tiling;
				imageInfo.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				imageInfo.flags = 0; // Optional

				Check(vkCreateImage(device.Handle(), &imageInfo, nullptr, &outputImages_[i]),
					"create image");

				outputImageMemory_[i].reset(new DeviceMemory(AllocateImageMemory(outputImages_[i])));

				//outputImages_[i].reset(new Image(device, extent, format, tiling, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
				//outputImageMemory_[i].reset(new DeviceMemory(outputImages_[i]->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
				outputImageViews_[i].reset(new ImageView(SwapChain().Device(), outputImages_[i], format, VK_IMAGE_ASPECT_COLOR_BIT));
			}

			const auto& debugUtils = SwapChain().Device().DebugUtils();

			for (int i = 0; i < SwapChain().Images().size(); i++)
			{
				debugUtils.SetObjectName(outputImages_[i], "Viewport Image");
				debugUtils.SetObjectName(outputImageMemory_[i]->Handle(), "Viewport Image Memory");
				debugUtils.SetObjectName(outputImageViews_[i]->Handle(), "Viewport ImageView");
			}
		}

		commandPool_.reset(new class CommandPool(SwapChain().Device(), SwapChain().Device().GraphicsFamilyIndex(), true));
		depthBuffer_.reset(new class DepthBuffer(*commandPool_, SwapChain().Extent()));

		for (size_t i = 0; i != SwapChain().ImageViews().size(); ++i)
		{
			imageAvailableSemaphores_.emplace_back(SwapChain().Device());
			renderFinishedSemaphores_.emplace_back(SwapChain().Device());
			inFlightFences_.emplace_back(SwapChain().Device(), true);
			uniformBuffers_.emplace_back(SwapChain().Device(), sizeof(Assets::UniformBufferObject));
		}

		graphicsPipeline_.reset(new class GraphicsPipeline(swapChain_, *depthBuffer_, uniformBuffers_, scene_, isWireFrame_));

		frameBuffers_.reserve(outputImageViews_.size());
		for (const auto& imageView : outputImageViews_)
		{
			frameBuffers_.emplace_back(*imageView, graphicsPipeline_->RenderPass());
		}

		viewportDSet_.resize(outputImageViews_.size());
		for (uint32_t i = 0; i < outputImageViews_.size(); i++)
			viewportDSet_[i] = ImGui_ImplVulkan_AddTexture(
				sampler_->Handle(),
				outputImageViews_[i]->Handle(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	Viewport::~Viewport()
	{
	}

	Assets::UniformBufferObject Viewport::GetUniformBufferObject(VkExtent2D extent) const
	{
		return Assets::UniformBufferObject();
	}

	Assets::PushConstantModel Viewport::GetPushConstantModel(const Assets::Model& model) const
	{
		Assets::PushConstantModel ubo = {};
		ubo.WorldMatrix = model.GetWorldMatrix();

		return ubo;
	}

	void Viewport::RenderRasterized(VkCommandBuffer commandBuffer, const uint32_t imageIndex)
	{
		currentFrame_ = imageIndex;

		// Copy swap-chain image into output image.
		VkImageCopy copyRegion;
		copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		copyRegion.srcOffset = { 0, 0, 0 };
		copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		copyRegion.dstOffset = { 0, 0, 0 };
		copyRegion.extent = { swapChain_.Extent().width, swapChain_.Extent().height, 1 };

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.layerCount = 1;

		ImageMemoryBarrier::Insert(commandBuffer, SwapChain().Images()[imageIndex], subresourceRange,
			VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		ImageMemoryBarrier::Insert(commandBuffer, outputImages_[currentFrame_], subresourceRange,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		vkCmdCopyImage(commandBuffer,
			SwapChain().Images()[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			outputImages_[currentFrame_], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &copyRegion);

		ImageMemoryBarrier::Insert(commandBuffer, outputImages_[currentFrame_], subresourceRange,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		ImageMemoryBarrier::Insert(commandBuffer, SwapChain().Images()[imageIndex], subresourceRange,
			VK_ACCESS_TRANSFER_READ_BIT, 0,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	}

	void Viewport::RenderRayTraced(VkCommandBuffer commandBuffer, uint32_t imageIndex, const VkImage image)
	{
		currentFrame_ = imageIndex;

		// copy dat bitch
		VkImageCopy copyRegion = {};
		copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		copyRegion.srcOffset = { 0, 0, 0 };
		copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		copyRegion.dstOffset = { 0, 0, 0 };
		copyRegion.extent = { swapChain_.Extent().width, swapChain_.Extent().height, 1 };

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.layerCount = 1;

		// Transition ray tracing output image to transfer source layout
		ImageMemoryBarrier::Insert(commandBuffer, image, subresourceRange,
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		// Transition destination (viewport) image to TRANSFER_DST
		ImageMemoryBarrier::Insert(commandBuffer, outputImages_[currentFrame_], subresourceRange,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		vkCmdCopyImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			outputImages_[currentFrame_], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &copyRegion);

		ImageMemoryBarrier::Insert(commandBuffer, image, subresourceRange,
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

		// Transition destination image back to SHADER_READ_ONLY for ImGui
		ImageMemoryBarrier::Insert(commandBuffer, outputImages_[currentFrame_], subresourceRange,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}


	void Viewport::drawUI()
	{
		ImGui::Begin("Viewport");

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		ImGui::Image(reinterpret_cast<ImTextureID>(viewportDSet_[currentFrame_]), ImVec2{ viewportPanelSize.x, viewportPanelSize.y });
		ImGui::End();

	}

	DeviceMemory Viewport::AllocateImageMemory(VkImage image) const
	{
		VkMemoryRequirements requirements;
		vkGetImageMemoryRequirements(SwapChain().Device().Handle(), image, &requirements);

		DeviceMemory memory(SwapChain().Device(), requirements.size, requirements.memoryTypeBits, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		Check(vkBindImageMemory(SwapChain().Device().Handle(), image, memory.Handle(), 0),
			"bind image memory");

		return memory;
	}

	void Viewport::UpdateUniformBuffer(const uint32_t imageIndex)
	{
		uniformBuffers_[imageIndex].SetValue(GetUniformBufferObject(swapChain_.Extent()));
	}
}
