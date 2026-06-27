#pragma once
#include "PhysicsEngine.h"
#include "PhysicsUtils.hpp"
#include "../../From-GDGRAP2/GameObject.h"

#include "TracerCollisionHandlers.h"

#include <memory>
#include <vector>

/**
 * @struct PhysicsBodyPair
 * @brief Tracks the relationship between a GameObject and its corresponding physics BodyID
 *
 * This structure maintains synchronization between the game object hierarchy and the physics engine,
 * allowing for easy updates and data synchronization between the two systems.
 */
struct PhysicsBodyPair
{
	GameObject* gameObject;	///< Unique pointer to the associated GameObject
	JPH::BodyID bodyID;						///< Corresponding physics engine BodyID

	/**
	 * @brief Constructor for PhysicsBodyPair
	 * @param go Unique pointer to the GameObject
	 * @param id The physics engine BodyID
	 */
	PhysicsBodyPair(GameObject* go, JPH::BodyID id)
		: gameObject(go), bodyID(id) {}
};


class TracerPhysics {
public:

	//Manual adding of physics bodies for testing purposes
	//Sponza ver
	void AddSponzaColliders();

	/**
	 * @brief Gets the singleton instance of TracerPhysics
	 * @return Reference to the singleton instance
	 */
	static TracerPhysics& GetInstance() {
		static TracerPhysics instance;
		return instance;
	}

	/**
	 * @brief Initializes the physics scene with default floor
	 */
	void Initialize();

	void AddSphere(GameObject* obj);
	void AddBox(GameObject* obj, bool isStatic = false);

	void Step(float deltaTime, bool broadcastSceneDirty = false);

	/**
	 * @brief Synchronizes physics engine bodies to GameObjects
	 * 
	 * Updates all GameObjects' positions and rotations to match their corresponding
	 * physics bodies. This should be called after physics simulation steps to reflect
	 * the current state of the physics world in the scene hierarchy.
	 * 
	 * @param broadcastDirty If true, broadcasts scene dirty event (expensive, use sparingly).
	 *                       If false, only updates GameObject transforms (fast).
	 */
	void SyncPhysicsToGameObjects(bool broadcastDirty = false);

	/**
	 * @brief Toggles a physics body's activation state
	 *
	 * Toggles the specified GameObject's physics body between active and inactive states.
	 * If activate is true, body will be set to EActivation::Activate.
	 * If activate is false, body will be set to EActivation::DontActivate.
	 *
	 * @param obj The GameObject whose physics body should be toggled
	 * @param activate If true, activates the body; if false, deactivates it
	 */
	void ToggleBodyActivation(GameObject* obj, bool activate);

	/**
	 * @brief Updates a physics body's position from a GameObject
	 *
	 * Sets the physics body's position to the GameObject's current world position.
	 *
	 * @param obj The GameObject whose position should be synced to the physics body
	 */
	void UpdateBodyPosition(GameObject* obj);

	/**
	 * @brief Updates a physics body's rotation from a GameObject
	 *
	 * Sets the physics body's rotation to the GameObject's current world rotation.
	 *
	 * @param obj The GameObject whose rotation should be synced to the physics body
	 */
	void UpdateBodyRotation(GameObject* obj);

	/**
	 * @brief Updates a physics body's size/scale from a GameObject
	 *
	 * Sets the physics body's scale to the GameObject's current world scale.
	 * Note: Only works for bodies that support dynamic scaling.
	 *
	 * @param obj The GameObject whose scale should be synced to the physics body
	 */
	void UpdateBodySize(GameObject* obj);

	/**
	 * @brief Force sets a physics body's position to a specific value
	 *
	 * Sets the physics body's position to the provided value, regardless of the GameObject's current position.
	 *
	 * @param obj The GameObject associated with the physics body
	 * @param position The new position to set (in world space)
	 */
	void SetBodyPosition(GameObject* obj, const glm::vec3& position);

	/**
	 * @brief Force sets a physics body's rotation to a specific value
	 *
	 * Sets the physics body's rotation to the provided quaternion value, regardless of the GameObject's current rotation.
	 *
	 * @param obj The GameObject associated with the physics body
	 * @param rotation The new rotation to set (as a quaternion)
	 */
	void SetBodyRotation(GameObject* obj, const glm::quat& rotation);

	/**
	 * @brief Force sets a physics body's size/scale to a specific value
	 *
	 * Attempts to set the physics body's size to the provided scale value.
	 * Note: Direct shape scaling is not supported in Jolt Physics; consider recreating the body instead.
	 *
	 * @param obj The GameObject associated with the physics body
	 * @param scale The new scale to set
	 */
	void SetBodySize(GameObject* obj, const glm::vec3& scale);

	/**
	 * @brief Applies a force to a physics body
	 *
	 * Applies the specified force vector to the physics body associated with the given GameObject.
	 * The force is applied immediately and will affect the body's motion if it's dynamic.
	 * For static or sleeping bodies, this may not have the desired effect.
	 *
	 * @param obj The GameObject whose physics body should receive the force
	 * @param force The force vector to apply (in world space)
	 */
	void ApplyForce(GameObject* obj, const glm::vec3& force);

	/**
	 * @brief Retrieves the GameObject associated with a physics BodyID
	 *
	 * Searches through the physics body pairs to find the GameObject that corresponds
	 * to the given BodyID. This is useful for reverse lookups when you have a BodyID
	 * from the physics engine and need to find its associated game object.
	 *
	 * @param bodyID The physics engine BodyID to search for
	 * @return Pointer to the GameObject if found, nullptr if not found or BodyID is invalid
	 */
	GameObject* GetGameObjectByBodyID(JPH::BodyID bodyID) const;

	// Delete copy and move constructors/assignments to prevent copies
	TracerPhysics(const TracerPhysics&) = delete;
	TracerPhysics& operator=(const TracerPhysics&) = delete;
	TracerPhysics(TracerPhysics&&) = delete;
	TracerPhysics& operator=(TracerPhysics&&) = delete;

private:
	//friend class std::make_unique<TracerPhysics>;

	/**
	 * @brief Private constructor for singleton pattern
	 */
	TracerPhysics() = default;

	void AddPair(GameObject* obj, JPH::BodyID id);

	// Tracking container for GameObject and BodyID pairs
	std::vector<PhysicsBodyPair> physics_body_pairs;	///< List tracking GameObjects and their paired BodyIDs

	bool floor_initialized = false; ///< Flag to track if default floor has been created
	bool sponza_colliders_added = false; ///< Flag to track if Sponza colliders have been added
};
