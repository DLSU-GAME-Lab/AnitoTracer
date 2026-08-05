#pragma once

#include "IPhysicsEngine.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <map>

class JoltPhysicsBody;

class JoltPhysicsEngine : public IPhysicsEngine {
public:
	JoltPhysicsEngine();
	~JoltPhysicsEngine() override;

	// World management
	void SetGravity(const glm::vec3& gravity) override;
	glm::vec3 GetGravity() const override;

	// Body management
	std::shared_ptr<IPhysicsBody> CreateRigidBody(
		const glm::vec3& position,
		const glm::quat& rotation,
		float mass
	) override;
	void DestroyRigidBody(std::shared_ptr<IPhysicsBody> body) override;

	// Simulation
	void Step(float deltaTime) override;

	// Raycasting
	bool Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, std::shared_ptr<IPhysicsBody>& outBody, glm::vec3& outHitPoint) override;

	// Internal access for JoltPhysicsBody
	JPH::PhysicsSystem* GetPhysicsSystem() { return mPhysicsSystem.get(); }
	JPH::BodyInterface& GetBodyInterface() { return mPhysicsSystem->GetBodyInterface(); }

private:
	std::unique_ptr<JPH::JobSystemThreadPool> mJobSystem;
	std::unique_ptr<JPH::PhysicsSystem> mPhysicsSystem;

	std::map<JPH::BodyID, std::shared_ptr<JoltPhysicsBody>> mBodies;
	glm::vec3 mGravity;
};