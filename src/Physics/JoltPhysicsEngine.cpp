#include "JoltPhysicsEngine.hpp"
#include "JoltPhysicsBody.hpp"
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

// Constants for Jolt initialization
static constexpr uint32_t cMaxBodies = 65536;
static constexpr uint32_t cNumBodyMutexes = 0;
static constexpr uint32_t cMaxBodyPairs = 65536;
static constexpr uint32_t cMaxContactConstraints = 10240;
static constexpr uint32_t cNumThreads = 4;

JoltPhysicsEngine::JoltPhysicsEngine() : mGravity(0.0f, -9.81f, 0.0f) {
	// Initialize Jolt
	JPH::Factory::sInstance = new JPH::Factory();
	JPH::RegisterTypes();

	// Create job system
	mJobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, cNumThreads);


}