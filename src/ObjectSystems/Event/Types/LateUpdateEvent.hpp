#pragma once

#include "IHandler.hpp"

struct LateUpdateEvent {
    float deltaTime;
};

namespace gbe {
    // Template specialization for UpdateEvent
    template <>
    class IHandler<LateUpdateEvent> {
    public:
        virtual ~IHandler() = default;

        // Direct delegate call
        virtual void OnEvent(const LateUpdateEvent& event) {
            OnLateUpdate(event.deltaTime);
        }

        virtual void OnLateUpdate(float deltaTime) = 0;
    };
}