#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_FORCE_LEFT_HANDED       

#include "../ComponentBase.hpp"
#include <glm/glm.hpp>

#include "../../../Common/MathDefs.hpp"

#include "Organization/IInstanceManager.hpp"

class LightBase : public ComponentBase, public gbe::IInstanceManager<LightBase> {
public:
    // Initializes the light component with a name, an optional owner, and default light properties.
    LightBase(const std::string& name, gbe::IInstanceManager<HierarchyObject>::Ref owner = {})
        : ComponentBase(name, owner),
        m_color(1.0f, 1.0f, 1.0f),
        m_intensity(1.0f) {}

    // Virtual destructor to ensure derived light types are cleaned up correctly.
    ~LightBase() override = default;

    // Delete copy constructor and assignment operator to prevent object slicing.
    LightBase(const LightBase&) = delete;
    LightBase& operator=(const LightBase&) = delete;

    // Allow moving for container compatibility.
    LightBase(LightBase&&) = default;
    LightBase& operator=(LightBase&&) = default;

    // Getters and Setters for the common light properties.
    const glm::vec3& GetColor() const { return m_color; }
    void SetColor(const glm::vec3& color) { m_color = color; }

    float GetIntensity() const { return m_intensity; }
    void SetIntensity(float intensity) { m_intensity = intensity; }

protected:
    Color3 m_color;
    GBE_SERIALIZE_FIELD_W_NAME(m_color, "Light Color");
    float m_intensity;
    GBE_SERIALIZE_FIELD_W_NAME(m_intensity, "Intensity");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(LightBase, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(LightBase, ComponentBase);