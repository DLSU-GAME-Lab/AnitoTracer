#pragma once

#include <imgui.h>
#include <wrl/client.h>
#include <string>
#include <shobjidl.h> 
#include <vector>

#include <memory>
#include <stb_image.h>
#include "Assets/CubeMapTexture.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Vulkan/PipelineLayout.hpp"

namespace Vulkan
{
	class CommandPool;
	class DeviceMemory;
	class VkDecscriptorSet;
	class Image;
	class ImageView;
	class Sampler;
}

namespace Assets {

	class Texture;
	class TextureImage;
	class ButtonTexture {

		public:
			ButtonTexture(TextureImage* textureImage);
			~ButtonTexture();

			TextureImage* textureImage;
			VkDescriptorSet textureDset;
		private:
			
	};
}