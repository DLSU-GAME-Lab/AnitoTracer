#pragma once

#include "IPhysicsEngine.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <map>

class JoltPhysicsBody;
class JoltContactListener;
class JoltDebugRenderer;

class JoltPhysicsEngine : public IPhysicsEngine {
public:
	JoltPhysicsEngine();
	~JoltPhysicsEngine() override;

	// World management
	void SetGravity(const glm::vec3& gravity) override;
	glm::vec3 GetGravity() const override;
	bool SetRestitution(IPhysicsBody* body, float restitution) override;
	bool SetFriction(IPhysicsBody* body, float friction) override;

	// Body management
	std::shared_ptr<IPhysicsBody> CreateRigidBody(
		const glm::vec3& position,
		const glm::quat& rotation,
		float mass,
		const std::vector<ColliderShape>& shapes = {},
		float restitution = 0.0f
	) override;
	void DestroyRigidBody(std::shared_ptr<IPhysicsBody> body) override;
	bool SetShapes(IPhysicsBody* body, const std::vector<ColliderShape>& shapes) override;

	// Collision callback
	void RegisterCollisionCallback(IPhysicsBody* body, CollisionCallback callback) override;
	void UnregisterCollisionCallback(IPhysicsBody* body) override;

	// Simulation
	void Step(float deltaTime) override;

	// Raycasting
	bool Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, std::shared_ptr<IPhysicsBody>& outBody, glm::vec3& outHitPoint) override;

	// Activate bodies
	void WakeBodiesAroundBody(IPhysicsBody* body) override;

	std::vector<DebugLineVertex> GetDebugLines() override;

	// Internal access for JoltPhysicsBody
	JPH::PhysicsSystem* GetPhysicsSystem() { return mPhysicsSystem.get(); }
	JPH::BodyInterface& GetBodyInterface() { return mPhysicsSystem->GetBodyInterface(); }

	bool HasCollisionCallback() const;
	void InvokeCollisionCallbacks(std::shared_ptr<IPhysicsBody> bodyA, std::shared_ptr<IPhysicsBody> bodyB, const glm::vec3& point);
	std::shared_ptr<JoltPhysicsBody> FindBodyByID(JPH::BodyID id);

private:
	JPH::ShapeSettings::ShapeResult BuildShapeSettings(const ColliderShape& shape);
	JPH::RefConst<JPH::Shape> BuildCompoundShape(const std::vector<ColliderShape>& shapes);
	std::unique_ptr<JPH::JobSystemThreadPool> mJobSystem;
	std::unique_ptr<JPH::PhysicsSystem> mPhysicsSystem;
	std::unique_ptr<JoltContactListener> mContactListener;
	std::unique_ptr<JoltDebugRenderer> mDebugRenderer;

	std::map<JPH::BodyID, std::shared_ptr<JoltPhysicsBody>> mBodies;
	glm::vec3 mGravity;

	std::map<IPhysicsBody*, CollisionCallback> mCollisionCallbacks;

	struct CachedLines {
		JPH::RVec3 pos;
		JPH::Quat rot;
		std::vector<DebugLineVertex> lines;
	};
	std::unordered_map<uint32_t, CachedLines> mStaticLineCache; // key is BodyID index+sequence
};