#pragma once

#include "Vulkan/Vulkan.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace Vulkan::Game
{
	/// Maximum number of directional lights that can cast shadows simultaneously.
	/// Matches MAX_SHADOW_LIGHTS in shadow_vert.vert / game_frag.frag and
	/// ShadowMapPass::kMaxShadowLights (kept in sync manually).
	inline constexpr uint32_t kMaxShadowLights = 4;
	// ─────────────────────────────────────────────────────────────────────────
	/// @brief Optional per-light-slot overrides for ShadowMapSettings.
	///
	/// Create one of these for each directional light whose shadow behaviour
	/// should differ from the global defaults.  Any field left as std::nullopt
	/// transparently inherits the corresponding value from ShadowMapSettings.
	///
	/// Usage example — give the second directional light a higher-res shadow map
	/// and tighter depth bias while everything else uses the global defaults:
	/// @code
	///   ShadowMapSettings s{};
	///   s.LightOverrides.resize(2);          // slots 0 and 1
	///   s.LightOverrides[1].Resolution              = 4096;
	///   s.LightOverrides[1].DepthBiasConstantFactor = 0.5f;
	///   s.LightOverrides[1].DepthBiasSlopeFactor    = 1.0f;
	/// @endcode
	// ─────────────────────────────────────────────────────────────────────────
	struct ShadowLightSettings
	{
		// ── Depth image ───────────────────────────────────────────────────────

		/// Shadow map resolution override for this slot (square, in texels).
		std::optional<uint32_t> Resolution;

		// ── Depth bias ────────────────────────────────────────────────────────

		/// Override whether depth bias is applied for this light.
		std::optional<bool>    DepthBiasEnable;

		/// Constant depth offset override for this light.
		std::optional<float>   DepthBiasConstantFactor;

		/// Slope-scaled depth offset override for this light.
		std::optional<float>   DepthBiasSlopeFactor;

		/// Maximum depth bias clamp override for this light.
		std::optional<float>   DepthBiasClamp;

		// ── Ortho frustum ─────────────────────────────────────────────────────

		/// World-space AABB padding override for this light's frustum.
		std::optional<float>   SceneMargin;

		/// Near-plane distance override for this light's ortho camera.
		std::optional<float>   NearPlane;
	};

	// ─────────────────────────────────────────────────────────────────────────
	/// @brief Aggregates all tunable properties and settings for a ShadowMapPass.
	///
	/// Pass an instance of this struct to ShadowMapPass's constructor to
	/// control resolution, depth format, sampler behaviour, depth-bias offsets,
	/// scene-frustum margins, and rasteriser cull mode — without touching any
	/// Vulkan pipeline code directly.
	///
	/// All fields have sensible production defaults that reproduce the original
	/// ShadowMapPass behaviour (2048² PCF shadow map with depth bias).
	///
	/// Per-light overrides are specified in @ref LightOverrides.  Index i in
	/// that vector corresponds to the i-th directional light in the scene's
	/// light buffer (same ordering used for the shadow sub-pass loop).
	// ─────────────────────────────────────────────────────────────────────────
	struct ShadowMapSettings final
	{
		// ── Depth image ───────────────────────────────────────────────────────

		/// Default width and height of every (square) shadow depth image.
		/// Individual lights can override this via LightOverrides[i].Resolution.
		uint32_t Resolution{ 2048 };

		/// Vulkan depth format used for all shadow images and the render-pass.
		/// Must be the same for every slot (shared render pass).
		VkFormat DepthFormat{ VK_FORMAT_D32_SFLOAT };

		// ── Sampler (shared for all slots) ────────────────────────────────────

		/// Magnification filter applied when sampling shadow maps (PCF reads).
		VkFilter MagFilter{ VK_FILTER_LINEAR };

		/// Minification filter applied when sampling shadow maps.
		VkFilter MinFilter{ VK_FILTER_LINEAR };

		/// Address mode used on all three UV axes (U, V, W) for every sampler.
		/// CLAMP_TO_BORDER keeps pixels outside the frustum "lit" (shadow = 1.0).
		VkSamplerAddressMode AddressMode{ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER };

		/// Border colour returned when sampling outside [0, 1] UV range.
		/// FLOAT_OPAQUE_WHITE → compare returns 1.0 (not in shadow).
		VkBorderColor BorderColor{ VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE };

		/// Depth-compare operator used by the sampler2DShadow PCF hardware.
		VkCompareOp CompareOp{ VK_COMPARE_OP_LESS_OR_EQUAL };

		/// Mip-map filter mode.  NEAREST is correct for single-mip depth maps.
		VkSamplerMipmapMode MipmapMode{ VK_SAMPLER_MIPMAP_MODE_NEAREST };

		// ── Depth bias (default for all lights) ───────────────────────────────

		/// Default: enable rasteriser depth bias to prevent shadow acne.
		/// Individual lights can override this via LightOverrides[i].DepthBiasEnable.
		bool DepthBiasEnable{ true };

		/// Default constant depth offset (prevents surface acne).
		float DepthBiasConstantFactor{ 1.25f };

		/// Default slope-scaled depth offset (handles grazing-angle self-shadow).
		float DepthBiasSlopeFactor{ 1.75f };

		/// Default maximum depth bias clamp.  0 = unclamped.
		float DepthBiasClamp{ 0.0f };

		// ── Ortho frustum / scene coverage (default for all lights) ───────────

		/// Default world-space AABB padding for the ortho frustum.
		float SceneMargin{ 100.0f };

		/// Default near-plane distance of the ortho light camera.
		float NearPlane{ 0.1f };

		// ── Rasteriser (shared pipeline — applies to all lights) ──────────────

		/// Face-cull mode for the shared shadow pipeline.
		VkCullModeFlags CullMode{ VK_CULL_MODE_BACK_BIT };

		// ── Per-light slot overrides ──────────────────────────────────────────

		/// Optional per-light settings.  Index i applies to the i-th directional
		/// light (same ordering as the shadow sub-pass loop in ShadowMapPass).
		/// Lights beyond this vector's size use the global defaults above.
		std::vector<ShadowLightSettings> LightOverrides;
	};

} // namespace Vulkan::Game
