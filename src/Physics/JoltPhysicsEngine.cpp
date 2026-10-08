#include "JoltPhysicsEngine.hpp"
#include "JoltPhysicsBody.hpp"
#include "JoltDebugRenderer.hpp"
#include "JoltContactListener.hpp"
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseQuery.h>
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
	JPH::RegisterDefaultAllocator();
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

	// Debug visualization
	mDebugRenderer = std::make_unique<JoltDebugRenderer>();

	mPhysicsSystem->SetGravity(JPH::Vec3(mGravity.x, mGravity.y, mGravity.z));
}

JoltPhysicsEngine::~JoltPhysicsEngine() {
	mBodies.clear();
	mCollisionCallbacks.clear();
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

JPH::ShapeSettings::ShapeResult JoltPhysicsEngine::BuildShapeSettings(const ColliderShape& shape) {
	switch (shape.type) {
	case ShapeType::Box: {
		JPH::BoxShapeSettings boxSettings(JPH::Vec3(shape.params.v.x, shape.params.v.y, shape.params.v.z));
		return boxSettings.Create();
	}
	case ShapeType::Sphere: {
		float radius = shape.params.v.x;
		if (radius <= 0.0f) { std::cerr << "[JoltPhysicsEngine] Error: Sphere radius must be greater than zero.\n"; return JPH::ShapeSettings::ShapeResult(); }
		return JPH::SphereShapeSettings(radius).Create();
	}
	case ShapeType::Capsule: {
		float radius = shape.params.v.x;
		float halfHeight = shape.params.v.y;
		if (radius <= 0.0f || halfHeight <= 0.0f) { std::cerr << "[JoltPhysicsEngine] Error: Capsule radius and half-height must be greater than zero.\n"; return JPH::ShapeSettings::ShapeResult(); }
		return JPH::CapsuleShapeSettings(halfHeight, radius).Create();
	}
	case ShapeType::Mesh: {
		if (!shape.meshData || shape.meshData->vertices.empty() || shape.meshData->indices.size() < 3) {
			std::cerr << "[JoltPhysicsEngine] Error: Mesh collider has no geometry.\n";
			return JPH::ShapeSettings::ShapeResult();
		}

		JPH::VertexList vertexList;
		vertexList.reserve(shape.meshData->vertices.size());
		for (const glm::vec3& v : shape.meshData->vertices) {
			vertexList.push_back(JPH::Float3(v.x, v.y, -v.z));
		}

		JPH::IndexedTriangleList triList;
		const auto& idx = shape.meshData->indices;
		triList.reserve(idx.size() / 3);
		for (size_t i = 0; i + 2 < idx.size(); i += 3) {
			// Negating Z mirrors the mesh, which reverses triangle winding.
			// Swap two indices per triangle to keep normals facing outward.
			triList.push_back(JPH::IndexedTriangle(idx[i], idx[i + 2], idx[i + 1]));
		}

		JPH::MeshShapeSettings meshSettings(vertexList, triList);
		JPH::ShapeSettings::ShapeResult meshResult = meshSettings.Create();
		if (!meshResult.IsValid() || shape.scale == glm::vec3(1.0f)) {
			return meshResult;
		}

		// Mesh vertices aren't pre-scaled (unlike box/sphere/capsule params),
		// so wrap in a ScaledShape instead of rebuilding triangle data.
		JPH::ScaledShapeSettings scaled(meshResult.Get(), JPH::Vec3(shape.scale.x, shape.scale.y, shape.scale.z));
		return scaled.Create();
	}
	case ShapeType::ConvexHull: {
		if (!shape.meshData || shape.meshData->vertices.empty()) {
			std::cerr << "[JoltPhysicsEngine] Error: Convex hull collider has no geometry.\n";
			return JPH::ShapeSettings::ShapeResult();
		}

		JPH::Array<JPH::Vec3> points;
		const std::vector<glm::vec3>& src = shape.meshData->vertices;

		if (src.size() <= JPH::ConvexHullShape::cMaxPointsInHull) {
			points.reserve(src.size());
			for (const glm::vec3& v : src) {
				points.push_back(JPH::Vec3(v.x, v.y, -v.z)); // same LH->RH flip as elsewhere
			}
		}
		else {
			// Too many source vertices for Jolt's hull limit so we downsample
			// evenly rather than just truncating so the hull still
			// approximates the full mesh instead of one region of it.
			points.reserve(JPH::ConvexHullShape::cMaxPointsInHull);
			float stride = static_cast<float>(src.size()) / JPH::ConvexHullShape::cMaxPointsInHull;
			for (int i = 0; i < JPH::ConvexHullShape::cMaxPointsInHull; ++i) {
				const glm::vec3& v = src[static_cast<size_t>(i * stride)];
				points.push_back(JPH::Vec3(v.x, v.y, -v.z));
			}
		}

		JPH::ConvexHullShapeSettings hullSettings(points);
		JPH::ShapeSettings::ShapeResult hullResult = hullSettings.Create();
		if (!hullResult.IsValid() || shape.scale == glm::vec3(1.0f)) {
			return hullResult;
		}

		JPH::ScaledShapeSettings scaled(hullResult.Get(), JPH::Vec3(shape.scale.x, shape.scale.y, shape.scale.z));
		return scaled.Create();
	}
	default:
		std::cerr << "[JoltPhysicsEngine] Error: Unknown shape type.\n";
		return JPH::ShapeSettings::ShapeResult();
	}
}

JPH::RefConst<JPH::Shape> JoltPhysicsEngine::BuildCompoundShape(const std::vector<ColliderShape>& shapes) {
	if (shapes.empty()) {
		// Placeholder geometry for a body that has no Colliders registered yet
		JPH::BoxShapeSettings placeholder(JPH::Vec3(0.5f, 0.5f, 0.5f));
		auto result = placeholder.Create();
		return result.IsValid() ? result.Get() : nullptr;
	}

	if (shapes.size() == 1 && shapes[0].offset == glm::vec3(0.0f)) {
		auto result = BuildShapeSettings(shapes[0]);
		return result.IsValid() ? result.Get() : nullptr;
	}

	JPH::StaticCompoundShapeSettings compound;
	for (const ColliderShape& s : shapes) {
		auto sub = BuildShapeSettings(s);
		if (!sub.IsValid()) {
			std::cerr << "[JoltPhysicsEngine] Error: Failed to create sub-shape for compound collider.\n";
			continue;
		}
		compound.AddShape(JPH::Vec3(s.offset.x, s.offset.y, s.offset.z), JPH::Quat::sIdentity(), sub.Get());
	}

	auto result = compound.Create();
	if (!result.IsValid()) {
		std::cerr << "[JoltPhysicsEngine] Error: Failed to create compound shape.\n";
		return nullptr;
	}
	return result.Get();
}

std::shared_ptr<IPhysicsBody> JoltPhysicsEngine::CreateRigidBody(
	const glm::vec3& position, 
	const glm::quat& rotation, 
	float mass, 
	const std::vector<ColliderShape>& shapes,
	float restitution) {
	if (!mPhysicsSystem)
	{
		std::cerr << "[JoltPhysicsEngine] Error: Physics system is not initialized.\n";
		return nullptr;
	}
	std::cout << "[DEBUG] CreateRigidBody called for body-to-be, shapes.size()=" << shapes.size();
	// Enforce static-only for mesh colliders
	if (mass > 0.0f) {
		for (const auto& s : shapes) {
			if (s.type == ShapeType::Mesh) {
				std::cerr << "[JoltPhysicsEngine] Error: Mesh colliders are static-only; body must have mass <= 0.\n";
				return nullptr;
			}
		}
	}
	for (const auto& s : shapes) {
		std::cout << " [type=" << static_cast<int>(s.type)
			<< " params=(" << s.params.v.x << "," << s.params.v.y << "," << s.params.v.z << ")"
			<< " offset=(" << s.offset.x << "," << s.offset.y << "," << s.offset.z << ")]";
	}
	std::cout << std::endl;

	JPH::RefConst<JPH::Shape> shape = BuildCompoundShape(shapes);
	if (!shape) return nullptr;

	// Create body settings
	JPH::BodyCreationSettings bodySettings(
		shape,
		JPH::RVec3(position.x, position.y, position.z),
		JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
		mass > 0.0f ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static,
		DEFAULT_LAYER
	);
	bodySettings.mRestitution = restitution;

	// Create body
	JPH::Body* body = mPhysicsSystem->GetBodyInterface().CreateBody(bodySettings);
	JPH::BodyID bodyID = body->GetID();

	// Add body
	mPhysicsSystem->GetBodyInterface().AddBody(bodyID, JPH::EActivation::Activate);

	// Create wrapper
	auto physicsBody = std::make_shared<JoltPhysicsBody>(bodyID, &mPhysicsSystem->GetBodyInterface(), &mPhysicsSystem->GetBodyLockInterface(), mass);
	mBodies[bodyID] = physicsBody; 

	std::cout << "[DEBUG] CreateRigidBody called, shapes.size()=" << shapes.size() << std::endl;

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

	mStaticLineCache.erase(bodyID.GetIndexAndSequenceNumber());

	// Remove from tracking
	mBodies.erase(bodyID);
}

bool JoltPhysicsEngine::SetShapes(IPhysicsBody* body, const std::vector<ColliderShape>& shapes) {
	if (!body || !mPhysicsSystem) return false;

	JPH::BodyID bodyID = static_cast<JoltPhysicsBody*>(body)->GetBodyID();

	JPH::RefConst<JPH::Shape> shape = BuildCompoundShape(shapes);
	if (!shape) return false;

	mPhysicsSystem->GetBodyInterface().SetShape(bodyID, shape, true, JPH::EActivation::Activate);
	mStaticLineCache.erase(bodyID.GetIndexAndSequenceNumber());

	return true;
}

bool JoltPhysicsEngine::SetRestitution(IPhysicsBody* body, float restitution) {
	if (!body || !mPhysicsSystem) return false;
	JPH::BodyID bodyID = static_cast<JoltPhysicsBody*>(body)->GetBodyID();
	mPhysicsSystem->GetBodyInterface().SetRestitution(bodyID, restitution);
	return true;
}

bool JoltPhysicsEngine::SetFriction(IPhysicsBody* body, float friction) {
	if (!body || !mPhysicsSystem) return false;
	JPH::BodyID bodyID = static_cast<JoltPhysicsBody*>(body)->GetBodyID();
	mPhysicsSystem->GetBodyInterface().SetFriction(bodyID, friction);
	return true;
}

void JoltPhysicsEngine::RegisterCollisionCallback(IPhysicsBody* body, CollisionCallback callback) {
	if (!body) return;
	mCollisionCallbacks[body] = callback;
}

void JoltPhysicsEngine::UnregisterCollisionCallback(IPhysicsBody* body) {
	if (!body) return;
	mCollisionCallbacks.erase(body);
}

void JoltPhysicsEngine::InvokeCollisionCallbacks(std::shared_ptr<IPhysicsBody> bodyA, std::shared_ptr<IPhysicsBody> bodyB, const glm::vec3& point) {
	auto fireIfRegistered = [&](std::shared_ptr<IPhysicsBody> body, std::shared_ptr<IPhysicsBody> other) {
		auto it = mCollisionCallbacks.find(body.get());
		if (it != mCollisionCallbacks.end()) {
			it->second(body, other, point);
		}
	};

	fireIfRegistered(bodyA, bodyB);
	fireIfRegistered(bodyB, bodyA);
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

bool JoltPhysicsEngine::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, std::shared_ptr<IPhysicsBody>& outBody, glm::vec3& outHitPoint, const IPhysicsBody* ignoredBody) {
	if (!mPhysicsSystem) {
		return false;
	}

	// Normalize direction just in case
	glm::vec3 dir = glm::normalize(direction);

	JPH::RVec3 rayOrigin(origin.x, origin.y, origin.z);
	JPH::RVec3 rayDirection(dir.x, dir.y, dir.z);
	JPH::RRayCast ray(rayOrigin, rayDirection);

	JPH::RayCastResult result;
	class IgnoreBodyFilter final : public JPH::BodyFilter {
	public:
		explicit IgnoreBodyFilter(const IPhysicsBody* ignoredBody)
			: mIgnoredBody(dynamic_cast<const JoltPhysicsBody*>(ignoredBody)) {}

		bool ShouldCollide(const JPH::BodyID& bodyID) const override {
			return !mIgnoredBody || bodyID != mIgnoredBody->GetBodyID();
		}

	private:
		const JoltPhysicsBody* mIgnoredBody;
	};

	bool hit = mPhysicsSystem->GetNarrowPhaseQuery().CastRay(
		ray, result, JPH::BroadPhaseLayerFilter{}, JPH::ObjectLayerFilter{}, IgnoreBodyFilter(ignoredBody));

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

void JoltPhysicsEngine::WakeBodiesAroundBody(IPhysicsBody* body) {
	if (!body || !mPhysicsSystem) return;

	auto* joltBody = dynamic_cast<JoltPhysicsBody*>(body);
	if (!joltBody) return;

	JPH::BodyID bodyID = joltBody->GetBodyID();

	JPH::AABox bounds = mPhysicsSystem->GetBodyInterface().GetTransformedShape(bodyID).GetWorldSpaceBounds();
	bounds.ExpandBy(JPH::Vec3(0.5f, 0.5f, 0.5f));

	// Query broadphase for overlapping bodies
	JPH::AllHitCollisionCollector<JPH::CollideShapeBodyCollector> collector;
	mPhysicsSystem->GetBroadPhaseQuery().CollideAABox(
		bounds,
		collector,
		JPH::BroadPhaseLayerFilter{},
		JPH::ObjectLayerFilter{}
	);

	// Wake all collected bodies
	if (!collector.mHits.empty()) {
		mPhysicsSystem->GetBodyInterface().ActivateBodies(
			collector.mHits.data(),
			static_cast<int>(collector.mHits.size())
		);
	}
}

std::vector<DebugLineVertex> JoltPhysicsEngine::GetDebugLines() {
	if (!mPhysicsSystem || !mDebugRenderer) return {};

	std::vector<DebugLineVertex> out;

	JPH::BodyIDVector ids;
	mPhysicsSystem->GetBodies(ids);
	const JPH::BodyLockInterface& lockIf = mPhysicsSystem->GetBodyLockInterfaceNoLock();

	for (JPH::BodyID id : ids) {
		JPH::BodyLockRead lock(lockIf, id);
		if (!lock.Succeeded()) continue;
		const JPH::Body& body = lock.GetBody();
		const uint32_t key = id.GetIndexAndSequenceNumber();

		// reuse cached lines if the body hasn't moved
		if (body.IsStatic()) {
			auto it = mStaticLineCache.find(key);
			if (it != mStaticLineCache.end()
				&& it->second.pos == body.GetPosition()
				&& it->second.rot == body.GetRotation()) {
				if (out.size() + it->second.lines.size() <= kMaxDebugLineVertices)
					out.insert(out.end(), it->second.lines.begin(), it->second.lines.end());
				continue;
			}
		}

		JPH::Color color = JPH::Color::sGrey;

		mDebugRenderer->Clear();
		body.GetShape()->Draw(mDebugRenderer.get(), body.GetCenterOfMassTransform(),
			JPH::Vec3::sReplicate(1.0f), color, false, true);
		const auto& lines = mDebugRenderer->GetLines();

		if (body.IsStatic())
			mStaticLineCache[key] = { body.GetPosition(), body.GetRotation(), lines };

		if (out.size() + lines.size() <= kMaxDebugLineVertices) // skip whole body if it won't fit
			out.insert(out.end(), lines.begin(), lines.end());
	}
	return out;
}