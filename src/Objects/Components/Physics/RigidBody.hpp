#pragma once

#include "PhysicsBase.hpp"
#include "../../HierarchyObject.hpp"
#include "../../../ObjectSystems/Event/Types/FixedUpdateTrigger.hpp"

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
	void ApplyImpulse(const glm::vec3& impulse);
	void SetVelocity(const glm::vec3& velocity);
	glm::vec3 GetVelocity() const;
	void SetAngularVelocity(const glm::vec3& angularVelocity);
	glm::vec3 GetAngularVelocity() const;

	void SetMass(float mass);
	float GetMass() const;

	// Recreate body with new shape at runtime
	void Rebuild(IPhysicsEngine::ShapeType shapeType, IPhysicsEngine::ShapeParams shapeParams);

	virtual std::string GetLabel() override { return "RigidBody"; }

protected:
	float mMass = 1.0f;
	IPhysicsEngine::ShapeType mShapeType = IPhysicsEngine::ShapeType::Box;
	IPhysicsEngine::ShapeParams mShapeParams = {};

	GBE_SERIALIZE_FIELD_W_CB(mMass, [this](float) {
		if (mBody) mBody->SetMass(mMass);
	});

	GBE_GENERATE_SERIALIZER_CONSTRUCTOR(RigidBody, PhysicsBase);
};

GBE_REGISTER_SERIALIZED_TYPE(RigidBody, PhysicsBase);