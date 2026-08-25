#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class IPhysicsBody {
public:
	virtual ~IPhysicsBody() = default;

	// Transform
	virtual void SetPosition(const glm::vec3& position) = 0;
	virtual glm::vec3 GetPosition() const = 0;

	virtual void SetRotation(const glm::quat& rotation) = 0;
	virtual glm::quat GetRotation() const = 0;

	virtual void SetPositionAndRotation(const glm::vec3& position, const glm::quat& rotation) = 0;

	// Activation
	virtual void Activate() = 0;

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