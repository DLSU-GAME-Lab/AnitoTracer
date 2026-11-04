#pragma once

#include <imgui.h>
#include "PropertyDrawer.hpp"
#include "AudioManager.hpp"

namespace gbe {

    template <>
    struct PropertyDrawer<AudioManager::AudioGroupType> {
        static bool Draw(const std::string& label, AudioManager::AudioGroupType& target) {
            static const char* names[] = { "BGM", "SFX" };
            int current = static_cast<int>(target);

            bool changed = ImGui::Combo(label.c_str(), &current, names, IM_ARRAYSIZE(names));
            if (changed) target = static_cast<AudioManager::AudioGroupType>(current);
            return changed;
        }
    };
}
