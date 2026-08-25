#pragma once

#include "../ComponentBase.hpp"
#include "../ITeleportable.hpp"
#include "../../../Physics/IPhysicsEngine.hpp"
#include "../../../Physics/PhysicsEngine.hpp"

#include "Organization/IInstanceManager.hpp"

#include ANITO_EVENT_INCLUDES

class PhysicsBase : public ComponentBase, public gbe::IInstanceManager<PhysicsBase>, public ITeleportable {
public:
	PhysicsBase(const std::string& name, gbe::IInstanceManager<HierarchyObject>::Ref owner = {})
		: ComponentBase(name, owner) {}

	~PhysicsBase() override {
		DestroyBody();
	}

	PhysicsBase(const PhysicsBase&) = delete;
	PhysicsBase& operator=(const PhysicsBase&) = delete;

	PhysicsBase(PhysicsBase&&) = default;
	PhysicsBase& operator=(PhysicsBase&&) = default;

	// Access body for raycasting, etc
	IPhysicsBody* GetBody() const { return mBody.get(); }

	// Directly move the body (bypasses simulation)
	void Teleport(const glm::vec3& position, const glm::quat& rotation) override;

protected:
	void CreateBody(
		const glm::vec3& position,
		const glm::quat& rotation,
		float mass,
		IPhysicsEngine::ShapeType shapeType,
		IPhysicsEngine::ShapeParams shapeParams
	);

	void DestroyBody();

	// Called via the per-body callback
	void HandleCollision(
		std::shared_ptr<IPhysicsBody> self,
		std::shared_ptr<IPhysicsBody> other,
		const glm::vec3& contactPoint
	);

	std::shared_ptr<IPhysicsBody> mBody;

	GBE_GENERATE_SERIALIZER_CONSTRUCTOR(PhysicsBase, ComponentBase);
};