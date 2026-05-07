#include "ButtonTexture.hpp"

#include "imgui_internal.h"
#include "imgui_impl_vulkan.h"
#include "Assets/Texture.hpp"
#include "Assets/TextureImage.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/Sampler.hpp"
#include <cstring>
#include <stdexcept>

Assets::ButtonTexture::ButtonTexture(TextureImage* textureImage)
{
	this->textureImage = textureImage;
	// Textures loaded from files are in SHADER_READ_ONLY_OPTIMAL, not GENERAL.
	// Registering with the wrong layout triggers VUID-vkCmdDraw-None-09600.
	this->textureDset = VK_NULL_HANDLE;
	if (ImGui::GetIO().BackendRendererUserData != nullptr)
	{
		this->textureDset = ImGui_ImplVulkan_AddTexture(
			textureImage->Sampler().Handle(),
			textureImage->ImageView().Handle(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

Assets::ButtonTexture::~ButtonTexture()
{

}
