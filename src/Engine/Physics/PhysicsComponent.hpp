#pragma once

#include "PhysicsDefines.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <stdexcept>

// Forward declare GameObject (not in Anito namespace)
class GameObject;

namespace Anito::Physics {

	/**
	 * @brief A component attached to GameObjects to give them physics
	 * 
	 * PhysicsComponent acts as a bridge between the GameObject/Transform system
	 * and the physics engine. It manages synchronization between the two.
	 */
	class PhysicsComponent {
	public:
		PhysicsComponent(GameObject* owner, const PhysicsBodySettings& settings);

		~PhysicsComponent();

		// --- Lifecycle ---

		/**
		 * @brief Initialize the physics component
		 * (Should be called when GameObject is added to scene)
		 */
		void Initialize();

		/**
		 * @brief Update the physics component
		 * (Called each frame to sync transform)
		 */
		void Update(float deltaTime);

		/**
		 * @brief Clean up the physics component
		 */
		void Cleanup();

		// --- Physics Body Access ---

		/**
		 * @brief Get the underlying physics body
		 */
		PhysicsBodyPtr GetPhysicsBody() const { return mPhysicsBody; }

		/**
		 * @brief Set the physics body (internal use)
		 */
		void SetPhysicsBody(PhysicsBodyPtr body) { mPhysicsBody = body; }

		// --- Transform Synchronization ---

		/**
		 * @brief Enable/disable automatic transform synchronization
		 * If enabled, the GameObject transform is updated from physics each frame
		 */
		void SetAutoSyncTransform(bool enabled) { mAutoSyncTransform = enabled; }

		/**
		 * @brief Manually sync physics transform to GameObject transform
		 */
		void SyncPhysicsToTransform();

		/**
		 * @brief Manually sync GameObject transform to physics
		 */
		void SyncTransformToPhysics();

		// --- Physics Queries (Convenience) ---

		glm::vec3 GetPosition() const;
		glm::vec3 GetVelocity() const;
		glm::vec3 GetAngularVelocity() const;

		// --- Physics Modification (Convenience) ---

		void SetVelocity(const glm::vec3& velocity);
		void ApplyForce(const glm::vec3& force);
		void ApplyImpulse(const glm::vec3& impulse);

		// --- Settings ---

		const PhysicsBodySettings& GetSettings() const { return mSettings; }

		void SetEnabled(bool enabled) { mEnabled = enabled; }
		bool IsEnabled() const { return mEnabled; }

	private:
		GameObject* mOwner = nullptr;
		PhysicsBodyPtr mPhysicsBody;
		PhysicsBodySettings mSettings;
		bool mAutoSyncTransform = true;
		bool mEnabled = true;
		bool mInitialized = false;
	};

} // namespace Anito::Physics
