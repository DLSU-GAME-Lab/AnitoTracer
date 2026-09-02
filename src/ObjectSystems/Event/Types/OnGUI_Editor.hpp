#pragma once

#include "ITrigger.hpp"

struct OnGUI_Editor {
    float deltaTime;
};

namespace gbe {
    // Template specialization for editor-mode per-frame GUI event
    template <>
    class ITrigger<OnGUI_Editor> {
    public:
        virtual ~ITrigger() = default;

        // Direct delegate call
        virtual void OnEvent(const OnGUI_Editor& event) {
            OnGUI_EditorEvent(event.deltaTime);
        }

        virtual void OnGUI_EditorEvent(float deltaTime) = 0;
    };
}