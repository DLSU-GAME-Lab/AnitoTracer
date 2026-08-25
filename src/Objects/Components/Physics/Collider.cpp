#include "Collider.hpp"
#include "StaticBody.hpp"
#include "../../HierarchyManager.hpp"

Collider::Collider(
	gbe::IInstanceManager<HierarchyObject>::Ref owner,
	IPhysicsEngine::ShapeType shapeType,
	IPhysicsEngine::ShapeParams shapeParams,
	const glm::vec3& offset)
	: ComponentBase("Collider", owner)
	, mShapeType(shapeType)
	, mShapeParams(shapeParams)
	, mOffset(offset)
{
	AttachToOwner();
}

Collider::~Collider() {
	DetachFromOwner();
}

void Collider::AttachToOwner() {
	HierarchyObject* o = m_owner.GetPtr();
	if (!o) return;

	PhysicsBase* body = o->GetComponent<PhysicsBase>();
	if (!body) {
		// No RigidBody (or StaticBody) present yet
		auto staticBody = std::make_unique<StaticBody>(m_owner);
		body = staticBody.get();
		o->AddComponent(std::move(staticBody));
	}

	mOwnerBody = body;
	mOwnerBody->RegisterCollider(this);
}

void Collider::DetachFromOwner() {
	if (mOwnerBody) {
		mOwnerBody->UnregisterCollider(this);
		mOwnerBody = nullptr;
	}
	// Note: doesn't clean up an orphaned auto-created StaticBody left with
	// zero colliders.
}

void Collider::SetShapeType(IPhysicsEngine::ShapeType type) {
	mShapeType = type;
	if (mOwnerBody) mOwnerBody->RebuildShapes();
}

void Collider::SetShapeParams(const IPhysicsEngine::ShapeParams& params) {
	mShapeParams = params;
	if (mOwnerBody) mOwnerBody->RebuildShapes();
}

void Collider::SetOffset(const glm::vec3& offset) {
	mOffset = offset;
	if (mOwnerBody) mOwnerBody->RebuildShapes();
}