#include "Texture.hpp"
#include "Utilities/StbImage.hpp"
#include "Utilities/Exception.hpp"
#include "Utilities/FileUtils.h"
#include <chrono>
#include <iostream>

namespace Assets {

Texture Texture::LoadTexture(const std::string& filename, const Vulkan::SamplerConfig& samplerConfig)
{
	std::cout << "- loading '" << filename << "'... " << std::flush;
	const auto timer = std::chrono::high_resolution_clock::now();
	bool failed = false;

	// Load the texture in normal host memory.
	int width, height, channels;
	const auto pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

	if (!pixels)
	{
		failed = true;
	}


	if (failed) 
	{
		std::string defaultpath = FileUtils::getAssetsFolderPath().generic_string() + "/textures/white.png";
		const auto defaultpixels = stbi_load(defaultpath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		std::cout << "(Failed to load " << filename << ". Loading default texture) ";
		return Texture(width, height, channels, defaultpixels);
	}
	else 
	{
		const auto elapsed = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - timer).count();
		std::cout << "(" << width << " x " << height << " x " << channels << ") ";
		std::cout << elapsed << "s" << '\n';
		return Texture(width, height, channels, pixels);
	}
	

	
}

Texture::Texture(int width, int height, int channels, unsigned char* const pixels) :
	width_(width),
	height_(height),
	channels_(channels),
	pixels_(pixels, stbi_image_free)
{
}
	
}
