#pragma once

#include "Vulkan/Vulkan.hpp"

namespace Vulkan::Game
{
	/// @brief Aggregates all tunable properties and settings for a ShadowMapPass.
	///
	/// Pass an instance of this struct to ShadowMapPass's constructor to
	/// control resolution, depth format, sampler behaviour, depth-bias offsets,
	/// scene-frustum margins, and rasteriser cull mode — without touching any
	/// Vulkan pipeline code directly.
	///
	/// All fields have sensible production defaults that reproduce the original
	/// ShadowMapPass behaviour (2048² PCF shadow map with depth bias).
	struct ShadowMapSettings final
	{
		// ── Depth image ───────────────────────────────────────────────────────

		/// Width and height of the (square) shadow depth image, in texels.
		uint32_t Resolution{ 2048 };

		/// Vulkan depth format used for the shadow image and render-pass attachment.
		VkFormat DepthFormat{ VK_FORMAT_D32_SFLOAT };

		// ── Sampler ───────────────────────────────────────────────────────────

		/// Magnification filter applied when sampling the shadow map (PCF reads).
		VkFilter MagFilter{ VK_FILTER_LINEAR };

		/// Minification filter applied when sampling the shadow map.
		VkFilter MinFilter{ VK_FILTER_LINEAR };

		/// Address mode used on all three UV axes (U, V, W).
		/// CLAMP_TO_BORDER keeps pixels outside the frustum "lit" (shadow = 1.0).
		VkSamplerAddressMode AddressMode{ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER };

		/// Border colour returned when sampling outside [0, 1] UV range.
		/// FLOAT_OPAQUE_WHITE → shadow comparison returns 1.0 (not in shadow).
		VkBorderColor BorderColor{ VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE };

		/// Depth-compare operator used by the sampler2DShadow PCF hardware.
		VkCompareOp CompareOp{ VK_COMPARE_OP_LESS_OR_EQUAL };

		/// Mip-map filter mode.  NEAREST is correct for a single-mip depth map.
		VkSamplerMipmapMode MipmapMode{ VK_SAMPLER_MIPMAP_MODE_NEAREST };

		// ── Depth bias ────────────────────────────────────────────────────────

		/// Enable rasteriser depth bias to prevent shadow acne.
		bool DepthBiasEnable{ true };

		/// Constant depth offset added to every fragment (prevents surface acne).
		float DepthBiasConstantFactor{ 1.25f };

		/// Slope-scaled depth offset (handles self-shadowing at grazing angles).
		float DepthBiasSlopeFactor{ 1.75f };

		/// Maximum (clamped) depth bias value.  0 = unclamped.
		float DepthBiasClamp{ 0.0f };

		// ── Ortho frustum / scene coverage ───────────────────────────────────

		/// World-space padding added around the scene AABB on all sides before
		/// sizing the orthographic frustum.  Increase if large meshes are clipped.
		float SceneMargin{ 100.0f };

		/// Near-plane distance of the orthographic light camera.
		float NearPlane{ 0.1f };

		// ── Rasteriser ────────────────────────────────────────────────────────

		/// Face-cull mode for the shadow pipeline.
		/// BACK_BIT (standard) combined with depth bias prevents most acne.
		VkCullModeFlags CullMode{ VK_CULL_MODE_BACK_BIT };
	};

} // namespace Vulkan::Game
