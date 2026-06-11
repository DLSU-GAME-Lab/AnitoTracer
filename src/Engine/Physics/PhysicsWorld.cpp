#include "PhysicsWorld.hpp"
#include "PhysicsContactListener.hpp"
#include "PhysicsBody.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Shapes/SphereShape.h>
#include <Jolt/Physics/Shapes/BoxShape.h>
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
		if (!mPhysicsSystem) {
			std::cerr << "[PhysicsWorld] Cannot create body - physics system not initialized" << std::endl;
			return nullptr;
		}

		try {
			// Create sphere shape (1 meter radius for now)
			// TODO: Support different shapes and sizes based on settings
			JPH::RefConst<JPH::Shape> shape = new JPH::SphereShape(0.5f);

			// Convert GLM types to Jolt types
			JPH::Vec3 joltPos(position.x, position.y, position.z);
			JPH::Quat joltRot(rotation.x, rotation.y, rotation.z, rotation.w);

			// Map Anito body type to Jolt
			JPH::EMotionType motionType = JPH::EMotionType::Dynamic;
			switch (settings.type) {
				case BodyType::STATIC:
					motionType = JPH::EMotionType::Static;
					break;
				case BodyType::DYNAMIC:
					motionType = JPH::EMotionType::Dynamic;
					break;
				case BodyType::KINEMATIC:
					motionType = JPH::EMotionType::Kinematic;
					break;
			}

			// Create body settings with the sphere shape
			JPH::BodyCreationSettings bodySettings(
				shape,
				joltPos,
				joltRot,
				motionType,
				static_cast<JPH::ObjectLayer>(settings.layer)
			);

			// Apply physics properties from our settings
			bodySettings.mMassPropertiesOverride.mMass = settings.mass;
			bodySettings.mLinearDamping = settings.material.linearDamping;
			bodySettings.mAngularDamping = settings.material.angularDamping;
			bodySettings.mGravityFactor = settings.useGravity ? 1.0f : 0.0f;
			bodySettings.mFriction = settings.material.friction;
			bodySettings.mRestitution = settings.material.restitution;

			// Create the actual Jolt body via the physics system
			JPH::BodyInterface& bodyInterface = mPhysicsSystem->GetBodyInterface();
			JPH::Body* joltBody = bodyInterface.CreateBody(bodySettings);

			if (!joltBody) {
				std::cerr << "[PhysicsWorld] Failed to create Jolt sphere body" << std::endl;
				return nullptr;
			}

			JPH::BodyID joltBodyId = joltBody->GetID();

			// Add the body to the physics system so it simulates
			bodyInterface.AddBody(joltBodyId, JPH::EActivation::Activate);

			// Generate unique Anito body ID
			uint32_t bodyId = static_cast<uint32_t>(mBodies.size());

			// Create our wrapper with references to the Jolt body
			auto body = std::make_shared<PhysicsBody>(
				bodyId, position, rotation, settings, 
				this,  // Pass this PhysicsWorld pointer
				joltBodyId  // Pass the Jolt BodyID
			);

			if (!body) {
				std::cerr << "[PhysicsWorld] Failed to allocate PhysicsBody wrapper" << std::endl;
				bodyInterface.DestroyBody(joltBodyId);
				return nullptr;
			}

			// Store the body and mapping
			mBodies[bodyId] = body;
			mBodyIdMapping[bodyId] = joltBodyId;

			std::cout << "[PhysicsWorld] Created sphere physics body" << std::endl;
			std::cout << "  - Anito ID: " << bodyId << std::endl;
			std::cout << "  - Jolt ID: " << joltBodyId.GetIndex() << std::endl;
			std::cout << "  - Position: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
			std::cout << "  - Mass: " << settings.mass << " kg" << std::endl;

			return body;
		}
		catch (const std::exception& e) {
			std::cerr << "[PhysicsWorld::CreateBody] Exception: " << e.what() << std::endl;
			return nullptr;
		}
	}

	void PhysicsWorld::DestroyBody(const PhysicsBodyPtr& body) {
		if (!body) {
			return;
		}

		try {
			uint32_t bodyId = body->GetBodyId();

			// Find and remove from Jolt
			auto it = mBodyIdMapping.find(bodyId);
			if (it != mBodyIdMapping.end()) {
				JPH::BodyID joltBodyId = it->second;
				if (mPhysicsSystem) {
					mPhysicsSystem->GetBodyInterface().DestroyBody(joltBodyId);
					std::cout << "[PhysicsWorld] Destroyed Jolt body ID: " << joltBodyId.GetIndex() << std::endl;
				}
				mBodyIdMapping.erase(it);
			}

			// Remove from our collection
			mBodies.erase(bodyId);
			std::cout << "[PhysicsWorld] Destroyed physics body ID: " << bodyId << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "[PhysicsWorld::DestroyBody] Exception: " << e.what() << std::endl;
		}
	}

	void PhysicsWorld::Clear() {
		try {
			if (mPhysicsSystem) {
				// Destroy all Jolt bodies
				for (const auto& [bodyId, joltBodyId] : mBodyIdMapping) {
					mPhysicsSystem->GetBodyInterface().DestroyBody(joltBodyId);
				}
			}
			mBodies.clear();
			mBodyIdMapping.clear();
			std::cout << "[PhysicsWorld] Cleared all physics bodies" << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "[PhysicsWorld::Clear] Exception: " << e.what() << std::endl;
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
			try {
				// TODO: Implement actual Jolt physics stepping
				// This requires proper understanding of the Jolt API and temp allocator setup
				// For now, just accumulate time
				// mPhysicsSystem->StepPhysics(...);

				mAccumulatedTime -= mSettings.timeStep;
			}
			catch (const std::exception& e) {
				std::cerr << "[PhysicsWorld::StepSimulation] Exception during physics step: " << e.what() << std::endl;
				break;
			}
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

	JPH::Body* PhysicsWorld::GetJoltBody(JPH::BodyID bodyId) const {
		if (!mPhysicsSystem || !bodyId.IsValid()) {
			return nullptr;
		}

		try {
			return mPhysicsSystem->GetBodyLockInterface().TryGetBody(bodyId);
		}
		catch (const std::exception& e) {
			std::cerr << "[PhysicsWorld::GetJoltBody] Exception: " << e.what() << std::endl;
			return nullptr;
		}
	}

} // namespace Anito::Physics


