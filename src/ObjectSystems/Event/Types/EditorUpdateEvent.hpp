#pragma once

#include "IHandler.hpp"

struct EditorUpdateEvent {
    float deltaTime;
};

namespace gbe {
    // Template specialization for UpdateEvent
    template <>
    class IHandler<EditorUpdateEvent> {
    public:
        virtual ~IHandler() = default;

        // Direct delegate call
        virtual void OnEvent(const EditorUpdateEvent& event) {
            OnEditorUpdate(event.deltaTime);
        }

        virtual void OnEditorUpdate(float deltaTime) = 0;
    };
}