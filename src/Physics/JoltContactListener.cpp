#include "JoltContactListener.hpp"
#include "JoltPhysicsEngine.hpp"
#include "JoltPhysicsBody.hpp"

JoltContactListener::JoltContactListener(JoltPhysicsEngine& engine) : mEngine(engine) {}

void JoltContactListener::OnContactAdded(
	const JPH::Body& inBody1,
	const JPH::Body& inBody2,
	const JPH::ContactManifold& inManifold,
	JPH::ContactSettings& ioSettings) {
	if (!mEngine.HasCollisionCallback()) {
		return;
	}

	auto bodyA = mEngine.FindBodyByID(inBody1.GetID());
	auto bodyB = mEngine.FindBodyByID(inBody2.GetID());

	if (bodyA && bodyB) {
		JPH::RVec3 contactPoint = inManifold.GetWorldSpaceContactPointOn1(0);
		glm::vec3 hitPos(contactPoint.GetX(), contactPoint.GetY(), contactPoint.GetZ());
		mEngine.InvokeCollisionCallback(bodyA, bodyB, hitPos);
	}
}