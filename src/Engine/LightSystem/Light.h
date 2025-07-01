#pragma once
#include "From-GDGRAP2/GameObject.h"
#include "Utilities/Glm.hpp"

namespace Assets
{

	struct alignas(16) LightProperties final
	{
		enum class Enum : uint32_t
		{
			PointLight = 0,
			DirectionalLight = 1,
			SpotLight = 2
		};

		alignas(16) glm::vec3 LightPos;
		alignas(16) glm::vec4 AmbientColor;
		alignas(16) glm::vec4 LightColor;
		alignas(4) Enum LightType;
	};
}

class Light : public GameObject
{
protected:
	Assets::LightProperties props_;

public:
	enum LightType { PointLight = 0, DirectionalLight = 1, SpotLight = 2 };

	Light(String name, LightType type) : GameObject(name, convertLightTypeToGameObjectType(type))
	{
		// Default Properties
		props_.LightPos = glm::vec3(0, 0, 0);
		props_.AmbientColor = glm::vec4(1.0, 1.0, 1.0, 0.02);
		props_.LightColor = glm::vec4(1.0, 1.0, 1.0, 500000.0f);
		props_.LightType = convertLightTypeEnum(type);

		GameObject::setLocalPosition(props_.LightPos);

		updateSceneView();
	}

	Light(String name, LightType type, glm::vec3 pos, glm::vec4 ambientCol, glm::vec4 lightCol)
		: GameObject(name, convertLightTypeToGameObjectType(type))
	{
		props_.LightPos = pos;
		props_.AmbientColor = ambientCol;
		props_.LightColor = lightCol;
		props_.LightType = convertLightTypeEnum(type);

		GameObject::setLocalPosition(props_.LightPos);

		updateSceneView();
	}

	const Assets::LightProperties Properties() const { return this->props_; }

	glm::vec4 getAmbientColor()
	{
		return this->props_.AmbientColor;
	}

	glm::vec4 getLightColor()
	{
		return this->props_.LightColor;
	}

	Assets::LightProperties::Enum getLightType()
	{
		return this->props_.LightType;
	}

	// setposition sets lightpos also
	void setLocalPosition(float x, float y, float z) override
	{
		props_.LightPos = glm::vec3(x, y, z);
		GameObject::setLocalPosition(x, y, z);
	}
	void setLocalPosition(vec3 newPos) override
	{
		props_.LightPos = newPos;
		GameObject::setLocalPosition(newPos);
	}

	void setAmbientColor(float r, float g, float b, float a)
	{
		this->props_.AmbientColor = glm::vec4(r, g, b, a);
	}

	void setAmbientColor(glm::vec4 ambientCol)
	{
		this->props_.AmbientColor = ambientCol;
	}

	void setLightColor(float r, float g, float b, float a)
	{
		this->props_.LightColor = glm::vec4(r, g, b, a);
	}

	void setLightColor(glm::vec4 lightCol)
	{
		this->props_.LightColor = lightCol;
	}

	void setLightType(LightType type)
	{
		this->props_.LightType = convertLightTypeEnum(type);
		this->type = convertLightTypeToGameObjectType(type);
	}

private:
	Assets::LightProperties::Enum convertLightTypeEnum(LightType type)
	{
		switch (type)
		{
		case PointLight:
			return Assets::LightProperties::Enum::PointLight;
			break;
		case DirectionalLight:
			return Assets::LightProperties::Enum::DirectionalLight;
			break;
		case SpotLight:
			return Assets::LightProperties::Enum::SpotLight;
			break;
		}
	}
	PrimitiveType convertLightTypeToGameObjectType(LightType type)
	{
		switch (type)
		{
		case PointLight:
			return POINT_LIGHT;
			break;
		case DirectionalLight:
			return DIRECTIONAL_LIGHT;
			break;
		case SpotLight:
			return SPOT_LIGHT;
			break;
		}
	}
};