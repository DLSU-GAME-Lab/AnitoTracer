#include "PhysicsBase.hpp"
#include "Collider.hpp"
#include "../Transform.hpp"
#include "../../HierarchyObject.hpp"

PhysicsBase::~PhysicsBase() {
	for (Collider* c : mColliders) {
		if (c) {
			c->Reparent(nullptr);
		}
	}
	mColliders.clear();
	DestroyBody();
}

void PhysicsBase::Teleport(const glm::vec3& position, const glm::quat& rotation) {
	// For debugging purposes, you can uncomment the following line to see when teleportation occurs.
	// std::cout << "[DEBUG] Teleport called!\n";
	if (!mBody) return;

	mBody->SetPositionAndRotation(position, rotation);
	mBody->SetVelocity(glm::vec3(0.0f));
	mBody->SetAngularVelocity(glm::vec3(0.0f));

	if (HierarchyObject* owner = m_owner.GetPtr()) {
		if (Transform* transform = owner->GetTransform()) {
			transform->SetPosition(position);
			transform->SetRotation(rotation);
		}
	}
}

void PhysicsBase::RegisterCollider(Collider* collider) {
	if (!collider) return;
	if (std::find(mColliders.begin(), mColliders.end(), collider) != mColliders.end()) return;
	mColliders.push_back(collider);
	RebuildShapes();
}

void PhysicsBase::UnregisterCollider(Collider* collider) {
	if (!collider) return;
	auto it = std::find(mColliders.begin(), mColliders.end(), collider);
	if (it != mColliders.end()) {
		mColliders.erase(it);
		RebuildShapes();
	}
}

void PhysicsBase::RebuildShapes() {
	if (!mBody) return;

	std::vector<IPhysicsEngine::ColliderShape> shapes;
	shapes.reserve(mColliders.size());
	for (Collider* c : mColliders) {
		shapes.push_back(c->GetShapeDescriptor());
	}

	PhysicsEngine::GetInstance().Get().SetShapes(mBody.get(), shapes);
}

std::vector<Collider*> PhysicsBase::TakeColliders() {
	std::vector<Collider*> result = std::move(mColliders);
	mColliders.clear();
	return result;
}

void PhysicsBase::CreateBody(
	const glm::vec3& position,
	const glm::quat& rotation,
	float mass)
{
	// Ensure the body is not already created
	if (mBody) {
		std::cerr << "[PhysicsBase] Error: Body already exists. Destroy it before creating a new one.\n";
		return;
	}

	IPhysicsEngine& engine = PhysicsEngine::GetInstance().Get();

	std::vector<IPhysicsEngine::ColliderShape> shapes;
	shapes.reserve(mColliders.size());
	for (Collider* c : mColliders) {
		shapes.push_back(c->GetShapeDescriptor());
	}

	// Create the physics body using the PhysicsEngine singleton
	mBody = engine.CreateRigidBody(position, rotation, mass, shapes);
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