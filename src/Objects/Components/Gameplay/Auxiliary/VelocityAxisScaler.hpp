#pragma once

#include "Components/ComponentBase.hpp"
#include "Components/Transform.hpp"
#include "Types/UpdateTrigger.hpp"

#include <glm/glm.hpp>

// Scales a referenced transform's local scale per axis from the owner's tracked world movement along its own local axes (e.g. thruster flames).
class VelocityAxisScaler : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    VelocityAxisScaler(gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~VelocityAxisScaler() override = default;

    VelocityAxisScaler(const VelocityAxisScaler&) = delete;
    VelocityAxisScaler& operator=(const VelocityAxisScaler&) = delete;

    VelocityAxisScaler(VelocityAxisScaler&&) = default;
    VelocityAxisScaler& operator=(VelocityAxisScaler&&) = default;

    void OnUpdate(float deltaTime) override;

private:
    gbe::ObjectRef<Transform> m_target = nullptr;
    GBE_SERIALIZE_FIELD_W_NAME(m_target, "Target");

    bool m_scaleX = false;
    GBE_SERIALIZE_FIELD_W_NAME(m_scaleX, "Scale X");

    bool m_scaleY = false;
    GBE_SERIALIZE_FIELD_W_NAME(m_scaleY, "Scale Y");

    bool m_scaleZ = true;
    GBE_SERIALIZE_FIELD_W_NAME(m_scaleZ, "Scale Z");

    // When true, movement along the negative local axis drives the scale instead of the positive.
    bool m_invertDirection = false;
    GBE_SERIALIZE_FIELD_W_NAME(m_invertDirection, "Invert Direction");

    // Speed along the axis (units/s) at which the scale reaches Max Scale.
    float m_maxSpeed = 10.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_maxSpeed, "Max Speed");

    float m_minScale = 0.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_minScale, "Min Scale");

    float m_maxScale = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_maxScale, "Max Scale");

    // Higher is snappier; 0 disables smoothing.
    float m_smoothness = 8.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_smoothness, "Smoothness");

    glm::vec3 m_lastPosition = glm::vec3(0.0f);
    bool m_hasLastPosition = false;
    float m_currentScale = 0.0f;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(VelocityAxisScaler, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(VelocityAxisScaler, ComponentBase);
