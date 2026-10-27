#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"
#include "Example/PlayerInput.hpp"

#include <vector>

class ParticleSystemComponent;

// Reads ship input and forwards it to PlayerMovementComponent and ShipWeapon on the same object.
class PlayerController : public ComponentBase, public gbe::ITrigger<UpdateTrigger>, public gbe::IInstanceManager<PlayerController> {
public:
    PlayerController(gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~PlayerController() noexcept override = default;

    PlayerController(const PlayerController&) = delete;
    PlayerController& operator=(const PlayerController&) = delete;

    PlayerController(PlayerController&&) = default;
    PlayerController& operator=(PlayerController&&) = default;

    void OnStart() override;
    void OnUpdate(float deltaTime) override;

private:
    void SetThrustParticles(bool active);

    PlayerInput m_input;
    std::vector<ParticleSystemComponent*> m_thrustParticles;
    bool m_particlesActive = false;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(PlayerController, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(PlayerController, ComponentBase);
