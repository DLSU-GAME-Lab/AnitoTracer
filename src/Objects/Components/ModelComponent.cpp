#include "ModelComponent.hpp"

// Returns true if the model is loaded and contains PBR properties
inline bool ModelComponent::HasPBRProperties() const {
    return m_pModel ? m_pModel->HasPBRProperties : false;
}

// Hardware buffer getters (returns raw pointers safely from Diligent's RefCntAutoPtr)
inline Diligent::IBuffer* ModelComponent::GetVertexBuffer() const {
    return m_pModel ? m_pModel->pVertexBuffer : nullptr;
}

inline Diligent::IBuffer* ModelComponent::GetIndexBuffer() const {
    return m_pModel ? m_pModel->pIndexBuffer : nullptr;
}

// Collection getters (returns const references; falls back to static empty vectors if model is null)
inline const std::vector<SubMesh>& ModelComponent::GetSubMeshes() const {
    static const std::vector<SubMesh> empty;
    return m_pModel ? m_pModel->SubMeshes : empty;
}

inline const std::vector<PBRMaterial>& ModelComponent::GetPBRMaterials() const {
    static const std::vector<PBRMaterial> empty;
    return m_pModel ? m_pModel->PBRMaterials : empty;
}

inline const std::vector<Diligent::RefCntAutoPtr<Diligent::ITextureView>>& ModelComponent::GetMaterials() const {
    static const std::vector<Diligent::RefCntAutoPtr<Diligent::ITextureView>> empty;
    return m_pModel ? m_pModel->Materials : empty;
}

inline const std::vector<Diligent::float4>& ModelComponent::GetMaterialColors() const {
    static const std::vector<Diligent::float4> empty;
    return m_pModel ? m_pModel->MaterialColors : empty;
}
