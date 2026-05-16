#include "ShadowMapPass.hpp"

#include "Assets/Scene.hpp"
#include "Assets/Vertex.hpp"
#include "Assets/Model.hpp"
#include "From-GDGRAP2/ModelManager.h"
#include "From-GDGRAP2/GameObject.h"
#include "Utilities/FileUtils.h"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/DescriptorBinding.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/DescriptorSetManager.hpp"
#include "Vulkan/DescriptorSets.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/DeviceMemory.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/ShaderModule.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace Vulkan::Game
{

// ─────────────────────────────────────────────────────────────────────────────
// Internal push-constant layout for the shadow vertex shader
// ─────────────────────────────────────────────────────────────────────────────

struct ShadowPushConstant
{
glm::mat4 WorldMatrix;  // 64 bytes
uint32_t  LightIndex;   //  4 bytes  →  total 68 bytes (≤ 128-byte minimum)
};
static_assert(sizeof(ShadowPushConstant) <= 128,
              "ShadowPushConstant exceeds the guaranteed Vulkan minimum of 128 bytes");

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

ShadowMapPass::ShadowMapPass(const Vulkan::Device& device,
                             const uint32_t        imageCount,
                             ShadowMapSettings     settings)
: device_(device)
, settings_(std::move(settings))
{
CreateDepthResources();
CreateRenderPass();
CreateFramebuffers();
CreateDescriptorSets(imageCount);
CreatePipeline();
}

ShadowMapPass::~ShadowMapPass()
{
// Reverse construction order.
if (pipeline_ != VK_NULL_HANDLE)
vkDestroyPipeline(device_.Handle(), pipeline_, nullptr);

if (pipelineLayout_ != VK_NULL_HANDLE)
vkDestroyPipelineLayout(device_.Handle(), pipelineLayout_, nullptr);

descriptorSetManager_.reset();
lightVPBuffers_.clear();
lightVPMemories_.clear();

// Destroy per-slot framebuffers before the render pass they reference.
for (auto& layer : layers_)
{
if (layer.Framebuffer != VK_NULL_HANDLE)
vkDestroyFramebuffer(device_.Handle(), layer.Framebuffer, nullptr);
}
layers_.clear();

if (renderPass_ != VK_NULL_HANDLE)
vkDestroyRenderPass(device_.Handle(), renderPass_, nullptr);

shadowSampler_.reset();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public — UpdateLightVP
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::UpdateLightVP(const uint32_t imageIndex, const Assets::Scene& scene)
{
// ── Collect world-space scene AABB (used for every light frustum) ─────────
constexpr float kBig = std::numeric_limits<float>::max();
glm::vec3 sceneMin( kBig,  kBig,  kBig);
glm::vec3 sceneMax(-kBig, -kBig, -kBig);

for (GameObject* go : ModelManager::getInstance()->getObjectList())
{
if (!go) continue;
const glm::vec3 pos = glm::vec3(go->getWorldMatrix()[3]);
sceneMin = glm::min(sceneMin, pos);
sceneMax = glm::max(sceneMax, pos);
}

if (sceneMin.x > sceneMax.x)
{
sceneMin = glm::vec3(-100.0f);
sceneMax = glm::vec3( 100.0f);
}

sceneMin -= glm::vec3(settings_.SceneMargin);
sceneMax += glm::vec3(settings_.SceneMargin);

// ── Fill ShadowUBO with one VP per directional light (up to kMaxShadowLights) ──
ShadowUBO ubo{};

for (const auto& light : scene.Lights())
{
if (light.LightType != Assets::LightProperties::Enum::DirectionalLight)
continue;

const glm::vec3 rawDir = glm::vec3(light.LightDir);
const glm::vec3 lightDir = (glm::length(rawDir) > 0.0001f)
                         ? glm::normalize(rawDir)
                         : glm::vec3(0.0f, -1.0f, 0.0f);

ubo.LightViewProj[ubo.Count] = ComputeLightVP(lightDir, sceneMin, sceneMax);
++ubo.Count;

if (ubo.Count >= kMaxShadowLights)
break;
}

// Cache for Render() to know how many sub-passes to record.
activeLightCount_ = ubo.Count;

void* data = lightVPMemories_[imageIndex].Map(0, sizeof(ShadowUBO));
std::memcpy(data, &ubo, sizeof(ShadowUBO));
lightVPMemories_[imageIndex].Unmap();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public — Render
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::Render(
VkCommandBuffer      commandBuffer,
const uint32_t       imageIndex,
const Assets::Scene& scene)
{
const VkClearValue clearDepth{ .depthStencil = { 1.0f, 0 } };

VkRenderPassBeginInfo rpInfo{};
rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
rpInfo.renderPass        = renderPass_;
rpInfo.renderArea.offset = { 0, 0 };
rpInfo.renderArea.extent = { settings_.Resolution, settings_.Resolution };
rpInfo.clearValueCount   = 1;
rpInfo.pClearValues      = &clearDepth;

VkViewport vp{};
vp.x        = 0.0f;
vp.y        = 0.0f;
vp.width    = static_cast<float>(settings_.Resolution);
vp.height   = static_cast<float>(settings_.Resolution);
vp.minDepth = 0.0f;
vp.maxDepth = 1.0f;

VkRect2D sc{};
sc.offset = { 0, 0 };
sc.extent = { settings_.Resolution, settings_.Resolution };

VkDescriptorSet ds = descriptorSetManager_->DescriptorSets().Handle(imageIndex);

VkBuffer     vbuf[]    = { scene.VertexBuffer().Handle() };
VkDeviceSize offsets[] = { 0 };

// ── One sub-pass per active shadow-casting directional light ──────────────
for (uint32_t li = 0; li < kMaxShadowLights; ++li)
{
auto& layer = layers_[li];

// ── Transition → DEPTH_STENCIL_ATTACHMENT_OPTIMAL ────────────────────
TransitionDepthImage(commandBuffer, layer,
                     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

rpInfo.framebuffer = layer.Framebuffer;
vkCmdBeginRenderPass(commandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

// Only draw scene geometry for active light slots; inactive slots are
// rendered as empty passes so depth clears to 1.0 (fully lit) and the
// descriptor slot holds a valid, fully-lit shadow map.
if (li < activeLightCount_)
{
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
vkCmdSetViewport(commandBuffer, 0, 1, &vp);
vkCmdSetScissor(commandBuffer, 0, 1, &sc);
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelineLayout_, 0, 1, &ds, 0, nullptr);
vkCmdBindVertexBuffers(commandBuffer, 0, 1, vbuf, offsets);
vkCmdBindIndexBuffer(commandBuffer, scene.IndexBuffer().Handle(), 0,
                     VK_INDEX_TYPE_UINT32);

uint32_t vertexOffset = 0;
uint32_t indexOffset  = 0;

for (GameObject* go : ModelManager::getInstance()->getObjectList())
{
if (!go || !go->getModel()) continue;

// Skip purely emissive meshes (ray-tracer area lights)
const auto& mats = go->getModel()->Materials();
const bool allEmissive = !mats.empty() && std::all_of(
mats.begin(), mats.end(),
[](const Assets::Material& m)
{ return m.MaterialModel == Assets::Material::Enum::DiffuseLight; });

const uint32_t idxCount = static_cast<uint32_t>(go->getModel()->NumberOfIndices());
const uint32_t vtxCount = static_cast<uint32_t>(go->getModel()->NumberOfVertices());

if (!allEmissive)
{
ShadowPushConstant pc{};
pc.WorldMatrix = go->getWorldMatrix();
pc.LightIndex  = li;

vkCmdPushConstants(commandBuffer, pipelineLayout_,
                   VK_SHADER_STAGE_VERTEX_BIT, 0,
                   sizeof(ShadowPushConstant), &pc);

vkCmdDrawIndexed(commandBuffer, idxCount, 1,
                 indexOffset, static_cast<int32_t>(vertexOffset), 0);
}

vertexOffset += vtxCount;
indexOffset  += idxCount;
}
}

vkCmdEndRenderPass(commandBuffer);

// ── Transition → SHADER_READ_ONLY_OPTIMAL for the main pass ──────────
TransitionDepthImage(commandBuffer, layer,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
}

// ─────────────────────────────────────────────────────────────────────────────
// Public — Accessors
// ─────────────────────────────────────────────────────────────────────────────

std::vector<VkImageView> ShadowMapPass::ShadowImageViews() const
{
std::vector<VkImageView> views;
views.reserve(layers_.size());
for (const auto& layer : layers_)
views.push_back(layer.ImageView->Handle());
return views;
}

VkSampler ShadowMapPass::ShadowSampler() const
{
return shadowSampler_->Handle();
}

const Vulkan::Buffer& ShadowMapPass::LightVPBuffer(const uint32_t imageIndex) const
{
return *lightVPBuffers_[imageIndex];
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateDepthResources
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::CreateDepthResources()
{
const VkFormat   fmt    = settings_.DepthFormat;
const VkExtent2D extent = { settings_.Resolution, settings_.Resolution };

layers_.resize(kMaxShadowLights);

for (auto& layer : layers_)
{
layer.Image = std::make_unique<Vulkan::Image>(
device_, extent, fmt,
VK_IMAGE_TILING_OPTIMAL,
VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

layer.Memory = std::make_unique<Vulkan::DeviceMemory>(
layer.Image->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

layer.ImageView = std::make_unique<Vulkan::ImageView>(
device_, layer.Image->Handle(), fmt, VK_IMAGE_ASPECT_DEPTH_BIT);
}

// ── Shared sampler for all shadow maps ────────────────────────────────────
// CLAMP_TO_BORDER + FLOAT_OPAQUE_WHITE: pixels outside the shadow frustum
// are treated as fully lit (compare returns 1.0 = not in shadow).
Vulkan::SamplerConfig cfg{};
cfg.MagFilter        = settings_.MagFilter;
cfg.MinFilter        = settings_.MinFilter;
cfg.AddressModeU     = settings_.AddressMode;
cfg.AddressModeV     = settings_.AddressMode;
cfg.AddressModeW     = settings_.AddressMode;
cfg.BorderColor      = settings_.BorderColor;
cfg.AnisotropyEnable = false;
cfg.CompareEnable    = true;
cfg.CompareOp        = settings_.CompareOp;
cfg.MipmapMode       = settings_.MipmapMode;

shadowSampler_ = std::make_unique<Vulkan::Sampler>(device_, cfg);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateRenderPass
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::CreateRenderPass()
{
// Single depth attachment, no colour.
// initialLayout = DEPTH_STENCIL_ATTACHMENT_OPTIMAL because our barriers
// transition the image before vkCmdBeginRenderPass is called.
VkAttachmentDescription depthAtt{};
depthAtt.format         = settings_.DepthFormat;
depthAtt.samples        = VK_SAMPLE_COUNT_1_BIT;
depthAtt.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
depthAtt.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
depthAtt.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
depthAtt.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
depthAtt.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

VkAttachmentReference depthRef{};
depthRef.attachment = 0;
depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

VkSubpassDescription subpass{};
subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
subpass.colorAttachmentCount    = 0;
subpass.pDepthStencilAttachment = &depthRef;

std::array<VkSubpassDependency, 2> deps{};

deps[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
deps[0].dstSubpass      = 0;
deps[0].srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
deps[0].dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
deps[0].srcAccessMask   = VK_ACCESS_SHADER_READ_BIT;
deps[0].dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

deps[1].srcSubpass      = 0;
deps[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
deps[1].srcStageMask    = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
deps[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
deps[1].srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
deps[1].dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

VkRenderPassCreateInfo rpInfo{};
rpInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
rpInfo.attachmentCount = 1;
rpInfo.pAttachments    = &depthAtt;
rpInfo.subpassCount    = 1;
rpInfo.pSubpasses      = &subpass;
rpInfo.dependencyCount = static_cast<uint32_t>(deps.size());
rpInfo.pDependencies   = deps.data();

Check(vkCreateRenderPass(device_.Handle(), &rpInfo, nullptr, &renderPass_),
      "create shadow map render pass");
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateFramebuffers
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::CreateFramebuffers()
{
for (auto& layer : layers_)
{
const VkImageView attachments[] = { layer.ImageView->Handle() };

VkFramebufferCreateInfo fbInfo{};
fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
fbInfo.renderPass      = renderPass_;
fbInfo.attachmentCount = 1;
fbInfo.pAttachments    = attachments;
fbInfo.width           = settings_.Resolution;
fbInfo.height          = settings_.Resolution;
fbInfo.layers          = 1;

Check(vkCreateFramebuffer(device_.Handle(), &fbInfo, nullptr, &layer.Framebuffer),
      "create shadow map framebuffer");
}
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateDescriptorSets
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::CreateDescriptorSets(const uint32_t imageCount)
{
const std::vector<DescriptorBinding> bindings =
{
{ 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT }
};

descriptorSetManager_ = std::make_unique<DescriptorSetManager>(
device_, bindings, imageCount);

auto& sets = descriptorSetManager_->DescriptorSets();

lightVPBuffers_.reserve(imageCount);
lightVPMemories_.reserve(imageCount);

for (uint32_t i = 0; i < imageCount; ++i)
{
lightVPBuffers_.push_back(std::make_unique<Vulkan::Buffer>(
device_, sizeof(ShadowUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));

lightVPMemories_.push_back(
lightVPBuffers_.back()->AllocateMemory(
VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

const ShadowUBO defaultUBO{};
void* data = lightVPMemories_.back().Map(0, sizeof(ShadowUBO));
std::memcpy(data, &defaultUBO, sizeof(ShadowUBO));
lightVPMemories_.back().Unmap();

VkDescriptorBufferInfo bufInfo{};
bufInfo.buffer = lightVPBuffers_.back()->Handle();
bufInfo.offset = 0;
bufInfo.range  = sizeof(ShadowUBO);

const std::vector<VkWriteDescriptorSet> writes = { sets.Bind(i, 0, bufInfo) };
sets.UpdateDescriptors(i, writes);
}
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreatePipeline
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::CreatePipeline()
{
const std::string shaderDir =
FileUtils::getAssetsFolderPath().generic_string() + "/shaders/";
const ShaderModule vertShader(device_, shaderDir + "shadow_vert.vert.spv");

const VkPipelineShaderStageCreateInfo vertStage =
vertShader.CreateShaderStage(VK_SHADER_STAGE_VERTEX_BIT);

const auto bindingDesc    = Assets::Vertex::GetBindingDescription();
const auto attributeDescs = Assets::Vertex::GetAttributeDescriptions();

VkPipelineVertexInputStateCreateInfo vertInput{};
vertInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
vertInput.vertexBindingDescriptionCount   = 1;
vertInput.pVertexBindingDescriptions      = &bindingDesc;
vertInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
vertInput.pVertexAttributeDescriptions    = attributeDescs.data();

VkPipelineInputAssemblyStateCreateInfo ia{};
ia.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
ia.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
ia.primitiveRestartEnable = VK_FALSE;

const std::array<VkDynamicState, 2> dynStates = {
VK_DYNAMIC_STATE_VIEWPORT,
VK_DYNAMIC_STATE_SCISSOR
};
VkPipelineDynamicStateCreateInfo dynState{};
dynState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
dynState.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
dynState.pDynamicStates    = dynStates.data();

VkPipelineViewportStateCreateInfo vpState{};
vpState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
vpState.viewportCount = 1;
vpState.scissorCount  = 1;

VkPipelineRasterizationStateCreateInfo rast{};
rast.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
rast.depthClampEnable        = VK_FALSE;
rast.rasterizerDiscardEnable = VK_FALSE;
rast.polygonMode             = VK_POLYGON_MODE_FILL;
rast.lineWidth               = 1.0f;
rast.cullMode                = settings_.CullMode;
rast.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
rast.depthBiasEnable         = settings_.DepthBiasEnable ? VK_TRUE : VK_FALSE;
rast.depthBiasConstantFactor = settings_.DepthBiasConstantFactor;
rast.depthBiasSlopeFactor    = settings_.DepthBiasSlopeFactor;
rast.depthBiasClamp          = settings_.DepthBiasClamp;

VkPipelineMultisampleStateCreateInfo ms{};
ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
ms.sampleShadingEnable  = VK_FALSE;
ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

VkPipelineDepthStencilStateCreateInfo ds{};
ds.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
ds.depthTestEnable  = VK_TRUE;
ds.depthWriteEnable = VK_TRUE;
ds.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;

VkPipelineColorBlendStateCreateInfo blend{};
blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
blend.logicOpEnable   = VK_FALSE;
blend.attachmentCount = 0;
blend.pAttachments    = nullptr;

const VkDescriptorSetLayout dsl =
descriptorSetManager_->DescriptorSetLayout().Handle();

// Push constant: WorldMatrix (64 B) + LightIndex (4 B) = 68 bytes.
VkPushConstantRange pcRange{};
pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
pcRange.offset     = 0;
pcRange.size       = sizeof(ShadowPushConstant);

VkPipelineLayoutCreateInfo layoutInfo{};
layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
layoutInfo.setLayoutCount         = 1;
layoutInfo.pSetLayouts            = &dsl;
layoutInfo.pushConstantRangeCount = 1;
layoutInfo.pPushConstantRanges    = &pcRange;

Check(vkCreatePipelineLayout(device_.Handle(), &layoutInfo, nullptr, &pipelineLayout_),
      "create shadow map pipeline layout");

VkGraphicsPipelineCreateInfo pipeInfo{};
pipeInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
pipeInfo.stageCount          = 1;
pipeInfo.pStages             = &vertStage;
pipeInfo.pVertexInputState   = &vertInput;
pipeInfo.pInputAssemblyState = &ia;
pipeInfo.pViewportState      = &vpState;
pipeInfo.pRasterizationState = &rast;
pipeInfo.pMultisampleState   = &ms;
pipeInfo.pDepthStencilState  = &ds;
pipeInfo.pColorBlendState    = &blend;
pipeInfo.pDynamicState       = &dynState;
pipeInfo.layout              = pipelineLayout_;
pipeInfo.renderPass          = renderPass_;
pipeInfo.subpass             = 0;

Check(vkCreateGraphicsPipelines(device_.Handle(), VK_NULL_HANDLE,
                                1, &pipeInfo, nullptr, &pipeline_),
      "create shadow map graphics pipeline");
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — ComputeLightVP
// ─────────────────────────────────────────────────────────────────────────────

glm::mat4 ShadowMapPass::ComputeLightVP(const glm::vec3  lightDir,
                                        const glm::vec3& sceneMin,
                                        const glm::vec3& sceneMax) const
{
const glm::vec3 sceneCenter = (sceneMin + sceneMax) * 0.5f;
const float     sceneRadius = glm::length(sceneMax - sceneCenter);

// Choose an up vector not collinear with lightDir.
const glm::vec3 up = (std::abs(lightDir.y) < 0.99f)
                   ? glm::vec3(0.0f, 1.0f, 0.0f)
                   : glm::vec3(1.0f, 0.0f, 0.0f);

const glm::vec3 eye  = sceneCenter - lightDir * sceneRadius;
const glm::mat4 view = glm::lookAt(eye, sceneCenter, up);

glm::mat4 proj = glm::ortho(
-sceneRadius, sceneRadius,
-sceneRadius, sceneRadius,
settings_.NearPlane, sceneRadius * 2.0f);

// Flip Y: GLM targets OpenGL (Y-up NDC); Vulkan uses Y-down NDC.
proj[1][1] *= -1.0f;

return proj * view;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — TransitionDepthImage
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::TransitionDepthImage(VkCommandBuffer commandBuffer,
                                         ShadowLayer&    layer,
                                         VkImageLayout   newLayout) const
{
const VkImageLayout oldLayout = layer.CurrentLayout;
if (oldLayout == newLayout)
return;

VkImageMemoryBarrier barrier{};
barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
barrier.oldLayout           = oldLayout;
barrier.newLayout           = newLayout;
barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
barrier.image               = layer.Image->Handle();
barrier.subresourceRange    = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

VkPipelineStageFlags srcStage{};
VkPipelineStageFlags dstStage{};

if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
    newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
{
barrier.srcAccessMask = 0;
barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                      | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
}
else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
         newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
{
barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                      | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
}
else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
         newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
{
barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
}
else
{
throw std::runtime_error(
"ShadowMapPass: unsupported depth image layout transition");
}

vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0,
                     0, nullptr, 0, nullptr, 1, &barrier);

layer.CurrentLayout = newLayout;
}

} // namespace Vulkan::Game
