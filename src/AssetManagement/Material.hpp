#pragma once
#include <memory>
#include "Utilities/Glm.hpp"

namespace Assets
{
	class Material final
	{
	public:
		enum class MaterialType : uint32_t
		{
			Lambertian = 0,
			Metallic = 1,
			Dielectric = 2,
			Isotropic = 3,
			DiffuseLight = 4
		};

		Material() = default;
		Material(const glm::vec4& diffuse, int32_t textureId, float fuzziness, float refractionIndex, MaterialType type) :
			m_diffuse(diffuse), m_diffuseTextureId(textureId), m_fuzziness(fuzziness), m_refractionIndex(refractionIndex), type(type) {}
		
		const glm::vec4& GetDiffuse() const { return m_diffuse; }
		void SetDiffuse(const glm::vec4& color) { m_diffuse = color; }
		void SetDiffuse(const glm::vec3& color) { m_diffuse = glm::vec4(color, 1.0f); }

		int32_t GetDiffuseTextureId() const { return m_diffuseTextureId; }
		void SetTextureId(int32_t textureId) { m_diffuseTextureId = textureId; }

		float GetFuzziness() const { return m_fuzziness; }
		void SetFuzziness(float value) { m_fuzziness = value; }

		float GetRefractionIndex() const { return m_refractionIndex; }
		void SetRefractionIndex(float value) { m_refractionIndex = value; }

		MaterialType GetType() const { return type; }

		void SetIndex(uint32_t idx) { index = idx; }
		uint32_t GetIndex() const { return index; }

		// Note: vec3 and vec4 gets aligned on 16 bytes in Vulkan shaders. 
		struct alignas(16) GPUData
		{
			glm::vec4 diffuse;
			int32_t diffuseTextureId;
			float fuzziness;
			float refractionIndex;
			MaterialType materialType;
		};

		GPUData GetGPUData() const
		{
			GPUData data{};
			data.diffuse = m_diffuse;
			data.diffuseTextureId = m_diffuseTextureId;
			data.fuzziness = m_fuzziness;
			data.refractionIndex = m_refractionIndex;
			data.materialType = type;
			return data;
		}
	
	private:
		/* Albedo */
		glm::vec4 m_diffuse = glm::vec4(0);
		int32_t m_diffuseTextureId = 0;

		/* Metal Fuzziness */
		float m_fuzziness = 0.0f;

		/* Dielectric  Refraction Index */
		float m_refractionIndex = 0.0f;

		/* Material Type */
		MaterialType type = MaterialType::Lambertian;

		uint32_t index = 0;
	};

}