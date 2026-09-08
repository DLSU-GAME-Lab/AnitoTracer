#pragma once

#include "../ComponentBase.hpp"
#include "../ITeleportable.hpp"
#include "../../../Physics/IPhysicsEngine.hpp"
#include "../../../Physics/PhysicsEngine.hpp"

#include "Organization/IInstanceManager.hpp"

#include ANITO_EVENT_INCLUDES

#include <vector>

class Collider;

class PhysicsBase : public ComponentBase, public gbe::IInstanceManager<PhysicsBase>, public ITeleportable {
public:
	PhysicsBase(const std::string& name, gbe::IInstanceManager<HierarchyObject>::Ref owner = {})
		: ComponentBase(name, owner) {
		std::cout << "[DEBUG] PhysicsBase constructor called for: " << name << std::endl;
	}

	~PhysicsBase() override;

	PhysicsBase(const PhysicsBase&) = delete;
	PhysicsBase& operator=(const PhysicsBase&) = delete;

	PhysicsBase(PhysicsBase&&) = default;
	PhysicsBase& operator=(PhysicsBase&&) = default;

	// Access body for raycasting, etc
	IPhysicsBody* GetBody() const { return mBody.get(); }

	// Get/Set elasticity
	float GetRestitution() const { return mRestitution; }
	void SetRestitution(float restitution);

	// Directly move the body (bypasses simulation)
	void Teleport(const glm::vec3& position, const glm::quat& rotation) override;

	// Called by Collider components on attach/detach/change
	void RegisterCollider(Collider* collider);
	void UnregisterCollider(Collider* collider);
	void RebuildShapes();

	// Used when a RigidBody takes over an auto-created StaticBody's colliders
	std::vector<Collider*> TakeColliders();

protected:
	void CreateBody(
		const glm::vec3& position,
		const glm::quat& rotation,
		float mass
	);

	void DestroyBody();

	// Called via the per-body callback
	void HandleCollision(
		std::shared_ptr<IPhysicsBody> self,
		std::shared_ptr<IPhysicsBody> other,
		const glm::vec3& contactPoint
	);

	void AbsorbFrom(PhysicsBase* other);

	std::shared_ptr<IPhysicsBody> mBody;
	std::vector<Collider*> mColliders;
	float mRestitution = 0.0f;
	GBE_SERIALIZE_FIELD_W_CB(mRestitution, [this](float&) {
		if (mBody) PhysicsEngine::GetInstance().Get().SetRestitution(mBody.get(), mRestitution);
	});

	GBE_GENERATE_SERIALIZER_CONSTRUCTOR(PhysicsBase, ComponentBase);
};