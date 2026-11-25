#pragma once

#include "ComponentBase.hpp"
#include "Models/MaterialOverrides.hpp"

#include <glm/glm.hpp>

// The material properties an object, or one part of it, replaces. Each property has a switch (Override ...) and a value; a
// property whose switch is off keeps the material's own value. Shared by ModelComponent, which overrides them for every
// submesh of the object, and SubMeshComponent, which overrides them for one submesh (laid over what the object overrides).
class MaterialOverrideComponent : public ComponentBase {
public:
    MaterialOverrideComponent(const std::string& name, gbe::IInstanceManager<HierarchyObject>::Ref owner = {})
        : ComponentBase(name, owner) {}

    ~MaterialOverrideComponent() override = default;

    MaterialOverrideComponent(const MaterialOverrideComponent&) = delete;
    MaterialOverrideComponent& operator=(const MaterialOverrideComponent&) = delete;
    MaterialOverrideComponent(MaterialOverrideComponent&&) = default;
    MaterialOverrideComponent& operator=(MaterialOverrideComponent&&) = default;

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

    // How many times brighter the light the surface's glow casts on its surroundings is than the glow looks (1: as bright).
    // Only the rays that gather indirect light see it (the Deferred Hybrid renderer's per-pixel GI), so the surface and its
    // reflections look the same. It scales whatever glow there is: the material's own, or the emissive override above.
    void SetOverrideEmissiveLight(bool enabled) { m_overrideEmissiveLight = enabled; }
    bool GetOverrideEmissiveLight() const { return m_overrideEmissiveLight; }
    void SetEmissiveLight(float multiplier) { m_emissiveLight = multiplier; }
    float GetEmissiveLight() const { return m_emissiveLight; }

    // Draws the surface as glass: it is blended with this alpha (0 clear, 1 solid), whatever its material says
    void SetOverrideAlpha(bool enabled) { m_overrideAlpha = enabled; }
    bool GetOverrideAlpha() const { return m_overrideAlpha; }
    void SetAlpha(float alpha) { m_alpha = alpha; }
    float GetAlpha() const { return m_alpha; }

    // Layers over the base material: a clear coat, a sheen of cloth fibres, and a body that lets the view through.
    // Each replaces what the material has (see ModelRenderInstance::ResolveLayers).
    void SetOverrideClearcoat(bool enabled) { m_overrideClearcoat = enabled; }
    bool GetOverrideClearcoat() const { return m_overrideClearcoat; }
    void SetClearcoat(float strength) { m_clearcoat = strength; }
    float GetClearcoat() const { return m_clearcoat; }
    void SetClearcoatRoughness(float roughness) { m_clearcoatRoughness = roughness; }
    float GetClearcoatRoughness() const { return m_clearcoatRoughness; }

    void SetOverrideSheen(bool enabled) { m_overrideSheen = enabled; }
    bool GetOverrideSheen() const { return m_overrideSheen; }
    void SetSheenColor(const glm::vec3& color) { m_sheenColor = color; }
    const glm::vec3& GetSheenColor() const { return m_sheenColor; }
    void SetSheenRoughness(float roughness) { m_sheenRoughness = roughness; }
    float GetSheenRoughness() const { return m_sheenRoughness; }

    void SetOverrideTransmission(bool enabled) { m_overrideTransmission = enabled; }
    bool GetOverrideTransmission() const { return m_overrideTransmission; }
    void SetTransmission(float transmission) { m_transmission = transmission; }
    float GetTransmission() const { return m_transmission; }

    // The properties this component replaces, in the form the renderer takes them (values clamped to what the shaders accept)
    MaterialOverrides GetOverrides() const;

    // True when this component replaces any property
    bool HasAnyOverride() const;

    // Switches every override off. The values are kept, so switching one back on finds what was typed.
    void ClearOverrides();

private:
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

    bool m_overrideEmissiveLight = false;
    GBE_SERIALIZE_FIELD_W_NAME(m_overrideEmissiveLight, "Override Emissive Light");

    float m_emissiveLight = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_emissiveLight, "Emissive Light");

    bool m_overrideAlpha = false;
    GBE_SERIALIZE_FIELD_W_NAME(m_overrideAlpha, "Override Alpha");

    float m_alpha = 0.5f;
    GBE_SERIALIZE_FIELD_W_NAME(m_alpha, "Alpha");

    bool m_overrideClearcoat = false;
    GBE_SERIALIZE_FIELD_W_NAME(m_overrideClearcoat, "Override Clearcoat");

    float m_clearcoat = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_clearcoat, "Clearcoat");

    float m_clearcoatRoughness = 0.05f;
    GBE_SERIALIZE_FIELD_W_NAME(m_clearcoatRoughness, "Clearcoat Roughness");

    bool m_overrideSheen = false;
    GBE_SERIALIZE_FIELD_W_NAME(m_overrideSheen, "Override Sheen");

    glm::vec3 m_sheenColor = glm::vec3(1.0f);
    GBE_SERIALIZE_FIELD_W_NAME(m_sheenColor, "Sheen Color");

    float m_sheenRoughness = 0.5f;
    GBE_SERIALIZE_FIELD_W_NAME(m_sheenRoughness, "Sheen Roughness");

    bool m_overrideTransmission = false;
    GBE_SERIALIZE_FIELD_W_NAME(m_overrideTransmission, "Override Transmission");

    float m_transmission = 0.8f;
    GBE_SERIALIZE_FIELD_W_NAME(m_transmission, "Transmission");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(MaterialOverrideComponent, ComponentBase);
};
