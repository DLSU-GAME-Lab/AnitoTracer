#pragma once

#include "ComponentBase.hpp"
#include "ScriptApiRegistry.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_FORCE_LEFT_HANDED       

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform : public ComponentBase {
public:
    Transform(gbe::IInstanceManager<HierarchyObject>::Ref owner = nullptr)
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

    // World-space getters that include parent transforms.
    glm::vec3 GetPosition() const;
    glm::quat GetRotation() const;
    glm::vec3 GetScale() const;

    // Local-space accessors for editing/serialization.
    const glm::vec3& GetLocalPosition() const { return m_position; }
    const glm::quat& GetLocalRotation() const { return m_rotation; }
    const glm::vec3& GetLocalScale() const { return m_scale; }

    void SetPosition(const glm::vec3& position) { m_position = position; }
    void SetWorldPosition(const glm::vec3& worldPosition);
    void SetWorldRotation(const glm::quat& worldRotation);

    void SetRotation(const glm::quat& rotation) {
        m_rotation = rotation;
        m_eulerAnglesDegrees = glm::degrees(glm::eulerAngles(m_rotation));
    }

    void SetScale(const glm::vec3& scale) { m_scale = scale; }

    glm::vec3 GetEulerAnglesDegrees() const;
    void SetEulerAnglesDegrees(const glm::vec3& eulerDegrees);
    glm::mat4 GetLocalMatrix() const;

    glm::mat4 GetWorldMatrix() const;
    void SetWorldScale(const glm::vec3& worldScale);

    void SyncPhysics();
    void SyncChildrenPhysics();

private:
    glm::vec3 m_position;
    GBE_SERIALIZE_FIELD_W_NAME_CB(m_position, "Position", [this](glm::vec3 pos) {
        m_position = pos;
		SyncPhysics();
    });

    glm::quat m_rotation; // Don't serialize this anymore, rely on euler angles.

    glm::vec3 m_eulerAnglesDegrees;
    GBE_SERIALIZE_FIELD_W_NAME_CB(m_eulerAnglesDegrees, "Rotation", [this](glm::vec3 eulerDegrees) {
        SetEulerAnglesDegrees(eulerDegrees);
        SyncPhysics();
    });
    
    glm::vec3 m_scale;
    GBE_SERIALIZE_FIELD_W_NAME(m_scale, "Scale");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(Transform, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(Transform, ComponentBase);

ANITO_SCRIPT_TYPE(Transform, "transform", "anito::Transform", true);
ANITO_SCRIPT_PROPERTY(Transform, "transform", "position", GetPosition, SetWorldPosition);
ANITO_SCRIPT_PROPERTY(Transform, "transform", "localPosition", GetLocalPosition, SetPosition);
ANITO_SCRIPT_PROPERTY(Transform, "transform", "rotation", GetRotation, SetWorldRotation);
ANITO_SCRIPT_PROPERTY(Transform, "transform", "localRotation", GetLocalRotation, SetRotation);
ANITO_SCRIPT_PROPERTY(Transform, "transform", "localScale", GetLocalScale, SetScale);
ANITO_SCRIPT_PROPERTY_RO(Transform, "transform", "lossyScale", GetScale);
ANITO_SCRIPT_PROPERTY(Transform, "transform", "eulerAngles", GetEulerAnglesDegrees, SetEulerAnglesDegrees);