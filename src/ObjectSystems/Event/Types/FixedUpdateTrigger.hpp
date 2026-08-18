#pragma once

#include "ITrigger.hpp"

struct FixedUpdateTrigger {
    float deltaTime;
};

namespace gbe {
    // Template specialization for UpdateEvent
    template <>
    class ITrigger<FixedUpdateTrigger> {
    public:
        virtual ~ITrigger() = default;

        // Direct delegate call
        virtual void OnEvent(const FixedUpdateTrigger& event) {
            OnFixedUpdate(event.deltaTime);
        }

        virtual void OnFixedUpdate(float deltaTime) = 0;
    };
}