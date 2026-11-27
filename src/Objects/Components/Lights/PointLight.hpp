#pragma once

#include "LightBase.hpp"
#include "../Transform.hpp"

class PointLight : public LightBase {
public:
    // Initializes the point light with a default range matching the pipeline definition
    PointLight(gbe::IInstanceManager<HierarchyObject>::Ref owner = nullptr)
        : LightBase("PointLight", owner) {}

    ~PointLight() override = default;

    PointLight(const PointLight&) = delete;
    PointLight& operator=(const PointLight&) = delete;

    PointLight(PointLight&&) = default;
    PointLight& operator=(PointLight&&) = default;

    // Retrieves the world position from the attached Transform component
    glm::vec3 GetPosition() const;

    // Getters and setters for Point Light specific properties
    float GetRange() const { return m_range; }
    void SetRange(float range) { m_range = range; }

    // Radius of the light's body, in world units: how soft its shadows are and how big its highlight is
    float GetRadius() const { return m_radius; }
    void SetRadius(float radius) { m_radius = radius; }

    // Brightness in real units: the light's whole output, as a light that shines the same in every direction
    float GetLumens() const { return m_lumens; }
    void SetLumens(float lumens) { m_lumens = lumens; }

private:
    // Default range of 10.0f matches PointLightData. Set here, not in a constructor,
    // so the generated serializer constructor gets it too.
    float m_range = 10.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_range, "Range");
    float m_radius = 0.1f;      // The size every point light had before lights had one
    GBE_SERIALIZE_FIELD_W_NAME(m_radius, "Radius");
    float m_lumens = 800.0f;    // A household bulb. Used with real units; Intensity is used without.
    GBE_SERIALIZE_FIELD_W_NAME(m_lumens, "Lumens");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(PointLight, LightBase);
};

GBE_REGISTER_SERIALIZED_TYPE(PointLight, ComponentBase);