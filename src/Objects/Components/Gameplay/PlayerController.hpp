#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"
#include "Example/PlayerInput.hpp"

// Reads ship input and forwards it to PlayerMovementComponent and ShipWeapon on the same object.
class PlayerController : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    PlayerController(gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~PlayerController() noexcept override = default;

    PlayerController(const PlayerController&) = delete;
    PlayerController& operator=(const PlayerController&) = delete;

    PlayerController(PlayerController&&) = default;
    PlayerController& operator=(PlayerController&&) = default;

    void OnUpdate(float deltaTime) override;

private:
    PlayerInput m_input;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(PlayerController, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(PlayerController, ComponentBase);
