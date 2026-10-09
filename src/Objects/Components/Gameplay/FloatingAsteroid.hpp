#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/FixedUpdateTrigger.hpp"
#include "AssignableEvent/MethodRegistry.hpp"

#include <glm/glm.hpp>

// Obstacle that cancels world gravity so it hovers in place, slowly tumbling.
class FloatingAsteroid : public ComponentBase, public gbe::ITrigger<FixedUpdateTrigger> {
public:
    FloatingAsteroid(gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~FloatingAsteroid() override = default;

    FloatingAsteroid(const FloatingAsteroid&) = delete;
    FloatingAsteroid& operator=(const FloatingAsteroid&) = delete;

    FloatingAsteroid(FloatingAsteroid&&) = default;
    FloatingAsteroid& operator=(FloatingAsteroid&&) = default;

    void OnFixedUpdate(float deltaTime) override;

    // Exposed as a UnityFunction; deletes this asteroid's object.
    void OnDestroy();

private:
    glm::vec3 m_tumbleSpeed = glm::vec3(0.1f, 0.2f, 0.05f);
    GBE_SERIALIZE_FIELD_W_NAME(m_tumbleSpeed, "Tumble Speed (rad/s)");

    bool m_tumbleApplied = false;
    bool m_healthHooked = false;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(FloatingAsteroid, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(FloatingAsteroid, ComponentBase);

GBE_REGISTER_METHOD(FloatingAsteroid, OnDestroy);
