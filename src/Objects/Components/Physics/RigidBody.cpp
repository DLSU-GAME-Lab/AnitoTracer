#include "RigidBody.hpp"
#include "Collider.hpp"
#include "StaticBody.hpp"
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
	// std::cout << "[DEBUG] RigidBody created.
	if (HierarchyObject* o = m_owner.GetPtr()) {
		TakeOverAutoStaticBody(o);
	}
}

void RigidBody::OnFixedUpdate(float deltaTime) {
	InitializeBody();

	if (!mBody) return;

	for (Collider* c : mColliders) {
		if (c) {
			c->EnsureAttached();
		}
	}

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

	DestroyBody();
	CreateBody(pos, rot, mMass);
}

void RigidBody::InitializeBody() {
	if (mBody) return;  // Already initialized

	glm::vec3 startPos(0.0f);
	glm::quat startRot(1.0f, 0.0f, 0.0f, 0.0f);

	HierarchyObject* o = m_owner.GetPtr();
	if (o && o->GetTransform()) {
		startPos = o->GetTransform()->GetPosition();
		startRot = o->GetTransform()->GetRotation();
	}

	CreateBody(startPos, startRot, mMass);
	std::cout << "[DEBUG] RigidBody body initialized with mass=" << mMass << std::endl;
}


void RigidBody::TakeOverAutoStaticBody(HierarchyObject* owner) {
	PhysicsBase* existing = owner->GetComponent<PhysicsBase>();
	if (!existing || existing == this) return;

	StaticBody* autoBody = dynamic_cast<StaticBody*>(existing);
	if (!autoBody || !autoBody->WasAutoCreated()) {
		// Either no prior body, or a StaticBody the caller added deliberately.
		return;
	}

	AbsorbFrom(autoBody);
	owner->RemoveComponent(autoBody);
}

void RigidBody::OnOwnerAttached() {
	if (HierarchyObject* o = m_owner.GetPtr()) {
		TakeOverAutoStaticBody(o);
	}
}