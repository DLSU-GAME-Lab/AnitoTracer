#include "UserSettingsPanel.hpp"
#include "../../UserSettings.hpp"
#include "imgui.h"

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
            // Retrieve global user settings
            auto& shadowSettings = UserSettings::GetInstance().GetShadowSettings();

            // ------------------------------------------------------------------
            // Shadow & Lighting Settings Section
            // ------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Shadow & Ambient Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushItemWidth(150.0f);

                // Control for Shadow Bias
                ImGui::DragFloat("Shadow Bias", &shadowSettings.ShadowBias, 0.0002f, 0.0f, 0.1f, "%.4f");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Epsilon offset applied to ray origin to prevent shadow acne.");
                }

                // Control for Ambient Light Multiplier
                ImGui::DragFloat("Ambient Multiplier", &shadowSettings.AmbientMultiplier, 0.01f, 0.0f, 10.0f, "%.2f");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Scales the brightness of ambient lighting in the scene.");
                }

                ImGui::PopItemWidth();

                ImGui::Spacing();

                // Convenient Reset Button
                if (ImGui::Button("Reset to Defaults"))
                {
                    shadowSettings.ShadowBias = 0.015f;
                    shadowSettings.AmbientMultiplier = 1.0f;
                }
            }

            if (ImGui::CollapsingHeader("Graphics Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Enable MSAA (4x)", &UserSettings::GetInstance().GetEnableMSAA());
            }

            // Future settings categories (e.g., Graphics, Audio) can be added here
        }
        ImGui::End();
    }

}