#pragma once

#include "ITrigger.hpp"

struct EditorUpdateTrigger {
    float deltaTime;
};

namespace gbe {
    // Template specialization for UpdateEvent
    template <>
    class ITrigger<EditorUpdateTrigger> {
    public:
        virtual ~ITrigger() = default;

        // Direct delegate call
        virtual void OnEvent(const EditorUpdateTrigger& event) {
            OnEditorUpdate(event.deltaTime);
        }

        virtual void OnEditorUpdate(float deltaTime) = 0;
    };
}