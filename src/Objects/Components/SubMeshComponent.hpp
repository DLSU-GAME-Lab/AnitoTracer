#pragma once

#include "MaterialOverrideComponent.hpp"

#include <string>

class ModelComponent;
struct SubMesh;
struct Model;

// Marks a hierarchy object as one submesh of its parent's model, so that the part can be selected, renamed and given
// components like any object, and can override its material's properties on its own.
//
// HierarchyManager::SyncSubMeshNodes keeps one such child under every object whose model has more than one submesh. The
// node stands at the middle of its submesh, in the model's space, and keeps there (what is attached to the node, a light
// or a particle system, stays with the part); it does not move the geometry, which is baked into the model.
//
// The overrides of this component (MaterialOverrideComponent) are laid over what the object's ModelComponent overrides:
// a property whose switch is off here follows the object, and the material when the object has no override either.
class SubMeshComponent : public MaterialOverrideComponent {
public:
    SubMeshComponent(int submeshIndex = -1, const std::string& submeshName = "", gbe::IInstanceManager<HierarchyObject>::Ref owner = {})
        : MaterialOverrideComponent("SubMeshComponent", owner), m_submeshIndex(submeshIndex), m_submeshName(submeshName) {}

    ~SubMeshComponent() override = default;

    SubMeshComponent(const SubMeshComponent&) = delete;
    SubMeshComponent& operator=(const SubMeshComponent&) = delete;
    SubMeshComponent(SubMeshComponent&&) = default;
    SubMeshComponent& operator=(SubMeshComponent&&) = default;

    // Which submesh of the model this is, and what the model called it when the node was made (shown in the Inspector)
    int GetSubMeshIndex() const { return m_submeshIndex; }
    const std::string& GetSubMeshName() const { return m_submeshName; }
    void SetSubMesh(int index, const std::string& name) { m_submeshIndex = index; m_submeshName = name; }

    // The ModelComponent of the object this submesh belongs to (the node's parent), or null
    ModelComponent* GetModelComponent() const;

    // The submesh in that object's model, or null when the model is not available or has no such submesh
    const SubMesh* GetSubMesh() const;

    // True when the node no longer stands for a submesh: its parent has no model, or the model has no such submesh any more
    // (a model that was replaced or changed). Such a node does nothing, and may be moved and deleted like any object.
    bool IsOrphaned() const;

    // True when nothing here is worth keeping: no override, no component but the Transform and this one, no children
    bool IsPristine() const;

private:
    int m_submeshIndex = -1;
    GBE_SERIALIZE_FIELD_W_NAME(m_submeshIndex, "Submesh Index");

    std::string m_submeshName;
    GBE_SERIALIZE_FIELD_W_NAME(m_submeshName, "Submesh Name");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(SubMeshComponent, MaterialOverrideComponent);
};

GBE_REGISTER_SERIALIZED_TYPE(SubMeshComponent, ComponentBase);
