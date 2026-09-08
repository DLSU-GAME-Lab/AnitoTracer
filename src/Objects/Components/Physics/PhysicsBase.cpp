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

void PhysicsBase::SetRestitution(float restitution) {
	mRestitution = restitution;
	if (mBody) {
		PhysicsEngine::GetInstance().Get().SetRestitution(mBody.get(), mRestitution);
	}
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

	std::cout << "[DEBUG] Collider registered on body " << this
		<< ". Total colliders: " << mColliders.size() << std::endl;

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
	std::cout << "[DEBUG] RebuildShapes on " << this
		<< " (type=" << typeid(*this).name() << ")"
		<< ", mBody=" << mBody.get()
		<< ", mColliders.size()=" << mColliders.size() << std::endl;
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
	mBody = engine.CreateRigidBody(position, rotation, mass, shapes, mRestitution);
	if (!mBody) {
		std::cerr << "[PhysicsBase] Error: Failed to create physics body.\n";
		return;
	}

	std::cout << "[DEBUG] Physics body created successfully" << std::endl;

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

void PhysicsBase::AbsorbFrom(PhysicsBase* other) {
	if (!other || other == this) return;

	std::vector<Collider*> taken = other->TakeColliders();
	if (taken.empty()) return;

	for (Collider* c : taken) {
		if (!c) continue;
		c->Reparent(this);
		mColliders.push_back(c);
	}

	std::cout << "[DEBUG] Absorbed " << taken.size()
		<< " collider(s) from auto-created body " << other
		<< " into body " << this << std::endl;

	RebuildShapes();
}