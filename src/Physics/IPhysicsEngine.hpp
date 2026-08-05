#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

class IPhysicsBody;

class IPhysicsEngine {
public:
	virtual ~IPhysicsEngine() = default;

	// World management
	virtual void SetGravity(const glm::vec3& gravity) = 0;
	virtual glm::vec3 GetGravity() const = 0;

	// Body management
	virtual std::shared_ptr<IPhysicsBody> CreateRigidBody(
		const glm::vec3& position,
		const glm::quat& rotation,
		float mass
	) = 0;
	virtual void DestroyRigidBody(std::shared_ptr<IPhysicsBody> body) = 0;

	// Simulation
	virtual void Step(float deltaTime) = 0;

	// Ray casting
	virtual bool Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, std::shared_ptr<IPhysicsBody>& outBody, glm::vec3& outHitPoint) = 0;
};

class IPhysicsBody {
public:
	virtual ~IPhysicsBody() = default;

	// Transform
	virtual void SetPosition(const glm::vec3& position) = 0;
	virtual glm::vec3 GetPosition() const = 0;

	virtual void SetRotation(const glm::quat& rotation) = 0;
	virtual glm::quat GetRotation() const = 0;

	// Phsyics properties
	virtual void SetMass(float mass) = 0;
	virtual float GetMass() const = 0;

	virtual void SetVelocity(const glm::vec3& velocity) = 0;
	virtual glm::vec3 GetVelocity() const = 0;

	virtual void SetAngularVelocity(const glm::vec3& angularVelocity) = 0;
	virtual glm::vec3 GetAngularVelocity() const = 0;

	// Forces
	virtual void ApplyForce(const glm::vec3& force) = 0;
	virtual void ApplyImpulse(const glm::vec3& impulse) = 0;
};