#include "PhysicsWorld.hpp"
#include "PhysicsContactListener.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <iostream>

namespace Anito::Physics {

	// ============================================================================
	// BroadPhaseLayerInterface Implementation
	// ============================================================================

	class BroadPhaseLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface {
	public:
		BroadPhaseLayerInterfaceImpl() {
			mObjectToBroadPhase[static_cast<int>(ObjectLayer::STATIC)] = static_cast<int>(BroadPhaseLayer::STATIC);
			mObjectToBroadPhase[static_cast<int>(ObjectLayer::DYNAMIC)] = static_cast<int>(BroadPhaseLayer::DYNAMIC);
			mObjectToBroadPhase[static_cast<int>(ObjectLayer::TRIGGER)] = static_cast<int>(BroadPhaseLayer::DYNAMIC);
			mObjectToBroadPhase[static_cast<int>(ObjectLayer::KINEMATIC)] = static_cast<int>(BroadPhaseLayer::DYNAMIC);
			mObjectToBroadPhase[static_cast<int>(ObjectLayer::RAGDOLL)] = static_cast<int>(BroadPhaseLayer::DYNAMIC);
		}

		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
			return JPH::BroadPhaseLayer(mObjectToBroadPhase[inLayer]);
		}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
			switch (inLayer) {
			case 0: return "STATIC";
			case 1: return "DYNAMIC";
			default: return "UNKNOWN";
			}
		}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

	private:
		uint8_t mObjectToBroadPhase[static_cast<int>(ObjectLayer::NUM_LAYERS)];
	};

	// ============================================================================
	// ObjectLayerPairFilter Implementation
	// ============================================================================

	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
			// Configure which layers should collide with each other
			ObjectLayer layer1 = static_cast<ObjectLayer>(inObject1);
			ObjectLayer layer2 = static_cast<ObjectLayer>(inObject2);

			// Static doesn't collide with static
			if (layer1 == ObjectLayer::STATIC && layer2 == ObjectLayer::STATIC) {
				return false;
			}

			// Everything else can collide
			return true;
		}
	};

	// ============================================================================
	// PhysicsWorld Implementation
	// ============================================================================

	PhysicsWorldPtr PhysicsWorld::Create(const PhysicsWorldSettings& settings) {
		auto world = std::shared_ptr<PhysicsWorld>(new PhysicsWorld(settings));
		if (!world->Initialize()) {
			return nullptr;
		}
		return world;
	}

	PhysicsWorld::PhysicsWorld(const PhysicsWorldSettings& settings)
		: mSettings(settings),
		  mPhysicsSystem(nullptr),
		  mDebugDrawingEnabled(false),
		  mAccumulatedTime(0.0f)
	{
	}

	PhysicsWorld::~PhysicsWorld() {
		// Cleanup is handled in the destruction of mPhysicsSystem unique_ptr
		if (mPhysicsSystem) {
			mPhysicsSystem.reset();
		}
	}

	bool PhysicsWorld::Initialize() {
		try {
			// Create the physics system
			// Note: RegisterTypes() must have been called by PhysicsEngine before this!
			mPhysicsSystem = std::make_unique<JPH::PhysicsSystem>();

			if (!mPhysicsSystem) {
				std::cerr << "[PhysicsWorld] Failed to allocate PhysicsSystem" << std::endl;
				return false;
			}

			// Set gravity
			mPhysicsSystem->SetGravity(JPH::Vec3(
				mSettings.gravity.x,
				mSettings.gravity.y,
				mSettings.gravity.z
			));

			std::cout << "[PhysicsWorld] Successfully initialized with "
						  << mSettings.maxBodies << " max bodies" << std::endl;

			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[PhysicsWorld] Initialization failed: " << e.what() << std::endl;
			return false;
		}
	}

	PhysicsBodyPtr PhysicsWorld::CreateBody(
		const glm::vec3& position,
		const glm::quat& rotation,
		const PhysicsBodySettings& settings)
	{
		// TODO: Implement body creation
		return nullptr;
	}

	void PhysicsWorld::DestroyBody(const PhysicsBodyPtr& body) {
		if (!body) {
			return;
		}

		// TODO: Implement body destruction
		mBodies.erase(body->GetBodyId());
	}

	void PhysicsWorld::Clear() {
		mBodies.clear();
		if (mPhysicsSystem) {
			// TODO: Clear all bodies from the Jolt system
		}
	}

	PhysicsBodyPtr PhysicsWorld::GetBody(uint32_t bodyId) const {
		auto it = mBodies.find(bodyId);
		if (it != mBodies.end()) {
			return it->second;
		}
		return nullptr;
	}

	std::vector<PhysicsBodyPtr> PhysicsWorld::GetAllBodies() const {
		std::vector<PhysicsBodyPtr> bodies;
		bodies.reserve(mBodies.size());
		for (const auto& pair : mBodies) {
			bodies.push_back(pair.second);
		}
		return bodies;
	}

	uint32_t PhysicsWorld::GetBodyCount() const {
		return static_cast<uint32_t>(mBodies.size());
	}

	void PhysicsWorld::StepSimulation(float deltaTime) {
		if (!mPhysicsSystem || !mSettings.timeStep || deltaTime <= 0.0f) {
			return;
		}

		mAccumulatedTime += deltaTime;

		// Fixed time stepping
		while (mAccumulatedTime >= mSettings.timeStep) {
			// TODO: Perform physics step with Jolt
			mAccumulatedTime -= mSettings.timeStep;
		}
	}

	void PhysicsWorld::SetGravity(const glm::vec3& gravity) {
		mSettings.gravity = gravity;
		if (mPhysicsSystem) {
			mPhysicsSystem->SetGravity(JPH::Vec3(gravity.x, gravity.y, gravity.z));
		}
	}

	glm::vec3 PhysicsWorld::GetGravity() const {
		return mSettings.gravity;
	}

	void PhysicsWorld::SetTimeStep(float timeStep) {
		if (timeStep > 0.0f) {
			mSettings.timeStep = timeStep;
		}
	}

	bool PhysicsWorld::RayCast(
		const glm::vec3& from,
		const glm::vec3& to,
		uint32_t& outBodyId,
		glm::vec3& outPosition,
		glm::vec3& outNormal,
		float& outDistance) const
	{
		// TODO: Implement raycasting
		return false;
	}

	std::vector<PhysicsBodyPtr> PhysicsWorld::GetBodiesInAABB(
		const glm::vec3& min,
		const glm::vec3& max) const
	{
		// TODO: Implement AABB query
		return {};
	}

	std::vector<PhysicsBodyPtr> PhysicsWorld::GetBodiesInSphere(
		const glm::vec3& center,
		float radius) const
	{
		// TODO: Implement sphere query
		return {};
	}

	void PhysicsWorld::SetCollisionCallback(std::function<void(const CollisionEvent&)> callback) {
		mCollisionCallbacks.push_back(callback);
	}

	void PhysicsWorld::ClearCollisionCallbacks() {
		mCollisionCallbacks.clear();
	}

	void PhysicsWorld::SetSettings(const PhysicsWorldSettings& settings) {
		mSettings = settings;

		if (mPhysicsSystem) {
			// Update gravity
			mPhysicsSystem->SetGravity(JPH::Vec3(
				mSettings.gravity.x,
				mSettings.gravity.y,
				mSettings.gravity.z
			));
		}
	}

} // namespace Anito::Physics

