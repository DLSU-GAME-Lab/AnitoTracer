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
        if(ImGui::SliderFloat("Volume", &volume, AudioManager::MinVolume, AudioManager::MaxVolume, "%.0f%%", 
            ImGuiSliderFlags_AlwaysClamp))
            audioComponent->SetVolume(volume);

        bool loop = audioComponent->GetLoop();
        if (ImGui::Checkbox("Loop", &loop))
            audioComponent->SetLoop(loop);

        float pitch = audioComponent->GetPitch();
        if(ImGui::SliderFloat("Pitch", &pitch, AudioManager::MinPitch, AudioManager::MaxPitch, "%.2fx", 
            ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp))
            audioComponent->SetPitch(pitch); 

        bool spatialized = audioComponent->GetSpatialized();
        if (ImGui::Checkbox("Spatial Audio", &spatialized))
            audioComponent->SetSpatialized(spatialized);

        float minDistance = audioComponent->GetMinDistance();
        float maxDistance = audioComponent->GetMaxDistance();

        bool minDistanceSlider = ImGui::SliderFloat("Min Distance", &minDistance, AudioManager::MinDistanceLimit, 
            AudioManager::MaxDistanceLimit - AudioManager::DistanceGap, "%.2f", 
            ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
        bool maxDistanceSlider = ImGui::SliderFloat("Max Distance", &maxDistance, minDistance + AudioManager::DistanceGap, 
            AudioManager::MaxDistanceLimit, "%.2f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);

        if (minDistanceSlider || maxDistanceSlider) {
            if (maxDistance < minDistance + AudioManager::DistanceGap) {
                 maxDistance = minDistance + AudioManager::DistanceGap;
            }
            audioComponent->SetDistance(minDistance, maxDistance);
        }

        bool playOnAwake = audioComponent->GetPlayOnAwake();
        if (ImGui::Checkbox("Play on Awake", &playOnAwake))
            audioComponent->SetPlayOnAwake(playOnAwake);
            
    }
};