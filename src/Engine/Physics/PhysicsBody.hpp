#pragma once

#include "PhysicsDefines.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <functional>
#include <Jolt/Physics/Body/BodyID.h>

// Forward declare PhysicsWorld to avoid circular dependency
namespace Anito::Physics {
	class PhysicsWorld;
}

namespace Anito::Physics {

	/**
	 * @brief Wraps a Jolt physics body with a convenient interface
	 * 
	 * This class provides a user-friendly wrapper around Jolt's physics bodies,
	 * handling position, rotation, velocity, and force application with GLM types.
	 */
	class PhysicsBody {
	public:
		PhysicsBody(
			uint32_t bodyId,
			const glm::vec3& position,
			const glm::quat& rotation,
			const PhysicsBodySettings& settings,
			PhysicsWorld* physicsWorld = nullptr,
			JPH::BodyID joltBodyId = JPH::BodyID()
		);

		~PhysicsBody();

		// --- Position and Rotation ---

		/**
		 * @brief Get the current world position
		 */
		glm::vec3 GetPosition() const;

		/**
		 * @brief Set the world position
		 */
		void SetPosition(const glm::vec3& position);

		/**
		 * @brief Get the current world rotation as a quaternion
		 */
		glm::quat GetRotation() const;

		/**
		 * @brief Set the world rotation from a quaternion
		 */
		void SetRotation(const glm::quat& rotation);

		/**
		 * @brief Get the current transformation matrix
		 */
		glm::mat4 GetTransform() const;

		// --- Velocity and Angular Velocity ---

		/**
		 * @brief Get the linear velocity
		 */
		glm::vec3 GetLinearVelocity() const;

		/**
		 * @brief Set the linear velocity
		 */
		void SetLinearVelocity(const glm::vec3& velocity);

		/**
		 * @brief Get the angular velocity (radians per second)
		 */
		glm::vec3 GetAngularVelocity() const;

		/**
		 * @brief Set the angular velocity
		 */
		void SetAngularVelocity(const glm::vec3& angularVelocity);

		// --- Forces and Impulses ---

		/**
		 * @brief Apply a force at the body's center of mass
		 * @param force The force to apply (Newtons)
		 */
		void ApplyForce(const glm::vec3& force);

		/**
		 * @brief Apply a force at a specific world position
		 * @param force The force to apply
		 * @param position The world position to apply the force at
		 */
		void ApplyForceAtPoint(const glm::vec3& force, const glm::vec3& position);

		/**
		 * @brief Apply an impulse (instant change in velocity)
		 * @param impulse The impulse to apply (kg*m/s)
		 */
		void ApplyImpulse(const glm::vec3& impulse);

		/**
		 * @brief Apply a torque (rotational force)
		 * @param torque The torque to apply
		 */
		void ApplyTorque(const glm::vec3& torque);

		/**
		 * @brief Apply angular impulse (instant rotational velocity change)
		 */
		void ApplyAngularImpulse(const glm::vec3& angularImpulse);

		/**
		 * @brief Clear all forces from the body
		 */
		void ClearForces();

		// --- Properties ---

		/**
		 * @brief Get the body ID as used by Jolt
		 */
		uint32_t GetBodyId() const { return mBodyId; }

		/**
		 * @brief Get the mass in kilograms
		 */
		float GetMass() const;

		/**
		 * @brief Set the mass in kilograms
		 */
		void SetMass(float mass);

		/**
		 * @brief Get the inverse mass (1/mass)
		 */
		float GetInverseMass() const;

		/**
		 * @brief Get the body type
		 */
		BodyType GetBodyType() const { return mSettings.type; }

		/**
		 * @brief Check if body is dynamic
		 */
		bool IsDynamic() const { return mSettings.type == BodyType::DYNAMIC; }

		/**
		 * @brief Check if body is static
		 */
		bool IsStatic() const { return mSettings.type == BodyType::STATIC; }

		/**
		 * @brief Check if body is kinematic
		 */
		bool IsKinematic() const { return mSettings.type == BodyType::KINEMATIC; }

		/**
		 * @brief Check if body is active (awake)
		 */
		bool IsActive() const;

		/**
		 * @brief Wake up a sleeping body
		 */
		void SetActive(bool active);

		/**
		 * @brief Get the physics material
		 */
		const PhysicsMaterial& GetMaterial() const { return mSettings.material; }

		/**
		 * @brief Set the friction coefficient
		 */
		void SetFriction(float friction);

		/**
		 * @brief Set the restitution (bounciness)
		 */
		void SetRestitution(float restitution);

		/**
		 * @brief Set linear damping
		 */
		void SetLinearDamping(float damping);

		/**
		 * @brief Set angular damping
		 */
		void SetAngularDamping(float damping);

		// --- Collision ---

		/**
		 * @brief Get the object layer
		 */
		ObjectLayer GetObjectLayer() const { return mSettings.layer; }

		/**
		 * @brief Set the object layer
		 */
		void SetObjectLayer(ObjectLayer layer);

		// --- Bounds ---

		/**
		 * @brief Get axis-aligned bounding box
		 */
		void GetAABB(glm::vec3& outMin, glm::vec3& outMax) const;

		// --- State ---

		/**
		 * @brief Completely reset the body to initial state
		 */
		void Reset(const glm::vec3& position, const glm::quat& rotation);

	private:
		uint32_t mBodyId;                   ///< Our internal body ID
		JPH::BodyID mJoltBodyId;            ///< The Jolt physics body ID
		PhysicsWorld* mPhysicsWorld;        ///< Pointer to parent physics world
		PhysicsBodySettings mSettings;      ///< Cached settings
		glm::vec3 mLastPosition;            ///< For change detection
		glm::quat mLastRotation;            ///< For change detection

		friend class PhysicsWorld;
	};

} // namespace Anito::Physics
