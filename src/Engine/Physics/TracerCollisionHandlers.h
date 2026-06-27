#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>

#include "TracerPhysics.h"

#include <iostream>

using namespace JPH;

class TracerCollisionHandlers : public ContactListener {


    void ChangeColor(GameObject* obj);

    // Forwarding OnContactValidate
    virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override;

    // Forwarding OnContactAdded
    virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;

    // Forwarding OnContactPersisted
    virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;

    // Forwarding OnContactRemoved
    virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

};