#pragma once

#include "PhysicsDefs.h"
#include "PhysicsUtils.hpp"

class PhysicsEngine {
	private:
		static PhysicsEngine* instance;

		// Private constructor - cannot be instantiated directly
		PhysicsEngine();
		~PhysicsEngine();

		// Delete copy constructor and assignment operator
		PhysicsEngine(const PhysicsEngine&) = delete;
		PhysicsEngine& operator=(const PhysicsEngine&) = delete;

		// Move local variables to class members to persist memory
		TempAllocatorImpl* temp_allocator = nullptr;
		JobSystemThreadPool* job_system = nullptr;

		// Concrete implementations must also persist
		BPLayerInterfaceImpl* broad_phase_layer_interface = nullptr;
		ObjectVsBroadPhaseLayerFilterImpl* object_vs_broadphase_layer_filter = nullptr;
		ObjectLayerPairFilterImpl* object_vs_object_layer_filter = nullptr;

		MyBodyActivationListener* body_activation_listener = nullptr;
		MyContactListener* contact_listener = nullptr;

		BodyInterface* body_interface = nullptr;

	public:
		// Get the singleton instance
		static PhysicsEngine* GetInstance();
		static void DestroyInstance();

		// Initialize the physics engine
		void InitializeEngine();
		void Step(float deltaTime);

		void CreateDefaultFloor(glm::vec3 pos);
		void CreateDefaultBall(float r, glm::vec3 pos);

		BodyID CreateSphere(float r, glm::vec3 pos, float restitution = 0.8f);

		// Public physics system member
		PhysicsSystem physics_system;

		// Constants
		const uint cMaxBodies = 1024;
		const uint cNumBodyMutexes = 0;
		const uint cMaxBodyPairs = 1024;
		const uint cMaxContactConstraints = 1024;
		const uint cCollisionSteps = 4; // Increased for better collision detection

		BodyID ball;
};