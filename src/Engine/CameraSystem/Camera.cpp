#include "Camera.h"

#include <glm/fwd.hpp>
#include <glm/glm.hpp>

Camera::Camera(std::string name, ProjectionMode proj) : GameObject(name, PrimitiveType::CAMERA), m_projectionMode(proj)
{
}

Camera::Camera(const Camera& other) : GameObject(other), m_projectionMode(other.m_projectionMode),
	m_forward(other.m_forward), m_right(other.m_right), m_up(other.m_up),
	projection_(other.projection_), view_(other.view_),	windowWidth_(other.windowWidth_), windowHeight_(other.windowHeight_), m_isViewDirty(other.m_isViewDirty)
{
}

GameObject::GameObjectPtr Camera::Clone() const
{
	return std::make_unique<Camera>(*this);
}

void Camera::Reset()
{
	this->SetLocalPosition(0.0f, 0.0f, 0.0f);
	this->SetLocalRotationQuat(glm::quat(1, 0, 0, 0));
}

void Camera::SetLocalPosition(float x, float y, float z)
{
	GameObject::SetLocalPosition(vec3(x, y, z));
	this->m_isViewDirty = true;
}

void Camera::SetLocalPosition(vec3 newPos)
{
	GameObject::SetLocalPosition(newPos);
	this->m_isViewDirty = true;
}

void Camera::SetLocalRotationEuler(const vec3& eulerDeg)
{
	GameObject::SetLocalRotationEuler(eulerDeg);
	this->m_isViewDirty = true;
}

void Camera::SetLocalRotationQuat(const quat& q)
{
	GameObject::SetLocalRotationQuat(q);
	this->m_isViewDirty = true;
}

void Camera::SetLocalRotationEuler(float x, float y, float z)
{
	GameObject::SetLocalRotationEuler(x, y, z);
	this->m_isViewDirty = true;
}

glm::mat4 Camera::GetProjection(UserSettings settings, const VkExtent2D extent)
{
	switch (m_projectionMode)
	{
	case ProjectionMode::orthographic:
		projection_ = glm::ortho(-1000.0f, 1000.0f, 1000.0f, -1000.0f, 0.1f, 1000.0f);
		break;

	case ProjectionMode::perspective:
		projection_ = glm::perspective(glm::radians(settings.FieldOfView), static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 10000.0f);
		break;
	}

	windowWidth_ = extent.width;
	windowHeight_ = extent.height;

	projection_[1][1] *= -1;	// Inverting Y for Vulkan, https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/

	return projection_;
}

void Camera::SetProjectionType(ProjectionMode type)
{
	this->m_projectionMode = type;
}

void Camera::MoveForward(float amount)
{
	glm::vec3 forward = GetForward();
	SetLocalPosition(GetLocalPosition() + forward * amount);
	this->m_isViewDirty = true;
}

void Camera::MoveRight(float amount)
{
	glm::vec3 right = GetRight();
	SetLocalPosition(GetLocalPosition() + right * amount);
	this->m_isViewDirty = true;
}

void Camera::MoveUp(float amount)
{
	glm::vec3 up = GetUp();
	SetLocalPosition(GetLocalPosition() + up * amount);
	this->m_isViewDirty = true;
}

void Camera::lookAt(const glm::vec3& target)
{
	glm::vec3 position = GetLocalPosition();
	glm::vec3 direction = glm::normalize(target - position);

	float pitch = glm::degrees(asin(-direction.y));
	float yaw = glm::degrees(atan2(direction.x, -direction.z));

	SetLocalRotationEuler(glm::vec3(yaw, pitch, 0.0f));
}

void Camera::UpdateViewMatrix()
{
	if (this->m_isViewDirty)
	{
		const quat& q = this->GetLocalRotationQuat();

		glm::mat4 rotationMat = glm::toMat4(q);
		glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), -this->GetLocalPosition());
		this->view_ = rotationMat * translationMat;

		this->m_forward = glm::normalize(glm::rotate(q, glm::vec3(0.0f, 0.0f, -1.0f)));
		this->m_right = glm::normalize(glm::rotate(q, glm::vec3(1.0f, 0.0f, 0.0f)));
		this->m_up = glm::normalize(glm::rotate(q, glm::vec3(0.0f, 1.0f, 0.0f)));

		this->m_isViewDirty = false;
	}
}

glm::mat4 Camera::GetViewMatrix()
{
	this->UpdateViewMatrix();
	return this->view_;
}
