#pragma once

#include <array>
#include "Utilities/Glm.hpp"
#include "Vulkan/Vulkan.hpp"

namespace Assets
{
	struct RayVertex final
	{
		glm::vec3 Position;
		float pad;
		glm::vec4 Color;

		bool operator==(const RayVertex& other) const
		{
			return
				Color == other.Color &&
				Position == other.Position;
		}

		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(RayVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};


			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(RayVertex, Position);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(RayVertex, Color);

			return attributeDescriptions;
		}
	};
}