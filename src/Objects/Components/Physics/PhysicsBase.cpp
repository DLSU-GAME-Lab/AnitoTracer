#include "PhysicsBase.hpp"
#include "../Transform.hpp"
#include "../../HierarchyObject.hpp"

void PhysicsBase::Teleport(const glm::vec3& position, const glm::quat& rotation) {
	if (!mBody) return;

	mBody->SetPosition(position);
	mBody->SetRotation(rotation);
	mBody->SetVelocity(glm::vec3(0.0f));
	mBody->SetAngularVelocity(glm::vec3(0.0f));

	if (HierarchyObject* owner = m_owner.GetPtr()) {
		if (Transform* transform = owner->GetTransform()) {
			transform->SetPosition(position);
			transform->SetRotation(rotation);
		}
	}
}

void PhysicsBase::CreateBody(
	const glm::vec3& position,
	const glm::quat& rotation,
	float mass,
	IPhysicsEngine::ShapeType shapeType,
	IPhysicsEngine::ShapeParams shapeParams)
{
	// Ensure the body is not already created
	if (mBody) {
		std::cerr << "[PhysicsBase] Error: Body already exists. Destroy it before creating a new one.\n";
		return;
	}

	IPhysicsEngine& engine = PhysicsEngine::GetInstance().Get();

	// Create the physics body using the PhysicsEngine singleton
	mBody = engine.CreateRigidBody(position, rotation, mass, shapeType, shapeParams);
	if (!mBody) {
		std::cerr << "[PhysicsBase] Error: Failed to create physics body.\n";
		return;
	}

	engine.RegisterCollisionCallback(
		mBody.get(),
		[this](
			std::shared_ptr<IPhysicsBody> self, 
			std::shared_ptr<IPhysicsBody> other, 
			const glm::vec3& contactPoint) 
		{
			HandleCollision(self, other, contactPoint);
		}
	);
}

void PhysicsBase::DestroyBody() {
	if (!mBody) return;

	IPhysicsEngine& engine = PhysicsEngine::GetInstance().Get();
	engine.UnregisterCollisionCallback(mBody.get());
	engine.DestroyRigidBody(mBody);
	mBody.reset();
}

void PhysicsBase::HandleCollision(
	std::shared_ptr<IPhysicsBody> self,
	std::shared_ptr<IPhysicsBody> other,
	const glm::vec3& contactPoint)
{
	HierarchyObject* owner = m_owner.GetPtr();
	if (!owner) return;

	CollisionEnterTrigger event{ self, other, contactPoint };
	owner->DispatchEventData<CollisionEnterTrigger>(event);
}