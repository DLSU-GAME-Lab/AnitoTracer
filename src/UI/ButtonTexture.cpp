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
	this->textureDset = ImGui_ImplVulkan_AddTexture(textureImage->Sampler().Handle(), textureImage->ImageView().Handle(), VK_IMAGE_LAYOUT_GENERAL);
}

Assets::ButtonTexture::~ButtonTexture()
{

}
