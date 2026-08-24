#pragma once


#include "Components/Transform.hpp"
#include "Components/ComponentBase.hpp"

class KeyComponent : public ComponentBase {
public:
    KeyComponent(Transform* transform = nullptr, gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~KeyComponent() override;

    KeyComponent(const KeyComponent&) = delete;
    KeyComponent& operator=(const KeyComponent&) = delete;

    KeyComponent(KeyComponent&&) = default;
    KeyComponent& operator=(KeyComponent&&) = default;

private:
    Transform* m_transform = nullptr;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(KeyComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(KeyComponent, ComponentBase);