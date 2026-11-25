#pragma once

#include "../../../../Objects/Components/MaterialOverrideComponent.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

// The rows of a MaterialOverrideComponent in the Inspector: a switch and a value for each property it can replace. Shared by
// the Model component (every submesh of the object) and the Submesh component (one part of it).
namespace MaterialOverrideUI {

    // What a surface has without this component's overrides: shown while a row's switch is off, and where the row starts from
    // when it is switched on (so that overriding a property begins at what it was, not at a default).
    struct Inherited {
        float Roughness = 0.5f;
        float Metallic = 0.0f;
        glm::vec3 EmissiveColor{ 1.0f };
        float EmissiveStrength = 1.0f;
        float Alpha = 1.0f;
        float Clearcoat = 0.0f;
        float ClearcoatRoughness = 0.0f;
        glm::vec3 SheenColor{ 0.0f };
        float SheenRoughness = 0.5f;
        float Transmission = 0.0f;
    };

    using Component = MaterialOverrideComponent;

    // Checkbox to enable the override, plus a 0-1 slider that is greyed out while it's off. With `inherited` the slider shows
    // that value while the override is off, and starts from it when the override is switched on.
    inline void DrawFloatOverride(const char* label, Component* component,
        bool (Component::*getEnabled)() const, void (Component::*setEnabled)(bool),
        float (Component::*getValue)() const, void (Component::*setValue)(float),
        const float* inherited = nullptr)
    {
        ImGui::PushID(label);

        bool enabled = (component->*getEnabled)();
        if (ImGui::Checkbox("##Override", &enabled)) {
            (component->*setEnabled)(enabled);
            if (enabled && inherited) (component->*setValue)(*inherited);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Override the material's %s.", label);

        ImGui::SameLine();
        ImGui::BeginDisabled(!enabled);
        float value = (enabled || !inherited) ? (component->*getValue)() : *inherited;
        if (ImGui::SliderFloat(label, &value, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            (component->*setValue)(value);
        ImGui::EndDisabled();

        ImGui::PopID();
    }

    // A 0-1 slider lined up under the slider of DrawFloatOverride, greyed out while that override is off
    inline void DrawIndentedSlider(const char* label, bool enabled, float value, const float* inherited,
        void (Component::*setValue)(float), Component* component)
    {
        ImGui::BeginDisabled(!enabled);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x);
        if (!enabled && inherited) value = *inherited;
        if (ImGui::SliderFloat(label, &value, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            (component->*setValue)(value);
        ImGui::EndDisabled();
    }

    // Checkbox, then a color and an unbounded strength greyed out while it's off
    inline void DrawEmissiveOverride(Component* component, const Inherited* inherited)
    {
        ImGui::PushID("Emissive");

        bool enabled = component->GetOverrideEmissive();
        if (ImGui::Checkbox("##Override", &enabled)) {
            component->SetOverrideEmissive(enabled);
            if (enabled && inherited) {
                component->SetEmissiveColor(inherited->EmissiveColor);
                component->SetEmissiveStrength(inherited->EmissiveStrength);
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Override the material's emissive color and strength.\nEmissive maps still mask the glow.");

        ImGui::SameLine();
        ImGui::BeginDisabled(!enabled);
        glm::vec3 color = (enabled || !inherited) ? component->GetEmissiveColor() : inherited->EmissiveColor;
        if (ImGui::ColorEdit3("Emissive", glm::value_ptr(color)))
            component->SetEmissiveColor(color);

        float strength = (enabled || !inherited) ? component->GetEmissiveStrength() : inherited->EmissiveStrength;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::DragFloat("Strength", &strength, 0.05f, 0.0f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            component->SetEmissiveStrength(strength);
        ImGui::EndDisabled();

        ImGui::PopID();
    }

    // Checkbox, then the sheen color and how wide its rim is, greyed out while it's off
    inline void DrawSheenOverride(Component* component, const Inherited* inherited)
    {
        ImGui::PushID("Sheen");

        bool enabled = component->GetOverrideSheen();
        if (ImGui::Checkbox("##Override", &enabled)) {
            component->SetOverrideSheen(enabled);
            if (enabled && inherited) {
                component->SetSheenColor(inherited->SheenColor);
                component->SetSheenRoughness(inherited->SheenRoughness);
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Override the material's sheen (the soft rim of light on cloth).");

        ImGui::SameLine();
        ImGui::BeginDisabled(!enabled);
        glm::vec3 color = (enabled || !inherited) ? component->GetSheenColor() : inherited->SheenColor;
        if (ImGui::ColorEdit3("Sheen", glm::value_ptr(color)))
            component->SetSheenColor(color);
        ImGui::EndDisabled();

        DrawIndentedSlider("Sheen Roughness", enabled, component->GetSheenRoughness(),
            inherited ? &inherited->SheenRoughness : nullptr, &Component::SetSheenRoughness, component);

        ImGui::PopID();
    }

    inline void DrawHint(const char* text)
    {
        // Wrapped: the Inspector is narrow
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("%s", text);
        ImGui::PopStyleColor();
    }

    // Every override the component has, in the order of the material's own properties
    inline void DrawAll(Component* component, const Inherited* inherited)
    {
        DrawFloatOverride("Roughness", component,
            &Component::GetOverrideRoughness, &Component::SetOverrideRoughness,
            &Component::GetRoughness, &Component::SetRoughness, inherited ? &inherited->Roughness : nullptr);
        DrawFloatOverride("Metallic", component,
            &Component::GetOverrideMetallic, &Component::SetOverrideMetallic,
            &Component::GetMetallic, &Component::SetMetallic, inherited ? &inherited->Metallic : nullptr);
        DrawEmissiveOverride(component, inherited);

        DrawFloatOverride("Alpha", component,
            &Component::GetOverrideAlpha, &Component::SetOverrideAlpha,
            &Component::GetAlpha, &Component::SetAlpha, inherited ? &inherited->Alpha : nullptr);
        if (component->GetOverrideAlpha())
            DrawHint("Drawn as glass. Only the Hybrid and Deferred Hybrid renderers draw it.");

        // Layers over the base material
        DrawFloatOverride("Clearcoat", component,
            &Component::GetOverrideClearcoat, &Component::SetOverrideClearcoat,
            &Component::GetClearcoat, &Component::SetClearcoat, inherited ? &inherited->Clearcoat : nullptr);
        DrawIndentedSlider("Coat Roughness", component->GetOverrideClearcoat(), component->GetClearcoatRoughness(),
            inherited ? &inherited->ClearcoatRoughness : nullptr, &Component::SetClearcoatRoughness, component);
        DrawSheenOverride(component, inherited);

        DrawFloatOverride("Transmission", component,
            &Component::GetOverrideTransmission, &Component::SetOverrideTransmission,
            &Component::GetTransmission, &Component::SetTransmission, inherited ? &inherited->Transmission : nullptr);
        if (component->GetOverrideTransmission())
            DrawHint("Lets the view through, tinted by the base color. Only the Hybrid and Deferred Hybrid renderers show it.");
    }
}
