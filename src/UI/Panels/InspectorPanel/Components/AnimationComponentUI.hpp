#pragma once

#include "IComponentUI.hpp"
#include "../../../../Objects/Components/Animation/AnimationComponent.hpp"
#include "../../../PropertyDrawers/asset_drawer.hpp"

#include <imgui.h>

class AnimationComponentUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        auto* animation = static_cast<AnimationComponent*>(component);
        if (!ImGui::CollapsingHeader("Animation Component", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        bool changed = gbe::PropertyDrawer<gbe::AssetRef<AnimationClip>>::Draw(
            "Animation Clip", animation->GetAnimationClipRef());
        if (changed) animation->SetAnimationClip(animation->GetAnimationClipRef());

        bool playOnAwake = animation->GetPlayOnAwake();
        if (ImGui::Checkbox("Play On Awake", &playOnAwake))
            animation->SetPlayOnAwake(playOnAwake);

        bool loop = animation->GetLoop();
        if (ImGui::Checkbox("Loop", &loop))
            animation->SetLoop(loop);

        float speed = animation->GetSpeed();
        if (ImGui::DragFloat("Speed", &speed, 0.01f, 0.0f, 10.0f))
            animation->SetSpeed(speed);

        ImGui::BeginDisabled(!animation->HasAnimationClip());
        if (ImGui::Button("Play")) animation->Play();
        ImGui::SameLine();
        if (ImGui::Button("Pause")) animation->Pause();
        ImGui::SameLine();
        if (ImGui::Button("Resume")) animation->Resume();
        ImGui::SameLine();
        if (ImGui::Button("Stop")) animation->Stop();
        ImGui::EndDisabled();

        const float duration = animation->GetDuration();
        const float progress = duration > 0.0f ? animation->GetTime() / duration : 0.0f;
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f),
            animation->IsPlaying() ? "Playing" : (animation->IsPaused() ? "Paused" : "Stopped"));
        ImGui::TextDisabled("%.3f / %.3f seconds", animation->GetTime(), duration);
    }
};
