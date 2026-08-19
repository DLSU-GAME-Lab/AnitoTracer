#include "RigidBody.hpp"
#include "../Transform.hpp"
#include "../../HierarchyObject.hpp"

RigidBody::RigidBody(
	gbe::IInstanceManager<HierarchyObject>::Ref owner,
	float mass,
	IPhysicsEngine::ShapeType shapeType,
	IPhysicsEngine::ShapeParams shapeParams)
	: PhysicsBase("RigidBody", owner)
	, mMass(mass)
	, mShapeType(shapeType)
	, mShapeParams(shapeParams)
{
	glm::vec3 startPos(0.0f);
	glm::quat startRot(1.0f, 0.0f, 0.0f, 0.0f);

	if (HierarchyObject* o = m_owner.GetPtr()) {
		if (Transform* t = o->GetTransform()) {
			startPos = t->GetPosition();
			startRot = t->GetRotation();
		}
	}

	CreateBody(startPos, startRot, mMass, mShapeType, mShapeParams);
}

void RigidBody::OnFixedUpdate(float deltaTime) {
	if (!mBody) return;

	HierarchyObject* owner = m_owner.GetPtr();
	if (!owner) return;

	Transform* transform = owner->GetTransform();
	if (!transform) return;

	transform->SetPosition(mBody->GetPosition());
	transform->SetRotation(mBody->GetRotation());
}

void RigidBody::ApplyForce(const glm::vec3& force) {
	if (mBody) {
		mBody->ApplyForce(force);
	}
}

void RigidBody::ApplyImpulse(const glm::vec3& impulse) {
	if (mBody) {
		mBody->ApplyImpulse(impulse);
	}
}

void RigidBody::SetVelocity(const glm::vec3& velocity) {
	if (mBody) {
		mBody->SetVelocity(velocity);
	}
}

glm::vec3 RigidBody::GetVelocity() const {
	return mBody ? mBody->GetVelocity() : glm::vec3(0.0f);
}

void RigidBody::SetAngularVelocity(const glm::vec3& angularVelocity) {
	if (mBody) {
		mBody->SetAngularVelocity(angularVelocity);
	}
}

glm::vec3 RigidBody::GetAngularVelocity() const {
	return mBody ? mBody->GetAngularVelocity() : glm::vec3(0.0f);
}

void RigidBody::SetMass(float mass) {
	mMass = mass;
	if (mBody) {
		mBody->SetMass(mass);
	}
}

float RigidBody::GetMass() const {
	return mMass;
}

void RigidBody::Rebuild(
	IPhysicsEngine::ShapeType shapeType, 
	IPhysicsEngine::ShapeParams shapeParams) 
{
	mShapeType = shapeType;
	mShapeParams = shapeParams;

	glm::vec3 pos(0.0f);
	glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);

	if (HierarchyObject* o = m_owner.GetPtr()) {
		if (Transform* t = o->GetTransform()) {
			pos = t->GetPosition();
			rot = t->GetRotation();
		}
	}

	CreateBody(pos, rot, mMass, mShapeType, mShapeParams);
}