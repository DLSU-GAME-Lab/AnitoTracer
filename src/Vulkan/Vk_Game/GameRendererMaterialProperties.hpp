#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace Vulkan::Game
{
	/// @brief Game Renderer-specific material properties for normal mapping
	/// and other renderer-only features.
	/// 
	/// This struct is separate from Assets::Material to:
	/// - Keep the base Material unchanged (shared with legacy/compute renderers)
	/// - Store Game Renderer-only features (normal maps, metallic maps, etc.)
	/// - Maintain clear separation of concerns
	/// 
	/// Layout must match the shader buffer exactly (std140 alignment rules).
	struct alignas(16) GameRendererMaterialProperties final
	{
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

		// Padding to align to 16-byte boundary for std140 (we have 7 floats + 4 ints = 44 bytes,
		// round up to 48 bytes = 3 vec4s)
		float _pad[4];
	};

	static_assert(sizeof(GameRendererMaterialProperties) == 48, 
		"GameRendererMaterialProperties must be exactly 48 bytes (3 vec4s) for std140 alignment");
}
