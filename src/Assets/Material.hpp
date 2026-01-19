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
		
		// Note: vec3 and vec4 gets aligned on 16 bytes in Vulkan shaders. 
		
		// Base material
		glm::vec4 Diffuse;
		int32_t DiffuseTextureId;
		//float metallic = 1;
		//int32_t MetallicTextureId;
		//float smoothness = 1;
		//int32_t SmoothnessTextureId;
		//float flatness = 1;
		//int32_t NormalTextureId;

		// Metal fuzziness
		float Fuzziness;

		// Dielectric refraction index
		float RefractionIndex = 0;

		// Which material are we dealing with
		Enum MaterialModel;
		
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
	};

}