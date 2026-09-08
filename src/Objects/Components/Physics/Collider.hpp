#pragma once

#include "../../HierarchyObject.hpp"
#include "../ComponentBase.hpp"
#include "PhysicsBase.hpp"
#include "../../../Physics/IPhysicsEngine.hpp"

class Collider : public ComponentBase {
public:
	Collider(
		gbe::IInstanceManager<HierarchyObject>::Ref owner = {},
		IPhysicsEngine::ShapeType shapeType = IPhysicsEngine::ShapeType::Box,
		IPhysicsEngine::ShapeParams shapeParams = {},
		const glm::vec3& offset = glm::vec3(0.0f)
	);

	~Collider() override;

	Collider(const Collider&) = delete;
	Collider& operator=(const Collider&) = delete;
	Collider(Collider&&) = default;
	Collider& operator=(Collider&&) = default;

	void SetShapeType(IPhysicsEngine::ShapeType type);
	void SetShapeParams(const IPhysicsEngine::ShapeParams& params);
	void SetOffset(const glm::vec3& offset);

	IPhysicsEngine::ShapeType GetShapeType() const { return mShapeType; }
	const IPhysicsEngine::ShapeParams& GetShapeParams() const { return mShapeParams; }
	const glm::vec3& GetOffset() const { return mOffset; }

	IPhysicsEngine::ColliderShape GetShapeDescriptor() const {
		glm::vec3 scale(1.0f);
		if (HierarchyObject* o = m_owner.GetPtr()) {
			if (Transform* t = o->GetTransform()) {
				scale = t->GetScale();
			}
		}

		IPhysicsEngine::ShapeParams scaledParams = mShapeParams;
		scaledParams.v *= scale;

		return { mShapeType, scaledParams, mOffset * scale };
	}

	// Used only by Physicsbase/RigidBody when handling this Collider off
	void Reparent(PhysicsBase* newOwner) { mOwnerBody = newOwner; }

	virtual std::string GetLabel() override { return "Collider"; }

	// Call after deserialization to ensure it is attached to owner
	void EnsureAttached();

protected:
	IPhysicsEngine::ShapeType mShapeType = IPhysicsEngine::ShapeType::Box;
	IPhysicsEngine::ShapeParams mShapeParams = {};
	glm::vec3 mOffset = glm::vec3(0.0f);


	GBE_SERIALIZE_FIELD_W_CB(mShapeType, [this](IPhysicsEngine::ShapeType&) {
		if (mOwnerBody) mOwnerBody->RebuildShapes();
		});
	GBE_SERIALIZE_FIELD_W_CB(mShapeParams, [this](IPhysicsEngine::ShapeParams&) {
		if (mOwnerBody) mOwnerBody->RebuildShapes();
		});
	GBE_SERIALIZE_FIELD(mOffset);

	PhysicsBase* mOwnerBody = nullptr;

	void AttachToOwner();
	void DetachFromOwner();
	void OnOwnerAttached() override { EnsureAttached(); }

	GBE_GENERATE_SERIALIZER_CONSTRUCTOR(Collider, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(Collider, ComponentBase);