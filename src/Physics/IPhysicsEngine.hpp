#pragma once

#include "IPhysicsBody.hpp"
#include "../Common/CollisionMeshData.hpp"
#include "../Common/DebugLine.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <vector>
#include <functional>

// Physics event types
struct CollisionEnterTrigger {
	std::shared_ptr<IPhysicsBody> self;
	std::shared_ptr<IPhysicsBody> other;
	glm::vec3 contactPoint;
};

class IPhysicsEngine {
public:
	virtual ~IPhysicsEngine() = default;

	enum class ShapeType {
		Box,
		Sphere,
		Capsule,
		Mesh,
		ConvexHull
	};

	// Generic shape parameters
	struct ShapeParams {
		glm::vec3 v = glm::vec3(1.0f); // For box: half extents, for sphere: radius in x, for capsule: radius in x and half-height in y
	};

	struct ColliderShape {
		ShapeType type;
		ShapeParams params;
		glm::vec3 offset = glm::vec3(0.0f);
		glm::vec3 scale = glm::vec3(0.0f);
		std::shared_ptr<const CollisionMeshData> meshData;
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
		const std::vector<ColliderShape>& shapes = {},
		float restitution = 0.0f
	) = 0;
	virtual void DestroyRigidBody(std::shared_ptr<IPhysicsBody> body) = 0;

	virtual bool SetShapes(IPhysicsBody* body, const std::vector<ColliderShape>& shapes) = 0;
	virtual bool SetRestitution(IPhysicsBody* body, float restitution) = 0;
	virtual bool SetFriction(IPhysicsBody* body, float friction) = 0;

	// Collision callback
	virtual void RegisterCollisionCallback(IPhysicsBody* body, CollisionCallback callback) = 0;
	virtual void UnregisterCollisionCallback(IPhysicsBody* body) = 0;

	// Simulation
	virtual void Step(float deltaTime) = 0;

	// Ray casting
	virtual bool Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, std::shared_ptr<IPhysicsBody>& outBody, glm::vec3& outHitPoint) = 0;

	// Activate bodies
	virtual void WakeBodiesAroundBody(IPhysicsBody* body) = 0;

	// Returns the current frame's debug-draw lines
	// (collider wireframes, contact points, etc.) for the renderer to display.
	// Returns empty when debug drawing is disabled or no bodies exist.
	virtual std::vector<DebugLineVertex> GetDebugLines() = 0;
};