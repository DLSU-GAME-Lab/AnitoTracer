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
				imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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
				debugUtils.SetObjectName(outputImages_[i], "Output Image");
				debugUtils.SetObjectName(outputImageMemory_[i]->Handle(), "Output Image Memory");
				debugUtils.SetObjectName(outputImageViews_[i]->Handle(), "Output ImageView");
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

		for (const auto& imageView : outputImageViews_)
		{
			frameBuffers_.emplace_back(*imageView, graphicsPipeline_->RenderPass());
		}

		//dSet_.resize(outputImageViews_.size());
		//for (uint32_t i = 0; i < outputImageViews_.size(); i++)
		//	dSet_[i] = ImGui_ImplVulkan_AddTexture(scene_.TextureSamplers()[0], outputImageViews_[i]->Handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
		return Assets::PushConstantModel();
	}

	void Viewport::Render(VkCommandBuffer commandBuffer, const uint32_t imageIndex)
	{
		currentFrame_ = imageIndex;
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { {0.0f, 1.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = graphicsPipeline_->RenderPass().Handle();
		renderPassInfo.framebuffer = frameBuffers_[imageIndex].Handle();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { SwapChain().Extent().width, SwapChain().Extent().height };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			dSet_ = graphicsPipeline_->DescriptorSet(imageIndex);
			VkBuffer vertexBuffers[] = { scene_.VertexBuffer().Handle() };
			const VkBuffer indexBuffer = scene_.IndexBuffer().Handle();
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_->Handle());
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_->PipelineLayout().Handle(), 0, 1, &dSet_, 0, nullptr);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			uint32_t vertexOffset = 0;
			uint32_t indexOffset = 0;

			for (const auto& model : ModelManager::getInstance()->getAllObjectModels())
			{
				Assets::PushConstantModel modelConstant = GetPushConstantModel(model);
				vkCmdPushConstants(commandBuffer, graphicsPipeline_->PipelineLayout().Handle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Assets::PushConstantModel), &modelConstant);

				const auto vertexCount = static_cast<uint32_t>(model.NumberOfVertices());
				const auto indexCount = static_cast<uint32_t>(model.NumberOfIndices());

				vkCmdDrawIndexed(commandBuffer, indexCount, 1, indexOffset, vertexOffset, 0);

				vertexOffset += vertexCount;
				indexOffset += indexCount;

				ImGui_ImplVulkan_RemoveTexture(dSet_);
				dSet_ = ImGui_ImplVulkan_AddTexture(
					scene_.TextureSamplers()[0],
					outputImageViews_[imageIndex]->Handle(),
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}
		vkCmdEndRenderPass(commandBuffer);
	}

	void Viewport::drawUI()
	{
		ImGui::Begin("Viewport");

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		//Debug::Log("Viewport size: " + std::to_string(viewportPanelSize.x) + "x" + std::to_string(viewportPanelSize.y));
		ImGui::Image(reinterpret_cast<ImTextureID>(dSet_), ImVec2{ viewportPanelSize.x, viewportPanelSize.y });
		//ImGui::Image(reinterpret_cast<ImTextureID>(graphicsPipeline_->DescriptorSet(currentFrame_)), ImVec2{ viewportPanelSize.x, viewportPanelSize.y });
		//ImGui::Image(ImGui::GetIO().Fonts->TexID, ImVec2{viewportPanelSize.x, viewportPanelSize.y});

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
		uniformBuffers_[imageIndex].SetValue(GetUniformBufferObject({ 512, 512 }));
	}
}
