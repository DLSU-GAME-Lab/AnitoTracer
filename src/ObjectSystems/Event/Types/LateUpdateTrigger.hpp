#pragma once

#include "ITrigger.hpp"

struct LateUpdateTrigger {
    float deltaTime;
};

namespace gbe {
    // Template specialization for UpdateEvent
    template <>
    class ITrigger<LateUpdateTrigger> {
    public:
        virtual ~ITrigger() = default;

        // Direct delegate call
        virtual void OnEvent(const LateUpdateTrigger& event) {
            OnLateUpdate(event.deltaTime);
        }

        virtual void OnLateUpdate(float deltaTime) = 0;
    };
}