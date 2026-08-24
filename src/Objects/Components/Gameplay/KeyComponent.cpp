#include "KeyComponent.hpp"
#include "Components/Transform.hpp"

KeyComponent::KeyComponent(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("KeyComponent", owner), m_transform(transform) {}

KeyComponent::~KeyComponent() = default;