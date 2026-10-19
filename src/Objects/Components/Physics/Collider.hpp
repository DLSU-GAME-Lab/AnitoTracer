#pragma once

#include "../../HierarchyObject.hpp"
#include "../ComponentBase.hpp"
#include "PhysicsBase.hpp"
#include "../../../Physics/IPhysicsEngine.hpp"

enum class MeshSource {
	FromPath,
	FromOwner
};

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
	void SetMeshPath(const std::string& path);
	void SetMeshSource(MeshSource source);

	IPhysicsEngine::ShapeType GetShapeType() const { return mShapeType; }
	const IPhysicsEngine::ShapeParams& GetShapeParams() const { return mShapeParams; }
	const glm::vec3& GetOffset() const { return mOffset; }
	const std::string& GetMeshPath() const { return mMeshPath; }
	MeshSource GetMeshSource() const { return mMeshSource; }

	IPhysicsEngine::ColliderShape GetShapeDescriptor() const;

	// Used only by Physicsbase/RigidBody when handling this Collider off
	void Reparent(PhysicsBase* newOwner) { mOwnerBody = newOwner; }

	virtual std::string GetLabel() override { return "Collider"; }

	// Call after deserialization to ensure it is attached to owner
	void EnsureAttached();

protected:
	IPhysicsEngine::ShapeType mShapeType = IPhysicsEngine::ShapeType::Box;
	IPhysicsEngine::ShapeParams mShapeParams = {};
	glm::vec3 mOffset = glm::vec3(0.0f);
	std::string mMeshPath;
	MeshSource mMeshSource = MeshSource::FromOwner;


	GBE_SERIALIZE_FIELD_W_CB(mShapeType, [this](IPhysicsEngine::ShapeType&) {
		if (mOwnerBody) mOwnerBody->RebuildShapes();
		});
	GBE_SERIALIZE_FIELD_W_CB(mShapeParams, [this](IPhysicsEngine::ShapeParams&) {
		if (mOwnerBody) mOwnerBody->RebuildShapes();
		});
	GBE_SERIALIZE_FIELD_W_CB(mMeshPath, [this](std::string&) {
		if (mOwnerBody) mOwnerBody->RebuildShapes();
		});
	GBE_SERIALIZE_FIELD_W_CB(mMeshSource, [this](MeshSource&) {
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