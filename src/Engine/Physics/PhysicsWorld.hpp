#pragma once

#include "PhysicsDefines.hpp"
#include "PhysicsBody.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

namespace Anito::Physics {

	/**
	 * @brief Represents a single physics simulation world
	 * 
	 * A PhysicsWorld manages a complete physics simulation including:
	 * - Body creation and destruction
	 * - Physics stepping/simulation
	 * - Collision detection and response
	 * - Constraint management and more
	 * 
	 * Typically, you'd have one PhysicsWorld per Scene.
	 */
	class PhysicsWorld : public std::enable_shared_from_this<PhysicsWorld> {
	public:
		/**
		 * @brief Create a new physics world with given settings
		 */
		static PhysicsWorldPtr Create(const PhysicsWorldSettings& settings = PhysicsWorldSettings());

		~PhysicsWorld();

		// --- Body Management ---

		/**
		 * @brief Create a new physics body in this world
		 * @param position Initial world position
		 * @param rotation Initial world rotation
		 * @param settings Body configuration
		 * @return Shared pointer to the created body
		 */
		PhysicsBodyPtr CreateBody(
			const glm::vec3& position,
			const glm::quat& rotation,
			const PhysicsBodySettings& settings
		);

		/**
		 * @brief Create a body with identity rotation
		 */
		PhysicsBodyPtr CreateBody(
			const glm::vec3& position,
			const PhysicsBodySettings& settings
		) {
			return CreateBody(position, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), settings);
		}

		/**
		 * @brief Remove a body from the world
		 */
		void DestroyBody(const PhysicsBodyPtr& body);

		/**
		 * @brief Remove all bodies from the world
		 */
		void Clear();

		/**
		 * @brief Get a body by its ID
		 */
		PhysicsBodyPtr GetBody(uint32_t bodyId) const;

		/**
		 * @brief Get all bodies in the world
		 */
		std::vector<PhysicsBodyPtr> GetAllBodies() const;

		/**
		 * @brief Get number of active bodies
		 */
		uint32_t GetBodyCount() const;

		// --- Simulation ---

		/**
		 * @brief Step the physics simulation by deltaTime
		 * @param deltaTime Time step in seconds
		 */
		void StepSimulation(float deltaTime);

		/**
		 * @brief Set the gravity for this world
		 */
		void SetGravity(const glm::vec3& gravity);

		/**
		 * @brief Get the gravity
		 */
		glm::vec3 GetGravity() const;

		/**
		 * @brief Set the time step for physics simulation
		 */
		void SetTimeStep(float timeStep);

		/**
		 * @brief Get the current time step
		 */
		float GetTimeStep() const { return mSettings.timeStep; }

		// --- Queries ---

		/**
		 * @brief Cast a ray and get the closest hit
		 * @param from Start position
		 * @param to End position
		 * @param outBodyId Output parameter for hit body ID
		 * @param outPosition Output parameter for hit position
		 * @param outNormal Output parameter for hit surface normal
		 * @param outDistance Output parameter for distance traveled
		 * @return true if something was hit
		 */
		bool RayCast(
			const glm::vec3& from,
			const glm::vec3& to,
			uint32_t& outBodyId,
			glm::vec3& outPosition,
			glm::vec3& outNormal,
			float& outDistance
		) const;

		/**
		 * @brief Get all bodies overlapping an AABB
		 * @param min Minimum corner of bounding box
		 * @param max Maximum corner of bounding box
		 */
		std::vector<PhysicsBodyPtr> GetBodiesInAABB(
			const glm::vec3& min,
			const glm::vec3& max
		) const;

		/**
		 * @brief Get all bodies within a radius around a point
		 */
		std::vector<PhysicsBodyPtr> GetBodiesInSphere(
			const glm::vec3& center,
			float radius
		) const;

		// --- Events ---

		/**
		 * @brief Register a collision event callback
		 * @param callback Function to call when collision events occur
		 */
		void SetCollisionCallback(std::function<void(const CollisionEvent&)> callback);

		/**
		 * @brief Clear all collision callbacks
		 */
		void ClearCollisionCallbacks();

		// --- Settings ---

		/**
		 * @brief Get the world settings
		 */
		const PhysicsWorldSettings& GetSettings() const { return mSettings; }

		/**
		 * @brief Update world settings
		 */
		void SetSettings(const PhysicsWorldSettings& settings);

		// --- Debug ---

		/**
		 * @brief Enable or disable debug visualization
		 */
		void SetDebugDrawing(bool enabled) { mDebugDrawingEnabled = enabled; }

		/**
		 * @brief Check if debug drawing is enabled
		 */
		bool IsDebugDrawingEnabled() const { return mDebugDrawingEnabled; }

	private:
		PhysicsWorld(const PhysicsWorldSettings& settings);

		bool Initialize();

		PhysicsWorldSettings mSettings;
		std::unique_ptr<JoltPhysicsSystem> mPhysicsSystem;
		std::unordered_map<uint32_t, PhysicsBodyPtr> mBodies;
		std::unordered_map<uint32_t, JPH::BodyID> mBodyIdMapping;  // Maps Anito body ID to Jolt body ID
		std::vector<std::function<void(const CollisionEvent&)>> mCollisionCallbacks;
		bool mDebugDrawingEnabled = false;
		float mAccumulatedTime = 0.0f;

		// Helper to get Jolt body from BodyID
		JPH::Body* GetJoltBody(JPH::BodyID bodyId) const;

		friend class PhysicsEngine;
		friend class PhysicsBody;
	};

} // namespace Anito::Physics
