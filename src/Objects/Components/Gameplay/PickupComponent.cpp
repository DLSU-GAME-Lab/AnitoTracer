#include "PickupComponent.hpp"

#include "HierarchyObject.hpp"
#include "Components/Transform.hpp"

PickupComponent::PickupComponent(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner)
	: ComponentBase("PickupComponent", owner), m_transform(transform) {
	GBE_Init();
}

PickupComponent::~PickupComponent() = default;

Transform* PickupComponent::ResolveTransform() const {
	if (m_transform) {
		return m_transform;
	}

	if (HierarchyObject::Ref owner = GetOwner()) {
		return owner.GetPtr() ? owner.GetPtr()->GetTransform() : nullptr;
	}

	return nullptr;
}

void PickupComponent::GBE_Init()
{
}
