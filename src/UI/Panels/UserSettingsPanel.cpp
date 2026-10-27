#include "UserSettingsPanel.hpp"
#include "../../UserSettings.hpp"
#include "../../Asset/ProjectLoader.hpp"
#include "imgui.h"
#include <iostream>

namespace Diligent {

    UserSettingsPanel::UserSettingsPanel(const std::string& name)
        : BasePanel(name)
    {}

    void UserSettingsPanel::Draw()
    {
        // Do not render if the panel is toggled off
        if (!m_IsVisible) return;

        // Begin the ImGui window with the panel's name and visibility state
        if (ImGui::Begin(m_Name.c_str(), &m_IsVisible))
        {
            auto& projectSettings = ProjectLoader::GetProjectSettings();
            auto& lightingSettings = projectSettings.lighting;
            auto saveProjectSettings = [] {
                if (!ProjectLoader::GetCurrentProjectFile().empty() && !ProjectLoader::SaveProject()) {
                    std::cerr << "Failed to save project settings." << std::endl;
                }
            };

            // ------------------------------------------------------------------
            // Project Lighting Settings Section
            // ------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Project Lighting Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushItemWidth(150.0f);

                ImGui::DragFloat("Shadow Bias", &lightingSettings.shadow_bias, 0.0002f, 0.0f, 0.1f, "%.4f");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Epsilon offset applied to ray origin to prevent shadow acne.");
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();

                ImGui::ColorEdit3("Ambient Color", lightingSettings.ambient_color.data());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Color of the constant ambient contribution.");
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();

                ImGui::DragFloat("Ambient Multiplier", &lightingSettings.ambient_multiplier, 0.01f, 0.0f, 10.0f, "%.2f");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Scales the brightness of ambient lighting in the scene.");
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();

                ImGui::PopItemWidth();

                ImGui::Spacing();

                // Convenient Reset Button
                if (ImGui::Button("Reset to Defaults"))
                {
                    lightingSettings = ProjectLightingSettings{};
                    saveProjectSettings();
                }
            }

            if (ImGui::CollapsingHeader("Graphics Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Enable MSAA (4x)", &UserSettings::GetInstance().GetEnableMSAA());
            }

            if (ImGui::CollapsingHeader("Physics Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Checkbox("Enable Gravity", &projectSettings.physics.gravity_enabled)) {
                    saveProjectSettings();
                }
                ImGui::BeginDisabled(!projectSettings.physics.gravity_enabled);
                ImGui::DragFloat3("Gravity", projectSettings.physics.gravity.data(), 0.1f);
                if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();
                ImGui::EndDisabled();
            }

            if (ImGui::CollapsingHeader("Debug Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Checkbox("Show Physics Collider Wireframes", &projectSettings.debug.show_physics_collider_wireframes)) {
                    saveProjectSettings();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Draws Jolt collider shapes as wireframes in the editor viewport. Has no effect in the game view.");
                }
            }

            // Future settings categories (e.g., Graphics, Audio) can be added here
        }
        ImGui::End();
    }

}