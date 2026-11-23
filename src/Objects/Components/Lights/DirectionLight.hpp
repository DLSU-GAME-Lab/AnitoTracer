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

private:
    // Not serialized. Initialised here, not in a constructor, so the generated
    // serializer constructor (scene load, paste, Add Component) gets it too.
    glm::vec3 m_localDirection{ 0.0f, -1.0f, 0.0f };

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(DirectionalLight, LightBase);
};

GBE_REGISTER_SERIALIZED_TYPE(DirectionalLight, ComponentBase);