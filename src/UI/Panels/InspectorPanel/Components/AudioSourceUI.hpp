#pragma once

#include "IComponentUI.hpp"
#include "../../../../Objects/Components/Audio/AudioSourceComponent.hpp"
#include "../../../PropertyDrawers/asset_drawer.hpp"
#include <imgui.h>

class AudioSourceUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        auto* audioComponent = static_cast<AudioSourceComponent*>(component);
        auto& clipReference = audioComponent->GetAudioClipRef();

        if (!ImGui::CollapsingHeader("Audio Source Component", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        gbe::PropertyDrawer<gbe::AssetRef<AudioClip>>::Draw("Audio Clip", clipReference);

        if (ImGui::Button("Play"))
            audioComponent->Play();

        if (ImGui::Button("Stop"))
            audioComponent->Stop();

        float volume = audioComponent->GetVolume();
        if(ImGui::SliderFloat("Volume", &volume, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp))
            audioComponent->SetVolume(volume);
    }
};