#pragma once

#include "ComponentBase.hpp"
#include "Models/ModelManager.hpp"
#include "AssetRef.hpp"

class ModelComponent : public ComponentBase {
public:
    // Initializes the component with an optional loaded model and owner
    ModelComponent(gbe::AssetRef<Model> model = gbe::AssetRef<Model>(), gbe::IInstanceManager<HierarchyObject>::Ref owner = {})
        : ComponentBase("ModelComponent", owner), m_model(model) {}

    ~ModelComponent() override = default;

    // Delete copy constructor/assignment to prevent object slicing and resource duplication
    ModelComponent(const ModelComponent&) = delete;
    ModelComponent& operator=(const ModelComponent&) = delete;

    // Allow moving for container compatibility
    ModelComponent(ModelComponent&&) = default;
    ModelComponent& operator=(ModelComponent&&) = default;

    // --- Core Model Setters / Getters ---

    void SetModel(gbe::AssetRef<Model> model) { m_model = model; }
    gbe::AssetRef<Model> GetModel() const { return m_model; }
    gbe::AssetRef<Model>& GetModelRef() { return m_model; }
    bool HasModel() const { return !m_model.IsEmpty(); }

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

    // --- Material Overrides (applied to every submesh, replacing the material and texture values) ---

    void SetOverrideRoughness(bool enabled) { m_overrideRoughness = enabled; }
    bool GetOverrideRoughness() const { return m_overrideRoughness; }
    void SetRoughness(float roughness) { m_roughness = roughness; }
    float GetRoughness() const { return m_roughness; }

    void SetOverrideMetallic(bool enabled) { m_overrideMetallic = enabled; }
    bool GetOverrideMetallic() const { return m_overrideMetallic; }
    void SetMetallic(float metallic) { m_metallic = metallic; }
    float GetMetallic() const { return m_metallic; }

    // Replaces the material's emissive color and strength; the emissive map (if any) still masks it
    void SetOverrideEmissive(bool enabled) { m_overrideEmissive = enabled; }
    bool GetOverrideEmissive() const { return m_overrideEmissive; }
    void SetEmissiveColor(const glm::vec3& color) { m_emissiveColor = color; }
    const glm::vec3& GetEmissiveColor() const { return m_emissiveColor; }
    void SetEmissiveStrength(float strength) { m_emissiveStrength = strength; }
    float GetEmissiveStrength() const { return m_emissiveStrength; }

private:
    gbe::AssetRef<Model> m_model;
    GBE_SERIALIZE_FIELD(m_model);

    bool m_overrideRoughness = false;
    GBE_SERIALIZE_FIELD_W_NAME(m_overrideRoughness, "Override Roughness");

    float m_roughness = 0.5f;
    GBE_SERIALIZE_FIELD_W_NAME(m_roughness, "Roughness");

    bool m_overrideMetallic = false;
    GBE_SERIALIZE_FIELD_W_NAME(m_overrideMetallic, "Override Metallic");

    float m_metallic = 0.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_metallic, "Metallic");

    bool m_overrideEmissive = false;
    GBE_SERIALIZE_FIELD_W_NAME(m_overrideEmissive, "Override Emissive");

    glm::vec3 m_emissiveColor = glm::vec3(1.0f);
    GBE_SERIALIZE_FIELD_W_NAME(m_emissiveColor, "Emissive Color");

    float m_emissiveStrength = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_emissiveStrength, "Emissive Strength");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(ModelComponent, ComponentBase);

};

GBE_REGISTER_SERIALIZED_TYPE(ModelComponent, ComponentBase);