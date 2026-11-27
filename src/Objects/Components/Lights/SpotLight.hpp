#pragma once

#include "LightBase.hpp"
#include "../Transform.hpp"

// A point light that shines in a cone, along the object's -Y axis (like a directional light's direction).
// Between the inner and the outer angle the light fades out smoothly.
class SpotLight : public LightBase {
public:
    SpotLight(gbe::IInstanceManager<HierarchyObject>::Ref owner = nullptr)
        : LightBase("SpotLight", owner) {}

    ~SpotLight() override = default;

    SpotLight(const SpotLight&) = delete;
    SpotLight& operator=(const SpotLight&) = delete;

    SpotLight(SpotLight&&) = default;
    SpotLight& operator=(SpotLight&&) = default;

    float GetRange() const { return m_range; }
    void SetRange(float range) { m_range = range; }

    // Radius of the light's body, in world units: how soft its shadows are and how big its highlight is
    float GetRadius() const { return m_radius; }
    void SetRadius(float radius) { m_radius = radius; }

    // Half-angles of the cone in degrees: full light inside the inner one, none outside the outer one
    float GetInnerAngle() const { return m_innerAngle; }
    void SetInnerAngle(float degrees) { m_innerAngle = degrees; }
    float GetOuterAngle() const { return m_outerAngle; }
    void SetOuterAngle(float degrees) { m_outerAngle = degrees; }

    // Brightness in real units: what a point light with the same intensity would send out in all directions.
    // Narrowing the cone does not make the light brighter.
    float GetLumens() const { return m_lumens; }
    void SetLumens(float lumens) { m_lumens = lumens; }

private:
    // Defaults live here, not in a constructor, so the generated serializer constructor gets them too.
    float m_range = 10.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_range, "Range");
    float m_radius = 0.1f;
    GBE_SERIALIZE_FIELD_W_NAME(m_radius, "Radius");
    float m_innerAngle = 20.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_innerAngle, "Inner Angle");
    float m_outerAngle = 35.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_outerAngle, "Outer Angle");
    float m_lumens = 800.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_lumens, "Lumens");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(SpotLight, LightBase);
};

GBE_REGISTER_SERIALIZED_TYPE(SpotLight, ComponentBase);
