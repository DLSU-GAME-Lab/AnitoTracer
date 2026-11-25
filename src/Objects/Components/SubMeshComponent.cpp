#include "SubMeshComponent.hpp"

#include "ModelComponent.hpp"
#include "../HierarchyObject.hpp"

ModelComponent* SubMeshComponent::GetModelComponent() const {
    HierarchyObject* owner = m_owner.GetPtr();
    HierarchyObject* parent = owner ? owner->GetParent().GetPtr() : nullptr;
    return parent ? parent->GetComponent<ModelComponent>() : nullptr;
}

const SubMesh* SubMeshComponent::GetSubMesh() const {
    ModelComponent* modelComponent = GetModelComponent();
    const Model* model = modelComponent ? modelComponent->GetModel().Get() : nullptr;
    if (!model || m_submeshIndex < 0 || static_cast<size_t>(m_submeshIndex) >= model->SubMeshes.size()) {
        return nullptr;
    }
    return &model->SubMeshes[static_cast<size_t>(m_submeshIndex)];
}

bool SubMeshComponent::IsOrphaned() const {
    ModelComponent* modelComponent = GetModelComponent();
    if (!modelComponent || !modelComponent->HasModel()) {
        return true;
    }

    // The asset is not available right now (still loading, being reloaded): the node may well be right, so leave it be
    const Model* model = modelComponent->GetModel().Get();
    if (!model) {
        return false;
    }

    return model->SubMeshes.size() < 2 || m_submeshIndex < 0 || static_cast<size_t>(m_submeshIndex) >= model->SubMeshes.size();
}

bool SubMeshComponent::IsPristine() const {
    if (HasAnyOverride()) {
        return false;
    }

    const HierarchyObject* owner = m_owner.GetPtr();
    if (!owner) {
        return true;
    }
    // The Transform and this component
    return owner->GetChildren().empty() && owner->GetComponents().size() <= 2;
}
