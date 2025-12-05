#pragma once

#include <string>

#include "UserSettings.hpp"
#include "Utilities/Glm.hpp"
#include "AssetManagement/GameObject.hpp"
#include <vulkan/vulkan_core.h>

class Camera : public GameObject
{
public:
	enum ProjectionMode { orthographic = 0, perspective };

	Camera(std::string name, ProjectionMode proj = perspective);
	Camera(const Camera& other);
	~Camera() = default;

	virtual GameObjectPtr Clone() const override;

	virtual bool Update(double deltaTime) { return false; }

	void Reset();

	void SetLocalPosition(float x, float y, float z) override;
	void SetLocalPosition(vec3 newPos) override;

	void SetLocalRotationEuler(const vec3& eulerDeg) override;
	void SetLocalRotationQuat(const quat& q) override;
	void SetLocalRotationEuler(float x, float y, float z) override;

	void UpdateViewMatrix();
	glm::mat4 GetViewMatrix();

	glm::mat4 GetProjection(UserSettings settings, const VkExtent2D extent);
	void SetProjectionType(ProjectionMode type);

	void MoveForward(float amount);
	void MoveRight(float amount);
	void MoveUp(float amount);
	void lookAt(const glm::vec3& target);

	/* should move to transform / gameobject */
	glm::vec3 GetForward() const { return m_forward; }
	glm::vec3 GetRight() const { return m_right; }
	glm::vec3 GetUp() const { return m_up; }

protected:
	bool m_isSceneCamera = false;

public:
	ProjectionMode m_projectionMode = perspective;

	glm::vec3 m_forward{};
	glm::vec3 m_right{};
	glm::vec3 m_up{};
	glm::mat4 projection_{};
	glm::mat4 view_ = glm::mat4(1.0f);;

	float windowWidth_{};
	float windowHeight_{};

	bool m_isViewDirty = true;
};