#include "TracerCollisionHandlers.h"

void TracerCollisionHandlers::ChangeColor(GameObject* obj)
{

	obj->getModel()->getMaterial(0)->SetAlbedoColor(glm::vec4(0,0,1,1));

}

JPH::ValidateResult TracerCollisionHandlers::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult)
{
	return JPH::ValidateResult();
}

void TracerCollisionHandlers::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
	std::cout << "TRACER PHYS ADDED" << std::endl;

	auto gameObj1 = TracerPhysics::GetInstance().GetGameObjectByBodyID(inBody1.GetID());
	auto gameObj2 = TracerPhysics::GetInstance().GetGameObjectByBodyID(inBody2.GetID());

	if (gameObj1 != nullptr) ChangeColor(gameObj1);
	if (gameObj2 != nullptr) ChangeColor(gameObj2);
}

void TracerCollisionHandlers::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{}

void TracerCollisionHandlers::OnContactRemoved(const JPH::SubShapeIDPair & inSubShapePair)
{}
