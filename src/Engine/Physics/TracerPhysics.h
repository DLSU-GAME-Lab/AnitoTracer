#pragma once
#include "PhysicsEngine.h"
#include "PhysicsUtils.hpp"
#include "../../From-GDGRAP2/GameObject.h"

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
};
