#include "Light.h"

static GameObject::PrimitiveType LTypeToOType(Light::LightType type)
{
	switch (type)
	{
	case Light::LightType::PointLight:
		return GameObject::POINT_LIGHT;

		case Light::LightType::DirectionalLight:
		return GameObject::DIRECTIONAL_LIGHT;

	case Light::LightType::SpotLight:
		return GameObject::SPOT_LIGHT;
	}
}

Light::Light(String name, LightType type, glm::vec3 pos, glm::vec3 dir, glm::vec4 ambientCol, glm::vec4 lightCol) : GameObject(name, LTypeToOType(type)),
	m_ambientColor(ambientCol), m_lightColor(lightCol), m_lightType(type), m_direction(glm::vec4(dir, 0.0f))
{
	this->SetLocalPosition(pos);
}

Light::Light(const Light& other) : GameObject(other), m_ambientColor(other.m_ambientColor), m_lightColor(other.m_lightColor), m_lightType(other.m_lightType), m_direction(other.m_direction)
{
	this->SetLocalPosition(other.GetLocalPosition());
}

GameObject::GameObjectPtr Light::Clone() const
{
    return std::make_unique<Light>(*this);
}

glm::vec3 Light::GetDirection() const
{	
	return glm::normalize(this->GetWorldRotationQuat() * glm::vec3(0.0f, -1.0f, 0.0f));
}
