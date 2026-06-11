#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <memory>

// Forward declarations
namespace JPH {
	class PhysicsSystem;
	class Body;
	class Shape;
	class BodyInterface;
	class BroadPhaseLayerInterface;
	class ObjectLayerPairFilter;
	class ContactListener;
}

namespace Anito::Physics {
	// Type aliases for common Jolt types
	using JoltPhysicsSystem = JPH::PhysicsSystem;
	using JoltBody = JPH::Body;
	using JoltShape = JPH::Shape;
	using JoltBodyInterface = JPH::BodyInterface;

	// Object layers for collision filtering
	enum class ObjectLayer : uint8_t {
		STATIC = 0,      // Static geometry (terrain, buildings)
		DYNAMIC = 1,     // Dynamic objects (characters, props)
		TRIGGER = 2,     // Trigger volumes
		KINEMATIC = 3,   // Kinematic bodies (moving platforms, controlled objects)
		RAGDOLL = 4,
		NUM_LAYERS = 5
	};

	// Broad phase layers for broad phase filtering
	enum class BroadPhaseLayer : uint8_t {
		STATIC = 0,
		DYNAMIC = 1,
		NUM_LAYERS = 2
	};

	// Physics body types
	enum class BodyType {
		STATIC,      // Immovable
		DYNAMIC,     // Affected by physics
		KINEMATIC    // Moved by user, doesn't respond to forces normally
	};

	// Physics shape types
	enum class ShapeType {
		BOX,
		SPHERE,
		CAPSULE,
		CYLINDER,
		CONVEX_HULL,
		MESH,           // Static trimesh
		COMPOUND        // Multiple shapes
	};

	// Physics material properties
	struct PhysicsMaterial {
		float friction;          // 0.0 - 1.0+, higher = more friction
		float restitution;       // 0.0 - 1.0, higher = more bouncy
		float linearDamping;     // Damping for linear velocity
		float angularDamping;    // Damping for angular velocity
		bool isSensor;           // Collision without physics response
		uint16_t density;        // For automatic mass calculation

		PhysicsMaterial(
			float f = 0.5f,
			float r = 0.3f,
			float ld = 0.01f,
			float ad = 0.05f,
			bool s = false,
			uint16_t d = 1000
		) : friction(f), restitution(r), linearDamping(ld),
			angularDamping(ad), isSensor(s), density(d) {}
	};

	// Physics body creation settings
	struct PhysicsBodySettings {
		BodyType type = BodyType::DYNAMIC;
		ObjectLayer layer = ObjectLayer::DYNAMIC;
		PhysicsMaterial material;
		float mass = 1.0f;           // kg, ignored for static/kinematic
		bool useGravity = true;
		bool isLocked = false;       // Lock rotation/movement
		bool lockX = false;
		bool lockY = false;
		bool lockZ = false;
		bool lockRotX = false;
		bool lockRotY = false;
		bool lockRotZ = false;
	};

	// Physics world settings
	struct PhysicsWorldSettings {
		glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
		float timeStep = 1.0f / 60.0f;          // 60 Hz physics simulation
		uint32_t velocityIterations = 10;
		uint32_t positionIterations = 2;
		bool enableSleeping = true;
		float sleepThreshold = 0.03f;           // How still before going to sleep
		uint32_t maxBodies = 10000;
		uint32_t maxBodyPairs = 65536;
		uint32_t maxContactConstraints = 20480;
	};

	// Forward declarations of main classes
	class PhysicsBody;
	class PhysicsWorld;
	class PhysicsEngine;
	class PhysicsComponent;
	class PhysicsContactListener;
	class PhysicsDebugRenderer;

	// Type aliases - must be before CollisionEvent to avoid incomplete type issues
	using PhysicsBodyPtr = std::shared_ptr<PhysicsBody>;
	using PhysicsWorldPtr = std::shared_ptr<PhysicsWorld>;

	// Collision event data
	struct CollisionEvent {
		enum class Type {
			BEGIN,      // Collision just started
			PERSIST,    // Collision continuing
			END         // Collision just ended
		};

		Type eventType;
		PhysicsBody* bodyA;
		PhysicsBody* bodyB;
		glm::vec3 contactPoint;
		glm::vec3 contactNormal;
		float impactForce;
	};

	// Constants
	constexpr float GRAVITY_STRENGTH = 9.81f;
	constexpr float DEFAULT_TIME_STEP = 1.0f / 60.0f;
	constexpr uint32_t THREAD_POOL_SIZE = 4;

} // namespace Anito::Physics
