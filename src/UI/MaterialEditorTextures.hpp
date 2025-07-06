#pragma once 
#include "ButtonTexture.hpp"
#include "UserSettings.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"

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

namespace Assets {
	class MaterialEditorTextures {

	public:
		MaterialEditorTextures();
		~MaterialEditorTextures();

		static void initialize();
		static MaterialEditorTextures* GetInstance();

		ButtonTexture getTexture();
		void setTexture(ButtonTexture* texture);
		void setTexture(int32_t textureId);
		bool UIReset = true;

	private:

		static MaterialEditorTextures* sharedInstance;

		ButtonTexture buttonTexture;
		
	};
}