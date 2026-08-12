#pragma once

#include "IHandler.hpp"

struct FixedUpdateEvent {
    float deltaTime;
};

namespace gbe {
    // Template specialization for UpdateEvent
    template <>
    class IHandler<FixedUpdateEvent> {
    public:
        virtual ~IHandler() = default;

        // Direct delegate call
        virtual void OnEvent(const FixedUpdateEvent& event) {
            OnFixedUpdate(event.deltaTime);
        }

        virtual void OnFixedUpdate(float deltaTime) = 0;
    };
}