#pragma once

#include "LightBase.hpp"
#include "../Transform.hpp"

class DirectionalLight : public LightBase {
public:
    DirectionalLight(gbe::IInstanceManager<HierarchyObject>::Ref owner = nullptr)
        : LightBase("DirectionalLight", owner) {}

    ~DirectionalLight() override = default;

    DirectionalLight(const DirectionalLight&) = delete;
    DirectionalLight& operator=(const DirectionalLight&) = delete;

    DirectionalLight(DirectionalLight&&) = default;
    DirectionalLight& operator=(DirectionalLight&&) = default;

    // Calculates the world direction by multiplying the Transform's 
    // quaternion rotation by the local direction vector.
    glm::vec3 GetDirection() const;

    // Sets the base directional vector before any transform rotations are applied.
    void SetLocalDirection(const glm::vec3& direction) {
        m_localDirection = glm::normalize(direction);
    }

    // Radius of the light's disk as seen from the surface, in degrees: how soft its shadows are and how big its highlight is
    float GetAngularRadius() const { return m_angularRadius; }
    void SetAngularRadius(float degrees) { m_angularRadius = degrees; }

    // Brightness in real units: the light that reaches a surface facing it, in lux
    float GetLux() const { return m_lux; }
    void SetLux(float lux) { m_lux = lux; }

private:
    // Not serialized. Initialised here, not in a constructor, so the generated
    // serializer constructor (scene load, paste, Add Component) gets it too.
    glm::vec3 m_localDirection{ 0.0f, -1.0f, 0.0f };

    float m_angularRadius = 0.573f;     // 0.01 rad, the size every directional light had before lights had one. The sun is 0.27.
    GBE_SERIALIZE_FIELD_W_NAME(m_angularRadius, "Angular Radius");
    float m_lux = 100000.0f;            // Direct sun. Used with real units; Intensity is used without.
    GBE_SERIALIZE_FIELD_W_NAME(m_lux, "Lux");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(DirectionalLight, LightBase);
};

GBE_REGISTER_SERIALIZED_TYPE(DirectionalLight, ComponentBase);