#pragma once

#include "IPhysicsBody.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <vector>
#include <functional>

class IPhysicsEngine {
public:
	virtual ~IPhysicsEngine() = default;

	enum class ShapeType {
		Box,
		Sphere,
		Capsule
	};

	// Generic shape parameters
	struct ShapeParams {
		glm::vec3 v = glm::vec3(1.0f); // For box: half extents, for sphere: radius in x, for capsule: radius in x and height in y
	};

	// Collision callback
	using CollisionCallback = std::function<void(std::shared_ptr<IPhysicsBody>, std::shared_ptr<IPhysicsBody>, const glm::vec3&)>;

	// World management
	virtual void SetGravity(const glm::vec3& gravity) = 0;
	virtual glm::vec3 GetGravity() const = 0;

	// Body management
	virtual std::shared_ptr<IPhysicsBody> CreateRigidBody(
		const glm::vec3& position,
		const glm::quat& rotation,
		float mass,
		ShapeType shape = ShapeType::Box,
		const ShapeParams& shapeParams = {}
	) = 0;
	virtual void DestroyRigidBody(std::shared_ptr<IPhysicsBody> body) = 0;

	// Collision callback
	virtual void SetCollisionCallback(CollisionCallback callback) = 0;

	// Simulation
	virtual void Step(float deltaTime) = 0;

	// Ray casting
	virtual bool Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, std::shared_ptr<IPhysicsBody>& outBody, glm::vec3& outHitPoint) = 0;
};