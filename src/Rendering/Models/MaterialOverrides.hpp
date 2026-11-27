#pragma once

#include <glm/glm.hpp>

// Material properties an object, or one of its submeshes, replaces. Each property has a switch: switched off, the material's
// own value stands. The values are what the shaders get (see ModelRenderInstance, which holds one set for the whole object
// and one per submesh).
struct MaterialOverrides {
    bool OverrideRoughness = false;
    float Roughness = 0.5f;
    bool OverrideMetallic = false;
    float Metallic = 0.0f;
    // Replaces the material's emissive color * strength; the emissive map still applies
    bool OverrideEmissive = false;
    glm::vec3 Emissive{ 1.0f };
    // How many times brighter the light the surface's glow casts on its surroundings is than the glow itself. Only the rays that
    // gather indirect light (per-pixel GI) read it: how the surface looks, and what a reflection of it shows, stay as they are.
    bool OverrideEmissiveLight = false;
    float EmissiveLight = 1.0f;
    // Draws the surface as glass: it is blended with this alpha, whatever its material says
    bool OverrideAlpha = false;
    float Alpha = 1.0f;

    // Layers over the base material, replacing what the material has
    bool OverrideClearcoat = false;
    float Clearcoat = 1.0f;
    float ClearcoatRoughness = 0.05f;
    bool OverrideSheen = false;
    glm::vec3 SheenColor{ 1.0f };
    float SheenRoughness = 0.5f;
    // Lets light through the surface: drawn with the glass (nothing is blended: it is as opaque as 1 - Transmission)
    bool OverrideTransmission = false;
    float Transmission = 0.8f;

    // Lets every property `over` replaces replace this one's, and leaves the others as they are. Used to lay a submesh's
    // overrides over its object's.
    void Apply(const MaterialOverrides& over) {
        if (over.OverrideRoughness) { OverrideRoughness = true; Roughness = over.Roughness; }
        if (over.OverrideMetallic) { OverrideMetallic = true; Metallic = over.Metallic; }
        if (over.OverrideEmissive) { OverrideEmissive = true; Emissive = over.Emissive; }
        if (over.OverrideEmissiveLight) { OverrideEmissiveLight = true; EmissiveLight = over.EmissiveLight; }
        if (over.OverrideAlpha) { OverrideAlpha = true; Alpha = over.Alpha; }
        if (over.OverrideClearcoat) { OverrideClearcoat = true; Clearcoat = over.Clearcoat; ClearcoatRoughness = over.ClearcoatRoughness; }
        if (over.OverrideSheen) { OverrideSheen = true; SheenColor = over.SheenColor; SheenRoughness = over.SheenRoughness; }
        if (over.OverrideTransmission) { OverrideTransmission = true; Transmission = over.Transmission; }
    }

    // True when any property is replaced
    bool Any() const {
        return OverrideRoughness || OverrideMetallic || OverrideEmissive || OverrideEmissiveLight || OverrideAlpha
            || OverrideClearcoat || OverrideSheen || OverrideTransmission;
    }
};
