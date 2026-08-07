#include "ModelComponent.hpp"

// Returns true if the model is loaded and contains PBR properties
inline bool ModelComponent::HasPBRProperties() const {
    return !this->m_model.IsEmpty() ? m_model.Get()->HasPBRProperties : false;
}

// Hardware buffer getters (returns raw pointers safely from Diligent's RefCntAutoPtr)
inline Diligent::IBuffer* ModelComponent::GetVertexBuffer() const {
    return !this->m_model.IsEmpty() ? m_model.Get()->pVertexBuffer : nullptr;
}

inline Diligent::IBuffer* ModelComponent::GetIndexBuffer() const {
    return !this->m_model.IsEmpty() ? m_model.Get()->pIndexBuffer : nullptr;
}

// Collection getters (returns const references; falls back to static empty vectors if model is null)
inline const std::vector<SubMesh>& ModelComponent::GetSubMeshes() const {
    static const std::vector<SubMesh> empty;
    return !this->m_model.IsEmpty() ? m_model.Get()->SubMeshes : empty;
}

inline const std::vector<PBRMaterial>& ModelComponent::GetPBRMaterials() const {
    static const std::vector<PBRMaterial> empty;
    return !this->m_model.IsEmpty() ? m_model.Get()->PBRMaterials : empty;
}

inline const std::vector<Diligent::RefCntAutoPtr<Diligent::ITextureView>>& ModelComponent::GetMaterials() const {
    static const std::vector<Diligent::RefCntAutoPtr<Diligent::ITextureView>> empty;
    return !this->m_model.IsEmpty() ? m_model.Get()->Materials : empty;
}

inline const std::vector<Diligent::float4>& ModelComponent::GetMaterialColors() const {
    static const std::vector<Diligent::float4> empty;
    return !this->m_model.IsEmpty() ? m_model.Get()->MaterialColors : empty;
}
