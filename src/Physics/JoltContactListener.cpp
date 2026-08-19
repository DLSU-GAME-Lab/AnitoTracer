#include "JoltContactListener.hpp"
#include "JoltPhysicsEngine.hpp"
#include "JoltPhysicsBody.hpp"

JoltContactListener::JoltContactListener(JoltPhysicsEngine& engine) : mEngine(engine) {}

void JoltContactListener::OnContactAdded(
	const JPH::Body& inBody1,
	const JPH::Body& inBody2,
	const JPH::ContactManifold& inManifold,
	JPH::ContactSettings& ioSettings) {
	auto bodyA = mEngine.FindBodyByID(inBody1.GetID());
	auto bodyB = mEngine.FindBodyByID(inBody2.GetID());

	if (!bodyA || !bodyB) {
		return;
	}

	JPH::RVec3 cp = inManifold.GetWorldSpaceContactPointOn1(0);
	glm::vec3 contactPoint(static_cast<float>(cp.GetX()), static_cast<float>(cp.GetY()), static_cast<float>(cp.GetZ()));

	mEngine.InvokeCollisionCallbacks(bodyA, bodyB, contactPoint);
}