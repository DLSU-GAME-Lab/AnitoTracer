#pragma once

#include <memory>

#include "Utilities/Glm.hpp"
#include "From-GDGRAP2/EventBroadcaster.h"

namespace Assets
{

	struct alignas(16) Material final
	{
		static std::shared_ptr<Material> Lambertian(const glm::vec3& diffuse, const int32_t textureId = -1)
		{
			return std::make_shared<Material>(glm::vec4(diffuse, 1), textureId, 0.0f, 0.0f, Enum::Lambertian);
		}

		static std::shared_ptr<Material> Metallic(const glm::vec3& diffuse, const float fuzziness, const int32_t textureId = -1)
		{
			return std::make_shared<Material>(glm::vec4(diffuse, 1), textureId, fuzziness, 0.0f, Enum::Metallic);
		}

		static std::shared_ptr<Material> Dielectric(const float refractionIndex, const int32_t textureId = -1)
		{
			return std::make_shared<Material>(glm::vec4(0.7f, 0.7f, 1.0f, 1), textureId,  0.0f, refractionIndex, Enum::Dielectric);
		}

		static std::shared_ptr<Material> Isotropic(const glm::vec3& diffuse, const int32_t textureId = -1)
		{
			return std::make_shared<Material>(glm::vec4(diffuse, 1), textureId, 0.0f, 0.0f, Enum::Isotropic);
		}

		static std::shared_ptr<Material> DiffuseLight(const glm::vec3& diffuse, const int32_t textureId = -1)
		{
			return std::make_shared<Material>(glm::vec4(diffuse, 1), textureId, 0.0f, 0.0f, Enum::DiffuseLight);
		}

		enum class Enum : uint32_t
		{
			Lambertian = 0,
			Metallic = 1,
			Dielectric = 2,
			Isotropic = 3,
			DiffuseLight = 4
		};

		// Note: Vulkan std140 layout requires 16-byte (vec4) alignment.
		// This struct is 16-byte aligned and total size will pad to multiple of 16.
		// Current layout:
		//   Bytes 0-15:   glm::vec4 Diffuse
		//   Bytes 16-19:  int32_t DiffuseTextureId
		//   Bytes 20-23:  float Fuzziness
		//   Bytes 24-27:  float RefractionIndex
		//   Bytes 28-31:  uint32_t MaterialModel
		//   Bytes 32-35:  int32_t AlphaMapTextureId
		//   Bytes 36-39:  float AlphaCutoffThreshold
		//   Bytes 40-43:  uint32_t AlphaBlendMode
		//   Bytes 44-47:  float _pad (padding to 48-byte boundary)
		// Total: 48 bytes (3 vec4s when aligned)

		// Base material
		glm::vec4 Diffuse;
		int32_t DiffuseTextureId;
		float Fuzziness;
		float RefractionIndex = 0;
		Enum MaterialModel;

		// ===== NEW: Alpha/Transparency Support =====
		/// @brief Index into the texture array for alpha/transparency maps.
		/// -1 means no dedicated alpha map (will use diffuse alpha or be fully opaque).
		int32_t AlphaMapTextureId = -1;

		/// @brief Alpha cutoff threshold for alpha test/clip.
		/// Fragments with alpha < this threshold are discarded (for opaque rendering).
		/// Recommended range: [0.4, 0.6] for most materials.
		float AlphaCutoffThreshold = 0.5f;

		/// @brief Alpha blend mode selector:
		/// 0 = Opaque      (no blending, alpha cutoff applied)
		/// 1 = Transparent (alpha blending enabled, cutoff bypassed)
		/// 2 = Additive    (additive blending, no alpha cutoff)
		/// Default: 0 (Opaque)
		uint32_t AlphaBlendMode = 0u;

		/// @brief Padding for alignment (ensures 48-byte total)
		float _pad = 0.0f;

		void SetAlbedoColor(glm::vec4 color) 
		{ 
			this->Diffuse = color; 
			EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
		}

		void SetAlbedoTexture(int textureId)
		{
			this->DiffuseTextureId = textureId;
			EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
		}

		void SetFuzziness(float value)
		{
			this->Fuzziness = value;
		}

		void SetRefractionIndex(float value)
		{
			this->RefractionIndex = value;
		}

		/// @brief Set the material as transparent with optional custom cutoff threshold
		void SetTransparent(uint32_t blendMode = 1u, float cutoffThreshold = 0.5f, int32_t alphaMapTexId = -1)
		{
			AlphaBlendMode = blendMode;
			AlphaCutoffThreshold = cutoffThreshold;
			AlphaMapTextureId = alphaMapTexId;
			EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
		}

		/// @brief Set the material back to fully opaque
		void SetOpaque()
		{
			AlphaBlendMode = 0u;
			AlphaCutoffThreshold = 0.5f;
			AlphaMapTextureId = -1;
			EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
		}
	};

	// Verify Material struct is properly aligned for Vulkan shader std140 layout
	// Size must be 48 bytes (3 vec4s) to maintain compatibility with the original
	// GPU buffer calculations that expected 32-byte materials would pack efficiently
	static_assert(sizeof(Material) == 48, 
		"Material struct size must be 48 bytes for proper GPU buffer alignment. "
		"Check field ordering and padding!");

}