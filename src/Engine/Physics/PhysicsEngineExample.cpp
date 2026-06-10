/**
 * @file PhysicsEngineExample.cpp
 * @brief Practical example of integrating PhysicsEngine into RayTracer
 * 
 * This file demonstrates how to:
 * 1. Initialize the physics engine
 * 2. Access it throughout the application
 * 3. Step physics each frame
 * 4. Access physics worlds
 * 
 * Integration points for RayTracer are marked with "// TODO: INTEGRATE"
 */

#include "Engine/Physics/PhysicsEngine.hpp"
#include "Engine/Physics/PhysicsWorld.hpp"
#include <iostream>

namespace Anito::Physics {

	// ============================================================================
	// Example: Initialize physics engine at application startup
	// ============================================================================

	/**
	 * INTEGRATION POINT 1: Add this to RayTracer::OnDeviceSet()
	 * 
	 * This initializes the physics engine after the Vulkan device is set up.
	 * Should be called before any physics worlds or bodies are created.
	 */
	void InitializePhysicsEngineExample() {
		// TODO: INTEGRATE - Add to RayTracer::OnDeviceSet()

		// Get the singleton instance (creates it if needed)
		auto& physicsEngine = PhysicsEngine::Get();

		// Configure before initialization (optional)
		physicsEngine.SetThreadCount(4);  // Use 4 threads for physics
		physicsEngine.SetTempMemorySize(20 * 1024 * 1024);  // 20MB temp buffer

		// Initialize the engine
		if (!physicsEngine.Initialize()) {
			std::cerr << "FATAL: Failed to initialize physics engine!" << std::endl;
			// Handle error appropriately
			return;
		}

		std::cout << "Physics engine initialized successfully!" << std::endl;
	}

	// ============================================================================
	// Example: Step physics each frame
	// ============================================================================

	/**
	 * INTEGRATION POINT 2: Add this to RayTracer::Render() or RayTracer::DrawFrame()
	 * 
	 * Step the physics simulation for the current frame.
	 * Should be called BEFORE rendering but AFTER updating camera/input.
	 */
	void StepPhysicsEngineExample(float deltaTime) {
		// TODO: INTEGRATE - Add to RayTracer::Render() or RayTracer::DrawFrame()

		// Get singleton instance
		auto& physicsEngine = PhysicsEngine::Get();

		// Check if physics is enabled and initialized
		if (!physicsEngine.IsInitialized() || !physicsEngine.IsEnabled()) {
			return;
		}

		// Step all physics worlds
		physicsEngine.StepAllWorlds(deltaTime);

		// Optionally get statistics for profiling
		if (false) {  // Set to true to enable profiling output
			uint32_t totalBodies = physicsEngine.GetTotalBodyCount();
			std::cout << "Physics: " << totalBodies << " bodies simulated" << std::endl;
		}
	}

	// ============================================================================
	// Example: Access physics world from anywhere
	// ============================================================================

	/**
	 * Get the default physics world to create bodies or run queries.
	 * Can be called from GameObject, PhysicsComponent, or UI systems.
	 */
	PhysicsWorldPtr GetDefaultPhysicsWorld() {
		// TODO: INTEGRATE - Use this accessor pattern throughout the codebase

		auto& physicsEngine = PhysicsEngine::Get();
		return physicsEngine.GetDefaultWorld();
	}

	// ============================================================================
	// Example: Create custom physics world
	// ============================================================================

	/**
	 * Create a separate physics world for specific scenarios.
	 * For example, ragdoll physics separate from main world.
	 */
	PhysicsWorldPtr CreateCustomPhysicsWorld() {
		// TODO: INTEGRATE - Call this when you need a specialized world

		auto& physicsEngine = PhysicsEngine::Get();

		PhysicsWorldSettings customSettings;
		customSettings.gravity = glm::vec3(0.0f, -15.0f, 0.0f);  // Stronger gravity
		customSettings.maxBodies = 5000;
		customSettings.enableSleeping = true;

		auto world = physicsEngine.CreateWorld(customSettings);

		if (!world) {
			std::cerr << "Failed to create custom physics world!" << std::endl;
			return nullptr;
		}

		std::cout << "Custom physics world created!" << std::endl;
		return world;
	}

	// ============================================================================
	// Example: Enable/disable physics globally
	// ============================================================================

	/**
	 * Pause/resume physics simulation globally.
	 * Useful for pause menu or debugging.
	 */
	void SetPhysicsEngineEnabled(bool enabled) {
		// TODO: INTEGRATE - Call from pause menu or hotkey handler

		auto& physicsEngine = PhysicsEngine::Get();
		physicsEngine.SetEnabled(enabled);

		std::cout << "Physics engine " << (enabled ? "enabled" : "disabled") << std::endl;
	}

	// ============================================================================
	// Example: Debug/profiling queries
	// ============================================================================

	/**
	 * Print engine statistics for debugging and profiling.
	 * Useful for performance monitoring.
	 */
	void PrintPhysicsEngineStatistics() {
		// TODO: INTEGRATE - Call from UI or debug console

		auto& physicsEngine = PhysicsEngine::Get();
		physicsEngine.PrintStatistics();
	}

	/**
	 * Get information about a specific world.
	 */
	void PrintPhysicsWorldStatistics(size_t worldIndex) {
		// TODO: INTEGRATE - For detailed world analysis

		auto& physicsEngine = PhysicsEngine::Get();
		auto world = physicsEngine.GetWorld(worldIndex);

		if (!world) {
			std::cerr << "World at index " << worldIndex << " not found!" << std::endl;
			return;
		}

		std::cout << "\n========== Physics World #" << worldIndex << " ==========" << std::endl;
		std::cout << "Body Count: " << world->GetBodyCount() << std::endl;
		std::cout << "Max Bodies: " << world->GetSettings().maxBodies << std::endl;
		std::cout << "Gravity: (" << world->GetGravity().x << ", "
				  << world->GetGravity().y << ", "
				  << world->GetGravity().z << ")" << std::endl;
		std::cout << "Time Step: " << world->GetTimeStep() << " seconds" << std::endl;
		std::cout << "Velocity Iterations: " << world->GetSettings().velocityIterations << std::endl;
		std::cout << "Position Iterations: " << world->GetSettings().positionIterations << std::endl;
		std::cout << "====================================\n" << std::endl;
	}

	// ============================================================================
	// Example: Shutdown physics engine
	// ============================================================================

	/**
	 * Manual shutdown of the physics engine.
	 * Normally handled automatically, but can be called explicitly if needed.
	 */
	void ShutdownPhysicsEngineExample() {
		// TODO: INTEGRATE - Optionally call from RayTracer destructor if explicit cleanup needed

		auto& physicsEngine = PhysicsEngine::Get();
		physicsEngine.Shutdown();

		std::cout << "Physics engine shut down!" << std::endl;
	}

} // namespace Anito::Physics

// ============================================================================
// Integration Checklist
// ============================================================================

/*
 * To integrate physics engine into RayTracer:
 * 
 * 1. HEADER FILES
 *    - Add #include "Engine/Physics/PhysicsEngine.hpp" to RayTracer.hpp
 * 
 * 2. INITIALIZATION
 *    - Call InitializePhysicsEngineExample() in RayTracer::OnDeviceSet()
 *    - After this point, can create physics worlds and bodies
 * 
 * 3. FRAME LOOP
 *    - Call StepPhysicsEngineExample(deltaTime) in RayTracer::DrawFrame()
 *    - After camera update, before rendering
 * 
 * 4. WORLD ACCESS
 *    - Use GetDefaultPhysicsWorld() to access physics world
 *    - Pass world reference to PhysicsComponent and body creators
 * 
 * 5. HOTKEYS/DEBUGGING
 *    - Add hotkey to toggle physics: SetPhysicsEngineEnabled()
 *    - Add hotkey for statistics: PrintPhysicsEngineStatistics()
 * 
 * 6. CLEANUP
 *    - Engine auto-cleans on program exit
 *    - Optionally call ShutdownPhysicsEngineExample() for explicit cleanup
 * 
 * EXAMPLE MINIMAL INTEGRATION:
 * 
 * // In RayTracer.hpp - add member variable:
 * private:
 *     Anito::Physics::PhysicsWorldPtr mDefaultPhysicsWorld;
 * 
 * // In RayTracer::OnDeviceSet():
 * InitializePhysicsEngineExample();
 * mDefaultPhysicsWorld = GetDefaultPhysicsWorld();
 * 
 * // In RayTracer::DrawFrame():
 * const auto deltaTime = time_ - prevTime;
 * StepPhysicsEngineExample(deltaTime);
 * 
 * // Then use mDefaultPhysicsWorld to create bodies, run queries, etc.
 */
