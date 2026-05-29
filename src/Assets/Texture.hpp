#pragma once

#include "Vulkan/Sampler.hpp"
#include <memory>
#include <string>
#include <vector>

namespace Assets
{

	class Texture final
	{
	public:

		static Texture LoadTexture(const std::string& filename, const Vulkan::SamplerConfig& samplerConfig);

		Texture& operator = (const Texture&) = delete;
		Texture& operator = (Texture&&) = delete;

		Texture() = default;
		Texture(const Texture&) = default;
		Texture(Texture&&) = default;
		~Texture() = default;

		const unsigned char* Pixels() const { return pixels_.get(); }
		int Width() const { return width_; }
		int Height() const { return height_; }
		int Channels() const { return channels_; }
		std::string Name() { return name_; }
		void setName(std::string texName) { name_ = texName; }

		Texture(int width, int height, int channels, unsigned char* const pixels, std::string name);

	private:

		Vulkan::SamplerConfig samplerConfig_;
		int width_;
		int height_;
		int channels_;
		std::string name_;
		std::shared_ptr<unsigned char> pixels_;
	};

}
