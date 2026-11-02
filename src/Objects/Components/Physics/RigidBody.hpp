#pragma once

#include "PhysicsBase.hpp"
#include "ScriptApiRegistry.hpp"
#include "../../HierarchyObject.hpp"
#include "../../../ObjectSystems/Event/Types/FixedUpdateTrigger.hpp"

#include <cstdint>

struct PhysicsRaycastHit {
	bool hit = false;
	glm::vec3 point{ 0.0f };
	std::uint64_t object = 0;
};

ANITO_SCRIPT_TYPE_MAPPING(PhysicsRaycastHit, "raycasthit")

class RigidBody
	: public PhysicsBase
	, public gbe::ITrigger<FixedUpdateTrigger>
{
public:
	RigidBody(
		gbe::IInstanceManager<HierarchyObject>::Ref owner = {},
		float mass = 1.0f,
		IPhysicsEngine::ShapeType shapeType = IPhysicsEngine::ShapeType::Box,
		IPhysicsEngine::ShapeParams shapeParams = {}
	);

	~RigidBody() override = default;

	RigidBody(const RigidBody&) = delete;
	RigidBody& operator=(const RigidBody&) = delete;

	RigidBody(RigidBody&&) = default;
	RigidBody& operator=(RigidBody&&) = default;

	void OnFixedUpdate(float deltaTime) override;

	// Physics API
	void ApplyForce(const glm::vec3& force);
	void ApplyTorque(const glm::vec3& torque);
	void ApplyImpulse(const glm::vec3& impulse);
	void SetVelocity(const glm::vec3& velocity);
	glm::vec3 GetVelocity() const;
	void SetAngularVelocity(const glm::vec3& angularVelocity);
	glm::vec3 GetAngularVelocity() const;
	glm::vec3 GetWorldGravity() const;
	void SetWorldGravity(const glm::vec3& gravity);
	PhysicsRaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const;

	virtual void SetMass(float mass);
	float GetMass() const;
	void SetLockRotationX(bool lock) { mLockRotationX = lock; ApplyRotationLocks(); }
	void SetLockRotationY(bool lock) { mLockRotationY = lock; ApplyRotationLocks(); }
	void SetLockRotationZ(bool lock) { mLockRotationZ = lock; ApplyRotationLocks(); }
	bool GetLockRotationX() const { return mLockRotationX; }
	bool GetLockRotationY() const { return mLockRotationY; }
	bool GetLockRotationZ() const { return mLockRotationZ; }

	// Recreate body with new shape at runtime
	void Rebuild(IPhysicsEngine::ShapeType shapeType, IPhysicsEngine::ShapeParams shapeParams);

	void InitializeBody();

	virtual std::string GetLabel() override { return "RigidBody"; }

protected:
	bool mPlacedFromTransform = false;
	float mMass = 1.0f;
	IPhysicsEngine::ShapeType mShapeType = IPhysicsEngine::ShapeType::Box;
	IPhysicsEngine::ShapeParams mShapeParams = {};

	GBE_SERIALIZE_FIELD_W_CB(mMass, [this](float) {
		if (mBody) mBody->SetMass(mMass);
	});

	bool mLockRotationX = false;
	GBE_SERIALIZE_FIELD_W_NAME_CB(mLockRotationX, "Lock Rotation X", [this](bool) { ApplyRotationLocks(); });

	bool mLockRotationY = false;
	GBE_SERIALIZE_FIELD_W_NAME_CB(mLockRotationY, "Lock Rotation Y", [this](bool) { ApplyRotationLocks(); });

	bool mLockRotationZ = false;
	GBE_SERIALIZE_FIELD_W_NAME_CB(mLockRotationZ, "Lock Rotation Z", [this](bool) { ApplyRotationLocks(); });

	GBE_GENERATE_SERIALIZER_CONSTRUCTOR(RigidBody, PhysicsBase);

	void TakeOverAutoStaticBody(HierarchyObject* owner);
	void ApplyRotationLocks();
	void OnOwnerAttached() override;
};

GBE_REGISTER_SERIALIZED_TYPE(RigidBody, PhysicsBase);

ANITO_SCRIPT_TYPE(RigidBody, "rigidbody", "anito::Rigidbody", true);
ANITO_SCRIPT_PROPERTY(RigidBody, "rigidbody", "velocity", GetVelocity, SetVelocity);
ANITO_SCRIPT_PROPERTY(RigidBody, "rigidbody", "angularVelocity", GetAngularVelocity, SetAngularVelocity);
ANITO_SCRIPT_PROPERTY(RigidBody, "rigidbody", "mass", GetMass, SetMass);
ANITO_SCRIPT_PROPERTY(RigidBody, "rigidbody", "worldGravity", GetWorldGravity, SetWorldGravity);
ANITO_SCRIPT_METHOD(RigidBody, "rigidbody", "addForce", ApplyForce);
ANITO_SCRIPT_METHOD(RigidBody, "rigidbody", "addTorque", ApplyTorque);
ANITO_SCRIPT_METHOD(RigidBody, "rigidbody", "addImpulse", ApplyImpulse);
ANITO_SCRIPT_METHOD(RigidBody, "rigidbody", "teleport", Teleport);
ANITO_SCRIPT_METHOD(RigidBody, "rigidbody", "raycast", Raycast);