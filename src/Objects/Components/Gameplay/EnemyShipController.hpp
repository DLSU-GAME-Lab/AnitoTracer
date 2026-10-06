#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"
#include "AssignableEvent/MethodRegistry.hpp"

// Simple AI: steers toward the player through PlayerMovementComponent and fires the ShipWeapon on the same object.
class EnemyShipController : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    EnemyShipController(gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~EnemyShipController() override = default;

    EnemyShipController(const EnemyShipController&) = delete;
    EnemyShipController& operator=(const EnemyShipController&) = delete;

    EnemyShipController(EnemyShipController&&) = default;
    EnemyShipController& operator=(EnemyShipController&&) = default;

    void OnUpdate(float deltaTime) override;

    // Exposed as a UnityFunction; deletes this ship's object.
    void OnDestroy();

private:
    float m_turnGain = 2.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_turnGain, "Turn Gain");

    float m_keepDistance = 25.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_keepDistance, "Keep Distance");

    float m_fireRange = 80.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_fireRange, "Fire Range");

    float m_fireConeDegrees = 8.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_fireConeDegrees, "Fire Cone (deg)");

    bool m_healthHooked = false;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(EnemyShipController, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(EnemyShipController, ComponentBase);

GBE_REGISTER_METHOD(EnemyShipController, OnDestroy);
