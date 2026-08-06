#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>

class JoltPhysicsEngine;

class JoltContactListener : public JPH::ContactListener {
public:
    explicit JoltContactListener(JoltPhysicsEngine& engine);

    void OnContactAdded(
        const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold,
        JPH::ContactSettings& ioSettings) override;

private:
    JoltPhysicsEngine& mEngine;
};