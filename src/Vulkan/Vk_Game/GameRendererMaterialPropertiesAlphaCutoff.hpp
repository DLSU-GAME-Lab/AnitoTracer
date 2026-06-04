#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace Vulkan::Game
{
	/// @brief Extended Game Renderer material properties incorporating alpha cutoff
	/// for transparent objects and materials.
	/// 
	/// This struct extends GameRendererMaterialProperties with:
	/// - Alpha channel texture support
	/// - Alpha cutoff threshold (alpha test / clip)
	/// - Alpha blend mode control
	/// 
	/// Layout must match the shader buffer exactly (std140 alignment rules).
	/// Total size: 64 bytes (4 vec4s) for proper alignment.
	struct alignas(16) GameRendererMaterialPropertiesAlphaCutoff final
	{
		// ===== Original Properties (first 48 bytes) =====

		/// @brief Index into the texture array for normal maps.
		/// -1 means no normal map (use vertex normal only).
		int32_t NormalMapTextureId = -1;

		/// @brief Strength/intensity of normal map effect (0.0 = no effect, 1.0 = full effect).
		float NormalMapStrength = 1.0f;

		/// @brief Index into the texture array for metallic maps.
		/// -1 means no metallic map (use material Fuzziness uniformly).
		int32_t MetallicMapTextureId = -1;

		/// @brief Metallic value when no map is used (0.0 = dielectric, 1.0 = fully metallic).
		float MetallicValue = 0.0f;

		/// @brief Index into the texture array for roughness/smoothness maps.
		/// -1 means no roughness map (use material Fuzziness uniformly).
		int32_t RoughnessMapTextureId = -1;

		/// @brief Roughness value when no map is used (0.0 = mirror-smooth, 1.0 = completely rough).
		float RoughnessValue = 0.5f;

		/// @brief Index into the texture array for ambient occlusion maps.
		/// -1 means no AO map (no occlusion applied).
		int32_t AOMapTextureId = -1;

		/// @brief AO intensity/strength when using an AO map (0.0 = no occlusion, 1.0 = full occlusion).
		float AOStrength = 1.0f;

		// ===== NEW: Alpha Cutoff Properties (second 16 bytes) =====

		/// @brief Index into the texture array for alpha/transparency maps.
		/// -1 means no alpha map, use diffuse texture alpha channel instead.
		/// When both are -1, alpha is assumed to be fully opaque (1.0).
		int32_t AlphaMapTextureId = -1;

		/// @brief Alpha cutoff threshold for alpha test/clip.
		/// Fragments with alpha < this threshold are discarded (for opaque rendering).
		/// Recommended range: [0.4, 0.6] for most materials.
		/// Set to 0.0 to disable alpha cutoff (fully transparent-capable).
		float AlphaCutoffThreshold = 0.5f;

		/// @brief Alpha blend mode selector:
		/// 0 = Opaque      (no blending, alpha cutoff applied)
		/// 1 = Transparent (alpha blending enabled, cutoff bypassed)
		/// 2 = Additive    (additive blending, no alpha cutoff)
		/// Default: 0 (Opaque)
		uint32_t AlphaBlendMode = 0u;

		/// @brief Unused padding to align to 16-byte boundary.
		float _pad;

		// Final padding to reach exactly 64 bytes
		// (12 floats + 5 ints = 44 bytes, round to 64 = next multiple of 16)
		float _padEnd[3];
	};

	static_assert(sizeof(GameRendererMaterialPropertiesAlphaCutoff) == 64,
		"GameRendererMaterialPropertiesAlphaCutoff must be exactly 64 bytes (4 vec4s) for std140 alignment");

	// For backward compatibility, keep a type alias for the original struct
	using GameRendererMaterialProperties = GameRendererMaterialPropertiesAlphaCutoff;
}
