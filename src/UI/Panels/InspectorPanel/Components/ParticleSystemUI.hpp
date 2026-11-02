#pragma once

#include "IComponentUI.hpp"
#include "../../../../Objects/Components/Particles/ParticleSystemComponent.hpp"
#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

class ParticleSystemUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        auto* ps = static_cast<ParticleSystemComponent*>(component);

        if (!ImGui::CollapsingHeader("Particle System", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        // --- Playback ---
        if (ImGui::Button("Play")) ps->Play();
        ImGui::SameLine();
        if (ImGui::Button("Stop")) ps->Stop();
        ImGui::SameLine();
        if (ImGui::Button("Clear")) ps->Clear();
        ImGui::SameLine();
        ImGui::TextDisabled("%s | %zu alive", ps->m_playing ? "Playing" : "Stopped", ps->GetAliveCount());

        ImGui::Checkbox("Playing", &ps->m_playing);
        const char* stopBehaviors[] = { "Destroy All", "Nothing" };
        ImGui::Combo("Stop Behavior", &ps->m_stopBehavior, stopBehaviors, IM_ARRAYSIZE(stopBehaviors));
        const char* destroyBehaviors[] = { "Destroy particles too", "Let Decay" };
        ImGui::Combo("Destroy Behavior", &ps->m_destroyBehavior, destroyBehaviors, IM_ARRAYSIZE(destroyBehaviors));
        ImGui::Checkbox("Loop", &ps->m_loop);
        ImGui::DragFloat("Duration (s)", &ps->m_duration, 0.1f, 0.0f, 600.0f);

        // --- Emission ---
        if (ImGui::TreeNodeEx("Emission", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragInt("Max Particles", &ps->m_maxParticles, 1.0f, 0, 100000);
            ImGui::DragFloat("Rate (/s)", &ps->m_emissionRate, 0.5f, 0.0f, 10000.0f);
            ImGui::DragInt("Burst Count", &ps->m_burstCount, 1.0f, 0, 10000);
            ImGui::DragFloat3("Spawn Box Size", &ps->m_spawnBoxSize.x, 0.05f, 0.0f, 1000.0f);
            ImGui::TreePop();
        }

        // --- Initial velocity ---
        if (ImGui::TreeNodeEx("Velocity", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Start Velocity", &ps->m_startVelocity.x, 0.05f);
            ImGui::DragFloat3("Randomness (+/-)", &ps->m_velocityRandomness.x, 0.05f, 0.0f, 1000.0f);
            ImGui::Checkbox("Align To Local Axes", &ps->m_velocityLocal);
            ImGui::DragFloat3("Acceleration", &ps->m_gravity.x, 0.05f);
            ImGui::Checkbox("World Space Simulation", &ps->m_worldSpace);
            ImGui::TreePop();
        }

        // --- Lifetime / look over time ---
        if (ImGui::TreeNodeEx("Particle", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloatRange2("Lifetime (s)", &ps->m_lifetimeMin, &ps->m_lifetimeMax, 0.05f, 0.01f, 600.0f);
            ImGui::DragFloatRange2("Start Size", &ps->m_startSizeMin, &ps->m_startSizeMax, 0.01f, 0.0f, 1000.0f);
            ImGui::DragFloat("End Size Scale", &ps->m_endSizeScale, 0.01f, 0.0f, 100.0f);
            ImGui::DragFloatRange2("Start Rotation (deg)", &ps->m_startRotationMin, &ps->m_startRotationMax, 1.0f, -360.0f, 360.0f);
            ImGui::DragFloat("Rotation Speed (deg/s)", &ps->m_rotationSpeed, 1.0f);
            ImGui::ColorEdit4("Start Color", &ps->m_startColor.x);
            ImGui::ColorEdit4("End Color", &ps->m_endColor.x);
            ImGui::TreePop();
        }

        // --- Renderer ---
        if (ImGui::TreeNodeEx("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* blendModes[] = { "Alpha Blend", "Additive" };
            ImGui::Combo("Blend Mode", &ps->m_blendMode, blendModes, IM_ARRAYSIZE(blendModes));

            DrawTexturePicker(ps);

            ImGui::DragInt("Columns", &ps->m_flipbookColumns, 1.0f, 1, 64);
            ImGui::DragInt("Rows", &ps->m_flipbookRows, 1.0f, 1, 64);
            ImGui::DragInt("Frame Count (0 = all)", &ps->m_flipbookFrameCount, 1.0f, 0, 4096);
            ImGui::DragFloat("Flipbook FPS (0 = over lifetime)", &ps->m_flipbookFps, 0.5f, 0.0f, 240.0f);
            if (ps->m_flipbookFps > 0.0f)
                ImGui::Checkbox("Flipbook Loop", &ps->m_flipbookLoop);

            ps->m_flipbookColumns = std::max(ps->m_flipbookColumns, 1);
            ps->m_flipbookRows = std::max(ps->m_flipbookRows, 1);
            ps->m_lifetimeMax = std::max(ps->m_lifetimeMax, ps->m_lifetimeMin);
            ps->m_startSizeMax = std::max(ps->m_startSizeMax, ps->m_startSizeMin);
            ps->m_startRotationMax = std::max(ps->m_startRotationMax, ps->m_startRotationMin);

            if (ps->m_texture) {
                ImGui::Spacing();
                ImGui::Image(reinterpret_cast<ImTextureID>(ps->m_texture), ImVec2(128.0f, 128.0f));
            }
            ImGui::TreePop();
        }
    }

private:
    static std::vector<std::string>& TextureList() {
        static std::vector<std::string> textures;
        return textures;
    }

    static void RefreshTextureList() {
        auto& textures = TextureList();
        textures.clear();

        std::error_code ec;
        const std::filesystem::path root("Assets");
        if (!std::filesystem::exists(root, ec)) return;

        for (std::filesystem::recursive_directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
            if (!it->is_regular_file(ec)) continue;

            std::string ext = it->path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp") {
                textures.push_back(it->path().generic_string());
            }
        }
        std::sort(textures.begin(), textures.end());
    }

    static void DrawTexturePicker(ParticleSystemComponent* ps) {
        static bool listLoaded = false;
        if (!listLoaded) {
            RefreshTextureList();
            listLoaded = true;
        }

        std::string& path = ps->m_texturePath;
        const std::string preview = path.empty() ? "None (white)" : path;

        if (ImGui::BeginCombo("Texture", preview.c_str())) {
            if (ImGui::Selectable("None (white)", path.empty()))
                path.clear();

            for (const std::string& texture : TextureList()) {
                if (ImGui::Selectable(texture.c_str(), texture == path))
                    path = texture;
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        if (ImGui::SmallButton("Refresh"))
            RefreshTextureList();

        char buffer[260];
        std::snprintf(buffer, sizeof(buffer), "%s", path.c_str());
        if (ImGui::InputText("Texture Path", buffer, sizeof(buffer)))
            path = buffer;
    }
};
