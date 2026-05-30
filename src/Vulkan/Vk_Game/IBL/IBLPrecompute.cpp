#include "IBLPrecompute.hpp"

#include "Utilities/Exception.hpp"
#include "Utilities/FileUtils.h"
#include "Vulkan/Device.hpp"
#include "Vulkan/DeviceMemory.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/SingleTimeCommands.hpp"

#include <array>
#include <stdexcept>
#include <string>

namespace Vulkan::Game
{

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

IBLPrecompute::IBLPrecompute(
	const Vulkan::Device& device,
	Vulkan::CommandPool&  commandPool,
	VkImageView           skyboxView,
	VkSampler             skyboxSampler)
: device_(device)
, skyboxView_(skyboxView)
, skyboxSampler_(skyboxSampler)
{
	// 1. Allocate GPU images + samplers
	CreateIrradianceResources();
	CreatePrefilteredResources();
	CreateBrdfLutResources();

	// 2. Build compute pipelines (descriptor sets already written inside)
	CreateIrradiancePipeline();
	CreatePrefilterPipeline();
	CreateBrdfLutPipeline();

	// 3. Record + submit all dispatches; waits idle before returning
	DispatchAll(commandPool);
}

IBLPrecompute::~IBLPrecompute()
{
	const VkDevice dev = device_.Handle();

	// ── Destroy per-face/mip storage image views ──────────────────────────
	for (VkImageView v : irradianceFaceViews_)
		if (v != VK_NULL_HANDLE) vkDestroyImageView(dev, v, nullptr);

	for (auto& faceArr : prefilteredMipFaceViews_)
		for (VkImageView v : faceArr)
			if (v != VK_NULL_HANDLE) vkDestroyImageView(dev, v, nullptr);

	// ── Compute pipelines ─────────────────────────────────────────────────
	if (irrPipeline_       != VK_NULL_HANDLE) vkDestroyPipeline(dev, irrPipeline_, nullptr);
	if (irrPipelineLayout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(dev, irrPipelineLayout_, nullptr);
	if (irrDSL_            != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(dev, irrDSL_, nullptr);
	if (irrDescPool_       != VK_NULL_HANDLE) vkDestroyDescriptorPool(dev, irrDescPool_, nullptr);

	if (prePipeline_       != VK_NULL_HANDLE) vkDestroyPipeline(dev, prePipeline_, nullptr);
	if (prePipelineLayout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(dev, prePipelineLayout_, nullptr);
	if (preDSL_            != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(dev, preDSL_, nullptr);
	if (preDescPool_       != VK_NULL_HANDLE) vkDestroyDescriptorPool(dev, preDescPool_, nullptr);

	if (lutPipeline_       != VK_NULL_HANDLE) vkDestroyPipeline(dev, lutPipeline_, nullptr);
	if (lutPipelineLayout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(dev, lutPipelineLayout_, nullptr);
	if (lutDSL_            != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(dev, lutDSL_, nullptr);
	if (lutDescPool_       != VK_NULL_HANDLE) vkDestroyDescriptorPool(dev, lutDescPool_, nullptr);

	// Owned image resources destroyed by their unique_ptr destructors
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

VkImageView IBLPrecompute::IrradianceView()     const { return irradianceView_->Handle(); }
VkSampler   IBLPrecompute::IrradianceSampler()  const { return irradianceSampler_->Handle(); }
VkImageView IBLPrecompute::PrefilteredView()    const { return prefilteredView_->Handle(); }
VkSampler   IBLPrecompute::PrefilteredSampler() const { return prefilteredSampler_->Handle(); }
VkImageView IBLPrecompute::BrdfLutView()        const { return brdfLutView_->Handle(); }
VkSampler   IBLPrecompute::BrdfLutSampler()     const { return brdfLutSampler_->Handle(); }

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateIrradianceResources
// ─────────────────────────────────────────────────────────────────────────────

void IBLPrecompute::CreateIrradianceResources()
{
	constexpr VkFormat fmt = VK_FORMAT_R16G16B16A16_SFLOAT;
	const VkExtent2D   ext = { kIrradianceSize, kIrradianceSize };

	// Cubemap image: 6 layers, 1 mip, STORAGE_BIT for compute write + SAMPLED_BIT
	irradianceImage_ = std::make_unique<Vulkan::Image>(
		device_, ext, fmt,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

	irradianceMem_ = std::make_unique<Vulkan::DeviceMemory>(
		irradianceImage_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	// Whole-cube sampler view (used by game_frag.frag binding 9)
	irradianceView_ = std::make_unique<Vulkan::ImageView>(
		device_, irradianceImage_->Handle(), fmt,
		VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE, 6);

	// Per-face 2D storage views (compute writes one face at a time)
	for (uint32_t f = 0; f < 6; ++f)
	{
		VkImageViewCreateInfo ci{};
		ci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.image            = irradianceImage_->Handle();
		ci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
		ci.format           = fmt;
		ci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, f, 1 };
		Check(vkCreateImageView(device_.Handle(), &ci, nullptr, &irradianceFaceViews_[f]),
			  "create irradiance face image view");
	}

	// Sampler: linear, clamp-to-edge, no mips
	SamplerConfig cfg{};
	cfg.MagFilter        = VK_FILTER_LINEAR;
	cfg.MinFilter        = VK_FILTER_LINEAR;
	cfg.AddressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	cfg.AddressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	cfg.AddressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	cfg.AnisotropyEnable = false;
	cfg.MipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	cfg.MaxLod           = 0.0f;
	irradianceSampler_   = std::make_unique<Vulkan::Sampler>(device_, cfg);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreatePrefilteredResources
// ─────────────────────────────────────────────────────────────────────────────

void IBLPrecompute::CreatePrefilteredResources()
{
	constexpr VkFormat fmt = VK_FORMAT_R16G16B16A16_SFLOAT;
	const VkExtent2D   ext = { kPrefilteredSize, kPrefilteredSize };

	// Cubemap with full mip chain (kPrefilteredMips levels)
	prefilteredImage_ = std::make_unique<Vulkan::Image>(
		device_, ext, fmt,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
		kPrefilteredMips);

	prefilteredMem_ = std::make_unique<Vulkan::DeviceMemory>(
		prefilteredImage_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	// Whole-cube + all-mips sampler view (binding 10 in game_frag.frag)
	prefilteredView_ = std::make_unique<Vulkan::ImageView>(
		device_, prefilteredImage_->Handle(), fmt,
		VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE, 6);

	// Per-(mip, face) 2D storage views for compute writes
	prefilteredMipFaceViews_.resize(kPrefilteredMips);
	for (uint32_t m = 0; m < kPrefilteredMips; ++m)
	{
		for (uint32_t f = 0; f < 6; ++f)
		{
			VkImageViewCreateInfo ci{};
			ci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ci.image            = prefilteredImage_->Handle();
			ci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
			ci.format           = fmt;
			ci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, m, 1, f, 1 };
			Check(vkCreateImageView(device_.Handle(), &ci, nullptr,
									&prefilteredMipFaceViews_[m][f]),
				  "create prefiltered mip-face image view");
		}
	}

	// Sampler: linear + mip-linear interpolation, clamp-to-edge
	SamplerConfig cfg{};
	cfg.MagFilter        = VK_FILTER_LINEAR;
	cfg.MinFilter        = VK_FILTER_LINEAR;
	cfg.MipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	cfg.AddressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	cfg.AddressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	cfg.AddressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	cfg.AnisotropyEnable = false;
	cfg.MaxLod           = static_cast<float>(kPrefilteredMips - 1);
	prefilteredSampler_  = std::make_unique<Vulkan::Sampler>(device_, cfg);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateBrdfLutResources
// ─────────────────────────────────────────────────────────────────────────────

void IBLPrecompute::CreateBrdfLutResources()
{
	constexpr VkFormat fmt = VK_FORMAT_R16G16_SFLOAT;
	const VkExtent2D   ext = { kBrdfLutSize, kBrdfLutSize };

	brdfLutImage_ = std::make_unique<Vulkan::Image>(
		device_, ext, fmt,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		1, 0 /*no createFlags*/);

	brdfLutMem_ = std::make_unique<Vulkan::DeviceMemory>(
		brdfLutImage_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	brdfLutView_ = std::make_unique<Vulkan::ImageView>(
		device_, brdfLutImage_->Handle(), fmt,
		VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

	// Sampler: linear, clamp-to-edge, no mips
	SamplerConfig cfg{};
	cfg.MagFilter        = VK_FILTER_LINEAR;
	cfg.MinFilter        = VK_FILTER_LINEAR;
	cfg.AddressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	cfg.AddressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	cfg.AddressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	cfg.AnisotropyEnable = false;
	cfg.MipmapMode       = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	cfg.MaxLod           = 0.0f;
	brdfLutSampler_      = std::make_unique<Vulkan::Sampler>(device_, cfg);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateIrradiancePipeline
// ─────────────────────────────────────────────────────────────────────────────

void IBLPrecompute::CreateIrradiancePipeline()
{
	const VkDevice dev = device_.Handle();

	// ── Descriptor set layout ─────────────────────────────────────────────────
	// binding 0 : samplerCube  (envMap — input skybox)
	// binding 1 : image2D      (outFace — output irradiance face, storage)
	const std::array<VkDescriptorSetLayoutBinding, 2> bindings = {{
		{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
		{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
	}};

	VkDescriptorSetLayoutCreateInfo dslInfo{};
	dslInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	dslInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	dslInfo.pBindings    = bindings.data();
	Check(vkCreateDescriptorSetLayout(dev, &dslInfo, nullptr, &irrDSL_),
		  "create irradiance DSL");

	// ── Descriptor pool (6 sets — one per face) ───────────────────────────────
	const std::array<VkDescriptorPoolSize, 2> poolSizes = {{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          6 },
	}};
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.maxSets       = 6;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes    = poolSizes.data();
	Check(vkCreateDescriptorPool(dev, &poolInfo, nullptr, &irrDescPool_),
		  "create irradiance descriptor pool");

	// ── Allocate + write one set per face ─────────────────────────────────────
	const std::array<VkDescriptorSetLayout, 6> layouts = {
		irrDSL_, irrDSL_, irrDSL_, irrDSL_, irrDSL_, irrDSL_
	};
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool     = irrDescPool_;
	allocInfo.descriptorSetCount = 6;
	allocInfo.pSetLayouts        = layouts.data();
	Check(vkAllocateDescriptorSets(dev, &allocInfo, irrSets_.data()),
		  "allocate irradiance descriptor sets");

	for (uint32_t f = 0; f < 6; ++f)
	{
		VkDescriptorImageInfo envInfo{};
		envInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		envInfo.imageView   = skyboxView_;
		envInfo.sampler     = skyboxSampler_;

		VkDescriptorImageInfo faceInfo{};
		faceInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		faceInfo.imageView   = irradianceFaceViews_[f];
		faceInfo.sampler     = VK_NULL_HANDLE;

		std::array<VkWriteDescriptorSet, 2> writes{};
		writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet          = irrSets_[f];
		writes[0].dstBinding      = 0;
		writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].descriptorCount = 1;
		writes[0].pImageInfo      = &envInfo;

		writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet          = irrSets_[f];
		writes[1].dstBinding      = 1;
		writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[1].descriptorCount = 1;
		writes[1].pImageInfo      = &faceInfo;

		vkUpdateDescriptorSets(dev, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}

	// ── Pipeline layout (push constant: uint faceIndex) ───────────────────────
	VkPushConstantRange pcRange{};
	pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pcRange.offset     = 0;
	pcRange.size       = sizeof(uint32_t);

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount         = 1;
	layoutInfo.pSetLayouts            = &irrDSL_;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges    = &pcRange;
	Check(vkCreatePipelineLayout(dev, &layoutInfo, nullptr, &irrPipelineLayout_),
		  "create irradiance pipeline layout");

	// ── Compute pipeline ──────────────────────────────────────────────────────
	const std::string shaderDir =
		FileUtils::getAssetsFolderPath().generic_string() + "/shaders/";
	const ShaderModule shader(device_, shaderDir + "ibl_irradiance.comp.spv");

	VkComputePipelineCreateInfo pipeInfo{};
	pipeInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipeInfo.stage  = shader.CreateShaderStage(VK_SHADER_STAGE_COMPUTE_BIT);
	pipeInfo.layout = irrPipelineLayout_;
	Check(vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &irrPipeline_),
		  "create irradiance compute pipeline");
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreatePrefilterPipeline
// ─────────────────────────────────────────────────────────────────────────────

void IBLPrecompute::CreatePrefilterPipeline()
{
	const VkDevice dev = device_.Handle();

	// Same layout as irradiance: binding 0 = samplerCube, binding 1 = storage image
	const std::array<VkDescriptorSetLayoutBinding, 2> bindings = {{
		{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
		{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
	}};
	VkDescriptorSetLayoutCreateInfo dslInfo{};
	dslInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	dslInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	dslInfo.pBindings    = bindings.data();
	Check(vkCreateDescriptorSetLayout(dev, &dslInfo, nullptr, &preDSL_),
		  "create prefilter DSL");

	// Pool: kPrefilteredMips × 6 sets
	const uint32_t totalSets = kPrefilteredMips * 6;
	const std::array<VkDescriptorPoolSize, 2> poolSizes = {{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, totalSets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          totalSets },
	}};
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.maxSets       = totalSets;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes    = poolSizes.data();
	Check(vkCreateDescriptorPool(dev, &poolInfo, nullptr, &preDescPool_),
		  "create prefilter descriptor pool");

	// Allocate all sets at once
	preSets_.resize(kPrefilteredMips);
	std::vector<VkDescriptorSetLayout> allLayouts(totalSets, preDSL_);
	std::vector<VkDescriptorSet>       flatSets(totalSets);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool     = preDescPool_;
	allocInfo.descriptorSetCount = totalSets;
	allocInfo.pSetLayouts        = allLayouts.data();
	Check(vkAllocateDescriptorSets(dev, &allocInfo, flatSets.data()),
		  "allocate prefilter descriptor sets");

	// Re-pack into [mip][face] and write descriptors
	for (uint32_t m = 0; m < kPrefilteredMips; ++m)
	{
		for (uint32_t f = 0; f < 6; ++f)
		{
			preSets_[m][f] = flatSets[m * 6 + f];

			VkDescriptorImageInfo envInfo{};
			envInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			envInfo.imageView   = skyboxView_;
			envInfo.sampler     = skyboxSampler_;

			VkDescriptorImageInfo faceInfo{};
			faceInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			faceInfo.imageView   = prefilteredMipFaceViews_[m][f];
			faceInfo.sampler     = VK_NULL_HANDLE;

			std::array<VkWriteDescriptorSet, 2> writes{};
			writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].dstSet          = preSets_[m][f];
			writes[0].dstBinding      = 0;
			writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].descriptorCount = 1;
			writes[0].pImageInfo      = &envInfo;

			writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[1].dstSet          = preSets_[m][f];
			writes[1].dstBinding      = 1;
			writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writes[1].descriptorCount = 1;
			writes[1].pImageInfo      = &faceInfo;

			vkUpdateDescriptorSets(dev, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
		}
	}

	// Push constants: faceIndex (uint) + roughness (float) = 8 bytes
	VkPushConstantRange pcRange{};
	pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pcRange.offset     = 0;
	pcRange.size       = sizeof(uint32_t) + sizeof(float); // 8 bytes

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount         = 1;
	layoutInfo.pSetLayouts            = &preDSL_;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges    = &pcRange;
	Check(vkCreatePipelineLayout(dev, &layoutInfo, nullptr, &prePipelineLayout_),
		  "create prefilter pipeline layout");

	const std::string shaderDir =
		FileUtils::getAssetsFolderPath().generic_string() + "/shaders/";
	const ShaderModule shader(device_, shaderDir + "ibl_prefilter.comp.spv");

	VkComputePipelineCreateInfo pipeInfo{};
	pipeInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipeInfo.stage  = shader.CreateShaderStage(VK_SHADER_STAGE_COMPUTE_BIT);
	pipeInfo.layout = prePipelineLayout_;
	Check(vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &prePipeline_),
		  "create prefilter compute pipeline");
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateBrdfLutPipeline
// ─────────────────────────────────────────────────────────────────────────────

void IBLPrecompute::CreateBrdfLutPipeline()
{
	const VkDevice dev = device_.Handle();

	// Only one binding: output storage image (no input sampler needed)
	const VkDescriptorSetLayoutBinding binding{
		0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
	};
	VkDescriptorSetLayoutCreateInfo dslInfo{};
	dslInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	dslInfo.bindingCount = 1;
	dslInfo.pBindings    = &binding;
	Check(vkCreateDescriptorSetLayout(dev, &dslInfo, nullptr, &lutDSL_),
		  "create BRDF LUT DSL");

	const VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 };
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.maxSets       = 1;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes    = &poolSize;
	Check(vkCreateDescriptorPool(dev, &poolInfo, nullptr, &lutDescPool_),
		  "create BRDF LUT descriptor pool");

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool     = lutDescPool_;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts        = &lutDSL_;
	Check(vkAllocateDescriptorSets(dev, &allocInfo, &lutSet_),
		  "allocate BRDF LUT descriptor set");

	VkDescriptorImageInfo imgInfo{};
	imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imgInfo.imageView   = brdfLutView_->Handle();
	imgInfo.sampler     = VK_NULL_HANDLE;

	VkWriteDescriptorSet write{};
	write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet          = lutSet_;
	write.dstBinding      = 0;
	write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	write.descriptorCount = 1;
	write.pImageInfo      = &imgInfo;
	vkUpdateDescriptorSets(dev, 1, &write, 0, nullptr);

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts    = &lutDSL_;
	Check(vkCreatePipelineLayout(dev, &layoutInfo, nullptr, &lutPipelineLayout_),
		  "create BRDF LUT pipeline layout");

	const std::string shaderDir =
		FileUtils::getAssetsFolderPath().generic_string() + "/shaders/";
	const ShaderModule shader(device_, shaderDir + "ibl_brdf_lut.comp.spv");

	VkComputePipelineCreateInfo pipeInfo{};
	pipeInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipeInfo.stage  = shader.CreateShaderStage(VK_SHADER_STAGE_COMPUTE_BIT);
	pipeInfo.layout = lutPipelineLayout_;
	Check(vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &lutPipeline_),
		  "create BRDF LUT compute pipeline");
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — Transition helpers
// ─────────────────────────────────────────────────────────────────────────────

void IBLPrecompute::TransitionToGeneral(VkCommandBuffer cmd, VkImage image,
										uint32_t arrayLayers, uint32_t mipLevels) const
{
	VkImageMemoryBarrier barrier{};
	barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout           = VK_IMAGE_LAYOUT_GENERAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = image;
	barrier.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, arrayLayers };
	barrier.srcAccessMask       = 0;
	barrier.dstAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;

	vkCmdPipelineBarrier(cmd,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void IBLPrecompute::TransitionToShaderRead(VkCommandBuffer cmd, VkImage image,
										   uint32_t arrayLayers, uint32_t mipLevels) const
{
	VkImageMemoryBarrier barrier{};
	barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout           = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = image;
	barrier.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, arrayLayers };
	barrier.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — DispatchAll
// ─────────────────────────────────────────────────────────────────────────────

void IBLPrecompute::DispatchAll(Vulkan::CommandPool& commandPool)
{
	SingleTimeCommands::Submit(commandPool, [this](VkCommandBuffer cmd)
	{
		// ── 1. Irradiance convolution ─────────────────────────────────────────
		TransitionToGeneral(cmd, irradianceImage_->Handle(), 6, 1);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, irrPipeline_);

		for (uint32_t f = 0; f < 6; ++f)
		{
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
									irrPipelineLayout_, 0, 1, &irrSets_[f], 0, nullptr);

			const uint32_t faceIdx = f;
			vkCmdPushConstants(cmd, irrPipelineLayout_,
							   VK_SHADER_STAGE_COMPUTE_BIT, 0,
							   sizeof(uint32_t), &faceIdx);

			const uint32_t groups = (kIrradianceSize + 7u) / 8u;
			vkCmdDispatch(cmd, groups, groups, 1);

			// Memory barrier between faces (same image, different layers)
			VkMemoryBarrier mb{};
			mb.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			mb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			mb.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, 1, &mb, 0, nullptr, 0, nullptr);
		}

		TransitionToShaderRead(cmd, irradianceImage_->Handle(), 6, 1);

		// ── 2. Specular prefilter ─────────────────────────────────────────────
		TransitionToGeneral(cmd, prefilteredImage_->Handle(), 6, kPrefilteredMips);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, prePipeline_);

		for (uint32_t m = 0; m < kPrefilteredMips; ++m)
		{
			const float    roughness = static_cast<float>(m) / static_cast<float>(kPrefilteredMips - 1);
			const uint32_t mipSize   = kPrefilteredSize >> m;
			const uint32_t groups    = std::max((mipSize + 7u) / 8u, 1u);

			struct PrePC { uint32_t face; float roughness; };

			for (uint32_t f = 0; f < 6; ++f)
			{
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
										prePipelineLayout_, 0, 1, &preSets_[m][f], 0, nullptr);

				const PrePC pc{ f, roughness };
				vkCmdPushConstants(cmd, prePipelineLayout_,
								   VK_SHADER_STAGE_COMPUTE_BIT, 0,
								   sizeof(PrePC), &pc);

				vkCmdDispatch(cmd, groups, groups, 1);

				VkMemoryBarrier mb{};
				mb.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				mb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				mb.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				vkCmdPipelineBarrier(cmd,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0, 1, &mb, 0, nullptr, 0, nullptr);
			}
		}

		TransitionToShaderRead(cmd, prefilteredImage_->Handle(), 6, kPrefilteredMips);

		// ── 3. BRDF LUT ───────────────────────────────────────────────────────
		TransitionToGeneral(cmd, brdfLutImage_->Handle(), 1, 1);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, lutPipeline_);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
								lutPipelineLayout_, 0, 1, &lutSet_, 0, nullptr);

		const uint32_t lutGroups = (kBrdfLutSize + 7u) / 8u;
		vkCmdDispatch(cmd, lutGroups, lutGroups, 1);

		TransitionToShaderRead(cmd, brdfLutImage_->Handle(), 1, 1);
	});
}

} // namespace Vulkan::Game
