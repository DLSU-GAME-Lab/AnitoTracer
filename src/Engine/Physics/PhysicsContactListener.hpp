#pragma once

#include "PhysicsDefines.hpp"

namespace JPH {
	class ContactListener;
}

namespace Anito::Physics {

	/**
	 * @brief Handles physics collision callbacks
	 * 
	 * Implements Jolt's contact listener interface to handle collision events
	 * and forward them to registered callbacks in the PhysicsWorld.
	 */
	class PhysicsContactListener {
	public:
		PhysicsContactListener(PhysicsWorld* world);
		~PhysicsContactListener();

		/**
		 * @brief Get the underlying Jolt contact listener
		 */
		JPH::ContactListener* GetJoltListener() const;

	private:
		PhysicsWorld* mWorld = nullptr;
		JPH::ContactListener* mJoltListener = nullptr;

		friend class PhysicsWorld;
	};

} // namespace Anito::Physics
