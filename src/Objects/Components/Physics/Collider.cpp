#include "Collider.hpp"
#include "RigidBody.hpp"
#include "StaticBody.hpp"
#include "../ModelComponent.hpp"
#include "../../HierarchyManager.hpp"
#include "../../../Rendering/Models/ModelManager.hpp"

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
	std::cout << "[DEBUG] Collider constructor called" << std::endl;

	if (owner.GetPtr() != nullptr) {
		AttachToOwner();
	}
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
		staticBody->MarkAutoCreated();
		body = staticBody.get();
		o->AddComponent(std::move(staticBody));
	}

	mOwnerBody = body;
	mOwnerBody->RegisterCollider(this);

	if (!body->GetBody()) {
		if (RigidBody* rb = dynamic_cast<RigidBody*>(body)) {
			std::cout << "[DEBUG] Initializing RigidBody" << std::endl;
			rb->InitializeBody();
		}
		else if (StaticBody* sb = dynamic_cast<StaticBody*>(body)) {
			std::cout << "[DEBUG] Initializing StaticBody" << std::endl;
			sb->InitializeBody();
		}
	}
}

IPhysicsEngine::ColliderShape Collider::GetShapeDescriptor() const {
	glm::vec3 scale(1.0f);
	if (HierarchyObject* o = m_owner.GetPtr()) {
		if (Transform* t = o->GetTransform()) {
			scale = t->GetScale();
		}
	}

	if (mShapeType == IPhysicsEngine::ShapeType::Mesh ||
		mShapeType == IPhysicsEngine::ShapeType::ConvexHull) {
		IPhysicsEngine::ColliderShape shape;
		shape.type = mShapeType;
		shape.offset = mOffset * scale;
		shape.scale = scale;

		Model* model = nullptr;
		if (mMeshSource == MeshSource::FromOwner) {
			if (HierarchyObject* o = m_owner.GetPtr()) {
				if (ModelComponent* mc = o->GetComponent<ModelComponent>()) {
					if (mc->HasModel()) {
						model = mc->GetModel().Get();
					}
				}
			}
		}
		else {
			model = ModelManager::GetInstance().LoadModel(mMeshPath);
		}

		if (model) {
			shape.meshData = model->CollisionData;
		}

		return shape;
	}

	IPhysicsEngine::ShapeParams scaledParams = mShapeParams;
	scaledParams.v *= scale;
	return { mShapeType, scaledParams, mOffset * scale };
}

void Collider::DetachFromOwner() {
	if (mOwnerBody) {
		mOwnerBody->UnregisterCollider(this);
		mOwnerBody = nullptr;
	}
	// Note: doesn't clean up an orphaned auto-created StaticBody left with
	// zero colliders.
}

void Collider::EnsureAttached() {
	if (!mOwnerBody) {
		AttachToOwner();
	}
}

void Collider::SetShapeType(IPhysicsEngine::ShapeType type) {
	mShapeType = type;
	EnsureAttached();
	if (mOwnerBody) mOwnerBody->RebuildShapes();
}

void Collider::SetShapeParams(const IPhysicsEngine::ShapeParams& params) {
	mShapeParams = params;
	EnsureAttached();
	if (mOwnerBody) mOwnerBody->RebuildShapes();
}

void Collider::SetOffset(const glm::vec3& offset) {
	if (offset == mOffset) return;
	mOffset = offset;
	EnsureAttached();
	if (mOwnerBody) mOwnerBody->RebuildShapes();
}

void Collider::SetMeshPath(const std::string& path) {
	mMeshPath = path;
	EnsureAttached();
	if (mOwnerBody) mOwnerBody->RebuildShapes();
}

void Collider::SetMeshSource(MeshSource source) {
	mMeshSource = source;
	EnsureAttached();
	if (mOwnerBody) mOwnerBody->RebuildShapes();
}