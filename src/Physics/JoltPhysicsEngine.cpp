#include "JoltPhysicsEngine.hpp"
#include "JoltPhysicsBody.hpp"
#include "JoltContactListener.hpp"
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <iostream>

// Constants for Jolt initialization
static constexpr uint32_t cMaxBodies = 65536;
static constexpr uint32_t cNumBodyMutexes = 0;
static constexpr uint32_t cMaxBodyPairs = 65536;
static constexpr uint32_t cMaxContactConstraints = 10240;
static constexpr uint32_t cNumThreads = 4;

// Broadphase layer setup
namespace {
	static constexpr uint8_t DEFAULT_LAYER = 0;
	static constexpr uint8_t DEFAULT_BROAD_LAYER = 0;
}

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
	bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override {
		return true; // All layers collide with each other
		// return inLayer1 == DEFAULT_LAYER && inLayer2 == DEFAULT_LAYER;
	}
};

class BroadPhaseLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface {
public:
	JPH::uint GetNumBroadPhaseLayers() const override {
		return 1; // Only one broadphase layer
	}

	JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
		return JPH::BroadPhaseLayer(DEFAULT_BROAD_LAYER); // All object layers map to the default broadphase layer
	}
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
	bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
		return true; // All object layers collide with all broadphase layers
	}
};

JoltPhysicsEngine::JoltPhysicsEngine() : mGravity(0.0f, -9.81f, 0.0f) {
	// Initialize Jolt
	JPH::Factory::sInstance = new JPH::Factory();
	JPH::RegisterTypes();

	// Create job system
	mJobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, cNumThreads);

	static BroadPhaseLayerInterfaceImpl broadPhaseLayer;
	static ObjectLayerPairFilterImpl objectLayerPairFilter;
	static ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;

	// Create the physics system
	mPhysicsSystem = std::make_unique<JPH::PhysicsSystem>();
	mPhysicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broadPhaseLayer, objectVsBroadPhaseLayerFilter, objectLayerPairFilter);

	// Instantiate and register listener
	mContactListener = std::make_unique<JoltContactListener>(*this);
	mPhysicsSystem->SetContactListener(mContactListener.get());

	mPhysicsSystem->SetGravity(JPH::Vec3(mGravity.x, mGravity.y, mGravity.z));
}

JoltPhysicsEngine::~JoltPhysicsEngine() {
	mBodies.clear();
	mPhysicsSystem.reset();
	mJobSystem.reset();
	delete JPH::Factory::sInstance;
	JPH::Factory::sInstance = nullptr;
}

void JoltPhysicsEngine::SetGravity(const glm::vec3& gravity) {
	mGravity = gravity;
	if (mPhysicsSystem) {
		mPhysicsSystem->SetGravity(JPH::Vec3(gravity.x, gravity.y, gravity.z));
	}
}

glm::vec3 JoltPhysicsEngine::GetGravity() const {
	return mGravity;
}

std::shared_ptr<IPhysicsBody> JoltPhysicsEngine::CreateRigidBody(
	const glm::vec3& position, 
	const glm::quat& rotation, 
	float mass, 
	ShapeType shape, 
	const ShapeParams& shapeParams) {
	if (!mPhysicsSystem)
	{
		std::cerr << "[JoltPhysicsEngine] Error: Physics system is not initialized.\n";
		return nullptr;
	}

	// Build requested shape
	JPH::ShapeSettings::ShapeResult shapeResult;
	switch (shape) {
	case ShapeType::Box: {
		JPH::BoxShapeSettings boxSettings(JPH::Vec3(shapeParams.v.x * 0.5f, shapeParams.v.y * 0.5f, shapeParams.v.z * 0.5f));
		shapeResult = boxSettings.Create();
		break;
	}
	case ShapeType::Sphere: {
		float radius = shapeParams.v.x;
		if (radius <= 0.0f) {
			std::cerr << "[JoltPhysicsEngine] Error: Sphere radius must be greater than zero.\n";
			return nullptr;
		}
		JPH::SphereShapeSettings sphereSettings(radius);
		shapeResult = sphereSettings.Create();
		break;
	}
	case ShapeType::Capsule: {
		float radius = shapeParams.v.x;
		float halfHeight = shapeParams.v.y;
		if (radius <= 0.0f || halfHeight <= 0.0f) {
			std::cerr << "[JoltPhysicsEngine] Error: Capsule radius and half-height must be greater than zero.\n";
			return nullptr;
		}
		JPH::CapsuleShapeSettings capsuleSettings(radius, halfHeight);
		shapeResult = capsuleSettings.Create();
		break;
	}
		default:
			std::cerr << "[JoltPhysicsEngine] Error: Unknown shape type.\n";
			return nullptr;
	}

	if (!shapeResult.IsValid()) {
		std::cerr << "[JoltPhysicsEngine] Error: Failed to create shape.\n";
		return nullptr;
	}

	// Create body settings
	JPH::BodyCreationSettings bodySettings(
		shapeResult.Get(),
		JPH::RVec3(position.x, position.y, position.z),
		JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
		mass > 0.0f ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static,
		DEFAULT_LAYER
	);

	// Create body
	JPH::Body* body = mPhysicsSystem->GetBodyInterface().CreateBody(bodySettings);
	JPH::BodyID bodyID = body->GetID();

	// Add body
	mPhysicsSystem->GetBodyInterface().AddBody(bodyID, JPH::EActivation::Activate);

	// Create wrapper
	auto physicsBody = std::make_shared<JoltPhysicsBody>(bodyID, &mPhysicsSystem->GetBodyInterface(), mass);
	mBodies[bodyID] = physicsBody; 

	return physicsBody;
}

void JoltPhysicsEngine::DestroyRigidBody(std::shared_ptr<IPhysicsBody> body) {
	if (!body || !mPhysicsSystem) {
		return;
	}

	// Cast to JoltPhysicsBody
	auto joltBody = std::dynamic_pointer_cast<JoltPhysicsBody>(body);
	if (!joltBody) {
		return;
	}

	JPH::BodyID bodyID = joltBody->GetBodyID();

	// Remove the body from the physics system
	mPhysicsSystem->GetBodyInterface().RemoveBody(bodyID);
	mPhysicsSystem->GetBodyInterface().DestroyBody(bodyID);

	// Remove from tracking
	mBodies.erase(bodyID);
}

void JoltPhysicsEngine::SetCollisionCallback(CollisionCallback callback) {
	mCollisionCallback = callback;
}

bool JoltPhysicsEngine::HasCollisionCallback() const {
	return static_cast<bool>(mCollisionCallback);
}

void JoltPhysicsEngine::InvokeCollisionCallback(std::shared_ptr<IPhysicsBody> bodyA, std::shared_ptr<IPhysicsBody> bodyB, const glm::vec3& point) {
	if (mCollisionCallback) {
		mCollisionCallback(bodyA, bodyB, point);
	}
}

std::shared_ptr<JoltPhysicsBody> JoltPhysicsEngine::FindBodyByID(JPH::BodyID id) {
	auto it = mBodies.find(id);
	if (it != mBodies.end()) return it->second;
	return nullptr;
}

void JoltPhysicsEngine::Step(float deltaTime) {
	if (!mPhysicsSystem || !mJobSystem) {
		return;
	}

	JPH::TempAllocatorMalloc tempAllocator;

	// Timestep
	mPhysicsSystem->Update(deltaTime, 1, &tempAllocator, mJobSystem.get());
}

bool JoltPhysicsEngine::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, std::shared_ptr<IPhysicsBody>& outBody, glm::vec3& outHitPoint) {
	if (!mPhysicsSystem) {
		return false;
	}

	// Normalize direction just in case
	glm::vec3 dir = glm::normalize(direction);

	JPH::RVec3 rayOrigin(origin.x, origin.y, origin.z);
	JPH::RVec3 rayDirection(dir.x, dir.y, dir.z);
	JPH::RRayCast ray(rayOrigin, rayDirection);

	JPH::RayCastResult result;
	bool hit = mPhysicsSystem->GetNarrowPhaseQuery().CastRay(ray, result, JPH::BroadPhaseLayerFilter{}, JPH::ObjectLayerFilter{}, JPH::BodyFilter{});

	if (!hit) {
		return false;
	}

	outBody = FindBodyByID(result.mBodyID);
	if (!outBody) {
		// Jolt detects body but we don't have it in our map
		return false;
	}

	JPH::RVec3 hitPos = ray.GetPointOnRay(result.mFraction);
	outHitPoint = glm::vec3(hitPos.GetX(), hitPos.GetY(), hitPos.GetZ());

	return true;
}