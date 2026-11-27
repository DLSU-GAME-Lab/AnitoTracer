#pragma once

#include "../../../../Asset/ProjectLoader.hpp"
#include "../../../../Common/LightUnits.hpp"
#include "../../../../Objects/Components/Lights/LightBase.hpp"
#include <imgui.h>

// Shared by the light inspectors
namespace LightUI {

    // True when lights are set in lumens and lux (User Settings, Physical Lights) instead of the plain Intensity
    inline bool UsesRealUnits() {
        const auto& settings = ProjectLoader::GetProjectSettings().physical_lights;
        return settings.enabled && settings.real_units;
    }

    inline void Tooltip(const char* text) {
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s", text);
    }

    // The plain Intensity row, for lights that are not set in real units
    inline void DrawIntensity(LightBase* light, const char* tooltip) {
        float intensity = light->GetIntensity();
        if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f)) {
            light->SetIntensity(intensity);
        }
        Tooltip(tooltip);
    }

    // The row for the size of a light's body
    inline bool DrawRadius(float& radius) {
        const bool changed = ImGui::DragFloat("Radius", &radius, 0.005f, 0.0f, 5.0f, "%.2f m");
        Tooltip("Size of the light's body. A bigger light casts softer shadows and a bigger highlight on glossy surfaces.\n"
            "Needs Light Size and Highlight Shape under User Settings, Physical Lights; without them every light has the old fixed size.");
        return changed;
    }

    inline const char* kIntensityTip =
        "How bright the light is, in scene units: what it adds to a surface facing it at 1 m.\n"
        "Real Units in User Settings, Physical Lights, sets lights in lumens and lux instead.";
}
