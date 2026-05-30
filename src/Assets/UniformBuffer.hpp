#pragma once

#include "Utilities/Glm.hpp"
#include <memory>

namespace Vulkan
{
	class Buffer;
	class Device;
	class DeviceMemory;
}

namespace Assets
{
	class UniformBufferObject
	{
	public:

		glm::mat4 ModelView;
		glm::mat4 Projection;
		glm::mat4 ModelViewInverse;
		glm::mat4 ProjectionInverse;
		float Aperture;
		float FocusDistance;
		float HeatmapScale;
		uint32_t TotalNumberOfSamples;
		uint32_t NumberOfSamples;
		uint32_t SamplesPerInvocation; // samples computed inside a single shader dispatch
		uint32_t NumberOfBounces;
		uint32_t RandomSeed;
		uint32_t MaxRays;
		uint32_t HasSky; // bool
		uint32_t ShowHeatmap; // bool
		uint32_t EnableAdaptiveSampling; // bool
		float VarianceThreshold;
		uint32_t MinSamples;
		uint32_t _padFAC0;               // std140 padding: align vec3 FallbackAmbientColor to 16-byte boundary
		uint32_t _padFAC1;               // std140 padding
		glm::vec3 FallbackAmbientColor;  // RGB fallback ambient when IBL is disabled
		float     Exposure;              // Scene exposure scalar applied to direct lighting before tonemapping

		// UseColorIBL: when non-zero, IBLSkyColor replaces cubemap sampling in IBLAmbient.
		// std140: uint (4 B) + 3×uint pad (12 B) → vec3 IBLSkyColor aligned to offset+16.
		uint32_t  UseColorIBL;           // bool — use flat IBLSkyColor instead of cubemap
		uint32_t  _padIBL0;              // std140 padding: align IBLSkyColor vec3 to 16-byte boundary
		uint32_t  _padIBL1;              // std140 padding
		uint32_t  _padIBL2;              // std140 padding
		glm::vec3 IBLSkyColor;           // flat sky tint used when UseColorIBL != 0
		float     _padIBLEnd;            // std140 padding: complete the vec4 slot
	};

	// might move to a push constant class
	class PushConstantModel
	{
	public:
		glm::mat4 WorldMatrix;
	};

	class UniformBuffer
	{
	public:

		UniformBuffer(const UniformBuffer&) = delete;
		UniformBuffer& operator = (const UniformBuffer&) = delete;
		UniformBuffer& operator = (UniformBuffer&&) = delete;

		explicit UniformBuffer(const Vulkan::Device& device, const size_t bufferSize);
		UniformBuffer(UniformBuffer&& other) noexcept;
		~UniformBuffer();

		const Vulkan::Buffer& Buffer() const { return *buffer_; }

		void SetValue(const UniformBufferObject& ubo);
		void SetValue(const PushConstantModel& ubo);

	private:

		std::unique_ptr<Vulkan::Buffer> buffer_;
		std::unique_ptr<Vulkan::DeviceMemory> memory_;
	};

}