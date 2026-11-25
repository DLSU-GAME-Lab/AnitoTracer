#include "MaterialOverrideComponent.hpp"

#include <algorithm>

MaterialOverrides MaterialOverrideComponent::GetOverrides() const {
    MaterialOverrides overrides;
    overrides.OverrideRoughness = m_overrideRoughness;
    overrides.Roughness = m_roughness;
    overrides.OverrideMetallic = m_overrideMetallic;
    overrides.Metallic = m_metallic;
    overrides.OverrideEmissive = m_overrideEmissive;
    overrides.Emissive = m_emissiveColor * m_emissiveStrength;
    overrides.OverrideEmissiveLight = m_overrideEmissiveLight;
    overrides.EmissiveLight = std::max(m_emissiveLight, 0.0f);
    overrides.OverrideAlpha = m_overrideAlpha;
    overrides.Alpha = std::clamp(m_alpha, 0.0f, 1.0f);

    overrides.OverrideClearcoat = m_overrideClearcoat;
    overrides.Clearcoat = std::clamp(m_clearcoat, 0.0f, 1.0f);
    overrides.ClearcoatRoughness = std::clamp(m_clearcoatRoughness, 0.0f, 1.0f);
    overrides.OverrideSheen = m_overrideSheen;
    overrides.SheenColor = glm::max(m_sheenColor, glm::vec3(0.0f));
    overrides.SheenRoughness = std::clamp(m_sheenRoughness, 0.0f, 1.0f);
    overrides.OverrideTransmission = m_overrideTransmission;
    overrides.Transmission = std::clamp(m_transmission, 0.0f, 1.0f);
    return overrides;
}

bool MaterialOverrideComponent::HasAnyOverride() const {
    return m_overrideRoughness || m_overrideMetallic || m_overrideEmissive || m_overrideEmissiveLight || m_overrideAlpha
        || m_overrideClearcoat || m_overrideSheen || m_overrideTransmission;
}

void MaterialOverrideComponent::ClearOverrides() {
    m_overrideRoughness = false;
    m_overrideMetallic = false;
    m_overrideEmissive = false;
    m_overrideEmissiveLight = false;
    m_overrideAlpha = false;
    m_overrideClearcoat = false;
    m_overrideSheen = false;
    m_overrideTransmission = false;
}
