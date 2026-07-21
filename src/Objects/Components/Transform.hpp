#pragma once

#include "ComponentBase.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_FORCE_LEFT_HANDED       

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform : public ComponentBase {
public:
    Transform(HierarchyObject* owner = nullptr)
        : ComponentBase("Transform", owner),
        m_position(0.0f, 0.0f, 0.0f),
        m_rotation(1.0f, 0.0f, 0.0f, 0.0f),
        m_eulerAnglesDegrees(0.0f, 0.0f, 0.0f),
        m_scale(1.0f, 1.0f, 1.0f) {}

    ~Transform() override = default;

    Transform(const Transform&) = delete;
    Transform& operator=(const Transform&) = delete;

    Transform(Transform&&) = default;
    Transform& operator=(Transform&&) = default;

    const glm::vec3& GetPosition() const { return m_position; }
    const glm::quat& GetRotation() const { return m_rotation; }
    const glm::vec3& GetScale() const { return m_scale; }

    void SetPosition(const glm::vec3& position) { m_position = position; }

    void SetRotation(const glm::quat& rotation) {
        m_rotation = rotation;
        m_eulerAnglesDegrees = glm::degrees(glm::eulerAngles(m_rotation));
    }

    void SetScale(const glm::vec3& scale) { m_scale = scale; }

    glm::vec3 GetEulerAnglesDegrees() const;
    void SetEulerAnglesDegrees(const glm::vec3& eulerDegrees);
    glm::mat4 GetLocalMatrix() const;

private:
    glm::vec3 m_position;
    glm::quat m_rotation;
    glm::vec3 m_eulerAnglesDegrees;
    glm::vec3 m_scale;
};