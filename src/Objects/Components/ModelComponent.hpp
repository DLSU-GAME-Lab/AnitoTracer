#pragma once

#include "ComponentBase.hpp"
#include "../Models/ModelManager.hpp"

class ModelComponent : public ComponentBase {
public:
    // Initializes the component with an optional loaded model and owner
    ModelComponent(Model* model = nullptr, gbe::IInstanceManager<HierarchyObject>::Ref owner = {})
        : ComponentBase("ModelComponent", owner), m_pModel(model) {}

    ~ModelComponent() override = default;

    // Delete copy constructor/assignment to prevent object slicing and resource duplication
    ModelComponent(const ModelComponent&) = delete;
    ModelComponent& operator=(const ModelComponent&) = delete;

    // Allow moving for container compatibility
    ModelComponent(ModelComponent&&) = default;
    ModelComponent& operator=(ModelComponent&&) = default;

    // --- Core Model Setters / Getters ---

    void SetModel(Model* model) { m_pModel = model; }
    Model* GetModel() const { return m_pModel; }
    bool HasModel() const { return m_pModel != nullptr; }

    // --- Easy Getters for Model Properties ---

    // Returns true if the model is loaded and contains PBR properties
    bool HasPBRProperties() const;

    // Hardware buffer getters (returns raw pointers safely from Diligent's RefCntAutoPtr)
    Diligent::IBuffer* GetVertexBuffer() const;

    Diligent::IBuffer* GetIndexBuffer() const;

    // Collection getters (returns const references; falls back to static empty vectors if model is null)
    const std::vector<SubMesh>& GetSubMeshes() const;

    const std::vector<PBRMaterial>& GetPBRMaterials() const;

    const std::vector<Diligent::RefCntAutoPtr<Diligent::ITextureView>>& GetMaterials() const;

    const std::vector<Diligent::float4>& GetMaterialColors() const;

private:
    Model* m_pModel;
};