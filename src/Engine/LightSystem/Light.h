#pragma once
#include "AssetManagement/GameObject.hpp"
#include "Utilities/Glm.hpp"

class Light : public GameObject
{
public:
	enum class LightType : uint32_t
	{
		PointLight = 0,
		DirectionalLight = 1,
		SpotLight = 2
	};

	Light() = default;
	Light(String name, LightType type, glm::vec3 pos, glm::vec3 dir, glm::vec4 ambientCol, glm::vec4 lightCol);
	Light(const Light& other);
	~Light() = default;

	GameObjectPtr Clone() const override;

	void SetAmbientColor(const glm::vec4& color) { m_ambientColor = color; }
	glm::vec4 GetAmbientColor() const { return m_ambientColor; }

	void SetLightColor(const glm::vec4& color) { m_lightColor = color; }
	glm::vec4 GetLightColor() const { return m_lightColor; }

	void SetLightType(LightType type) { m_lightType = type; }
	LightType GetLightType() const { return m_lightType; }

	struct alignas(16) GPUData
	{
		glm::vec4 LightPos;
		glm::vec4 LightDir;
		glm::vec4 AmbientColor;
		glm::vec4 lightColor;
		LightType lightType;
	};

	GPUData GetGPUData() const
	{
		GPUData data{};
		data.LightPos = glm::vec4(this->GetWorldPosition(), 1.0f);
		data.LightDir = glm::vec4(this->GetDirection(), 0.0f);
		data.AmbientColor = this->m_ambientColor;
		data.lightColor = this->m_lightColor; 
		data.lightType = this->m_lightType;
		return data;
	}

private:
	glm::vec4 m_direction = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
	glm::vec4 m_ambientColor = glm::vec4(0.1f);
	glm::vec4 m_lightColor = glm::vec4(1.0f);
	LightType m_lightType = LightType::PointLight;

	glm::vec3 GetDirection() const;
};