#include "PhysicsEngine.hpp"
#include "PhysicsWorld.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/Factory.h>
#include <iostream>

namespace Anito::Physics {

	// ============================================================================
	// PhysicsEngine Singleton Implementation
	// ============================================================================

	PhysicsEngine* g_PhysicsEngineInstance = nullptr;

	PhysicsEngine& PhysicsEngine::Get() {
		if (g_PhysicsEngineInstance == nullptr) {
			g_PhysicsEngineInstance = new PhysicsEngine();
		}
		return *g_PhysicsEngineInstance;
	}

	PhysicsEngine::PhysicsEngine()
		: mInitialized(false),
		  mEnabled(true),
		  mThreadCount(4),
		  mTempMemorySize(10 * 1024 * 1024),  // 10MB
		  mDefaultWorld(nullptr)
	{
		std::cout << "[PhysicsEngine] Constructed (not yet initialized)" << std::endl;
	}

	PhysicsEngine::~PhysicsEngine() {
		Shutdown();
		std::cout << "[PhysicsEngine] Destroyed" << std::endl;
	}

	bool PhysicsEngine::Initialize() {
		if (mInitialized) {
			std::cout << "[PhysicsEngine] Already initialized, skipping" << std::endl;
			return true;
		}

		try {
			// First, we must register types with Jolt before using any physics functionality
			JPH::RegisterTypes();
			std::cout << "[PhysicsEngine] Jolt types registered" << std::endl;

			std::cout << "[PhysicsEngine] Initialized Jolt Physics Library" << std::endl;
			std::cout << "[PhysicsEngine]   - Thread count: " << mThreadCount << std::endl;
			std::cout << "[PhysicsEngine]   - Temp memory: " << (mTempMemorySize / (1024 * 1024)) << " MB" << std::endl;

			// Create default physics world
			PhysicsWorldSettings defaultSettings;
			mDefaultWorld = PhysicsWorld::Create(defaultSettings);

			if (!mDefaultWorld) {
				std::cerr << "[PhysicsEngine] Failed to create default world" << std::endl;
				return false;
			}

			mWorlds.push_back(mDefaultWorld);

			std::cout << "[PhysicsEngine] Default physics world created" << std::endl;

			mInitialized = true;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[PhysicsEngine] Initialization failed: " << e.what() << std::endl;
			return false;
		}
	}

	void PhysicsEngine::Shutdown() {
		if (!mInitialized) {
			return;
		}

		try {
			std::cout << "[PhysicsEngine] Shutting down..." << std::endl;

			// Destroy all worlds
			mWorlds.clear();
			mDefaultWorld.reset();

			std::cout << "[PhysicsEngine] All worlds destroyed" << std::endl;

			// Unregister types
			JPH::UnregisterTypes();
			std::cout << "[PhysicsEngine] Jolt types unregistered" << std::endl;

			mInitialized = false;
			std::cout << "[PhysicsEngine] Shutdown complete" << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "[PhysicsEngine] Error during shutdown: " << e.what() << std::endl;
		}
	}

	PhysicsWorldPtr PhysicsEngine::CreateWorld(const PhysicsWorldSettings& settings) {
		if (!mInitialized) {
			std::cerr << "[PhysicsEngine] Cannot create world - engine not initialized" << std::endl;
			return nullptr;
		}

		auto world = PhysicsWorld::Create(settings);
		if (world) {
			mWorlds.push_back(world);
			std::cout << "[PhysicsEngine] Created physics world (total: " << mWorlds.size() << ")" << std::endl;
		}

		return world;
	}

	void PhysicsEngine::DestroyWorld(const PhysicsWorldPtr& world) {
		if (!world) {
			return;
		}

		auto it = std::find(mWorlds.begin(), mWorlds.end(), world);
		if (it != mWorlds.end()) {
			mWorlds.erase(it);

			if (world == mDefaultWorld) {
				std::cout << "[PhysicsEngine] WARNING: Destroying default world!" << std::endl;
				if (!mWorlds.empty()) {
					mDefaultWorld = mWorlds[0];
				} else {
					mDefaultWorld = nullptr;
				}
			}

			std::cout << "[PhysicsEngine] World destroyed (remaining: " << mWorlds.size() << ")" << std::endl;
		}
	}

	PhysicsWorldPtr PhysicsEngine::GetWorld(size_t index) const {
		if (index < mWorlds.size()) {
			return mWorlds[index];
		}
		return nullptr;
	}

	std::vector<PhysicsWorldPtr> PhysicsEngine::GetAllWorlds() const {
		return mWorlds;
	}

	void PhysicsEngine::StepAllWorlds(float deltaTime) {
		if (!mInitialized || !mEnabled || deltaTime <= 0.0f) {
			return;
		}

		for (auto& world : mWorlds) {
			if (world) {
				world->StepSimulation(deltaTime);
			}
		}
	}

	void PhysicsEngine::StepWorld(const PhysicsWorldPtr& world, float deltaTime) {
		if (!mInitialized || !mEnabled || !world || deltaTime <= 0.0f) {
			return;
		}

		world->StepSimulation(deltaTime);
	}

	void PhysicsEngine::SetThreadCount(uint32_t threadCount) {
		if (mInitialized) {
			std::cerr << "[PhysicsEngine] Cannot change thread count after initialization" << std::endl;
			return;
		}

		mThreadCount = (threadCount > 0) ? threadCount : 1;
		std::cout << "[PhysicsEngine] Thread count set to: " << mThreadCount << std::endl;
	}

	void PhysicsEngine::SetTempMemorySize(uint32_t sizeInBytes) {
		if (mInitialized) {
			std::cerr << "[PhysicsEngine] Cannot change temp memory size after initialization" << std::endl;
			return;
		}

		mTempMemorySize = (sizeInBytes > 1024) ? sizeInBytes : 1024;  // Minimum 1KB
		std::cout << "[PhysicsEngine] Temp memory size set to: " << (mTempMemorySize / (1024 * 1024)) << " MB" << std::endl;
	}

	void PhysicsEngine::SetDebugVisualization(bool enabled) {
		for (auto& world : mWorlds) {
			if (world) {
				world->SetDebugDrawing(enabled);
			}
		}
	}

	uint32_t PhysicsEngine::GetTotalBodyCount() const {
		uint32_t totalBodies = 0;
		for (const auto& world : mWorlds) {
			if (world) {
				totalBodies += world->GetBodyCount();
			}
		}
		return totalBodies;
	}

	void PhysicsEngine::PrintStatistics() const {
		std::cout << "\n========== Physics Engine Statistics ==========" << std::endl;
		std::cout << "Initialized: " << (mInitialized ? "Yes" : "No") << std::endl;
		std::cout << "Enabled: " << (mEnabled ? "Yes" : "No") << std::endl;
		std::cout << "Thread Count: " << mThreadCount << std::endl;
		std::cout << "Temp Memory Size: " << (mTempMemorySize / (1024 * 1024)) << " MB" << std::endl;
		std::cout << "Total Worlds: " << mWorlds.size() << std::endl;
		std::cout << "Total Bodies: " << GetTotalBodyCount() << std::endl;

		for (size_t i = 0; i < mWorlds.size(); ++i) {
			if (mWorlds[i]) {
				std::cout << "\n  World " << i << ":" << std::endl;
				std::cout << "    - Bodies: " << mWorlds[i]->GetBodyCount() << std::endl;
				std::cout << "    - Gravity: "
						  << mWorlds[i]->GetGravity().x << ", "
						  << mWorlds[i]->GetGravity().y << ", "
						  << mWorlds[i]->GetGravity().z << std::endl;
				std::cout << "    - Time Step: " << mWorlds[i]->GetTimeStep() << "s" << std::endl;
			}
		}

		std::cout << "=============================================\n" << std::endl;
	}

} // namespace Anito::Physics

