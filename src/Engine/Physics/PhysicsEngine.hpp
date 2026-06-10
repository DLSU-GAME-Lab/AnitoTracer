#pragma once

#include "PhysicsDefines.hpp"
#include "PhysicsWorld.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace Anito::Physics {

	/**
	 * @brief Global and singleton physics engine manager
	 * 
	 * The PhysicsEngine manages the global physics system, including:
	 * - Initialization of Jolt Physics library
	 * - Creation and management of multiple physics worlds
	 * - Global physics settings and callbacks
	 * - Thread pool and memory management
	 * 
	 * Access via PhysicsEngine::Get()
	 */
	class PhysicsEngine {
	public:
		/**
		 * @brief Get the singleton instance
		 */
		static PhysicsEngine& Get();

		/**
		 * @brief Initialize the physics engine
		 * @return true if initialization was successful
		 */
		bool Initialize();

		/**
		 * @brief Shutdown the physics engine
		 */
		void Shutdown();

		/**
		 * @brief Check if engine is initialized
		 */
		bool IsInitialized() const { return mInitialized; }

		// --- World Management ---

		/**
		 * @brief Create a new physics world
		 * @param settings Configuration for the new world
		 * @return Shared pointer to the created world
		 */
		PhysicsWorldPtr CreateWorld(const PhysicsWorldSettings& settings = PhysicsWorldSettings());

		/**
		 * @brief Destroy a physics world
		 */
		void DestroyWorld(const PhysicsWorldPtr& world);

		/**
		 * @brief Get the default/primary physics world
		 * (Created automatically on engine init)
		 */
		PhysicsWorldPtr GetDefaultWorld() const { return mDefaultWorld; }

		/**
		 * @brief Get a world by index
		 */
		PhysicsWorldPtr GetWorld(size_t index) const;

		/**
		 * @brief Get all created worlds
		 */
		std::vector<PhysicsWorldPtr> GetAllWorlds() const;

		/**
		 * @brief Get the number of worlds
		 */
		size_t GetWorldCount() const { return mWorlds.size(); }

		// --- Simulation ---

		/**
		 * @brief Step all physics worlds
		 * @param deltaTime Time step in seconds
		 */
		void StepAllWorlds(float deltaTime);

		/**
		 * @brief Step a specific world
		 */
		void StepWorld(const PhysicsWorldPtr& world, float deltaTime);

		// --- Configuration ---

		/**
		 * @brief Set the number of threads for physics simulation
		 * (Must be called before Initialize)
		 */
		void SetThreadCount(uint32_t threadCount);

		/**
		 * @brief Get the thread count
		 */
		uint32_t GetThreadCount() const { return mThreadCount; }

		/**
		 * @brief Set the temporary memory buffer size
		 * (Must be called before Initialize)
		 */
		void SetTempMemorySize(uint32_t sizeInBytes);

		/**
		 * @brief Enable or disable physics engine globally
		 */
		void SetEnabled(bool enabled) { mEnabled = enabled; }

		/**
		 * @brief Check if physics engine is enabled
		 */
		bool IsEnabled() const { return mEnabled; }

		// --- Debug ---

		/**
		 * @brief Enable debug visualization for all worlds
		 */
		void SetDebugVisualization(bool enabled);

		/**
		 * @brief Get the total number of bodies across all worlds
		 */
		uint32_t GetTotalBodyCount() const;

		/**
		 * @brief Print engine statistics to console
		 */
		void PrintStatistics() const;

	private:
		PhysicsEngine();
		~PhysicsEngine();

		// Prevent copying
		PhysicsEngine(const PhysicsEngine&) = delete;
		PhysicsEngine& operator=(const PhysicsEngine&) = delete;

		bool mInitialized = false;
		bool mEnabled = true;
		uint32_t mThreadCount = 4;
		uint32_t mTempMemorySize = 10 * 1024 * 1024;  // 10MB

		PhysicsWorldPtr mDefaultWorld;
		std::vector<PhysicsWorldPtr> mWorlds;

		friend class PhysicsWorld;
	};

} // namespace Anito::Physics
