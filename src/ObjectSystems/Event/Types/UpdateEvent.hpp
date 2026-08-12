#pragma once

#include "IHandler.hpp"

struct UpdateEvent {
    float deltaTime;
};

namespace gbe {
    // Template specialization for UpdateEvent
    template <>
    class IHandler<UpdateEvent> {
    public:
        virtual ~IHandler() = default;

        // Direct delegate call
        virtual void OnEvent(const UpdateEvent& event) {
            OnUpdate(event.deltaTime);
        }

        virtual void OnUpdate(float deltaTime) = 0;
    };
}