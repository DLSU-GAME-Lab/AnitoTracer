#pragma once

#include "LightBase.hpp"
#include "../Transform.hpp"

class PointLight : public LightBase {
public:
    // Initializes the point light with a default range matching the pipeline definition
    PointLight(HierarchyObject* owner = nullptr)
        : LightBase("PointLight", owner),
        m_range(10.0f) {} // Default range of 10.0f matches PointLightData

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

private:
    float m_range;
};