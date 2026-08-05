#include "JoltPhysicsBody.hpp"

#include <Jolt/Physics/Body/BodyLockInterface.h>

JoltPhysicsBody::JoltPhysicsBody(JPH::BodyID bodyID, JPH::BodyInterface* bodyInterface, float mass)
	: mBodyID(bodyID), mBodyInterface(bodyInterface), mMass(mass) {
}