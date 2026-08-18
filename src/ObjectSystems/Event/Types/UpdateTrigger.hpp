#pragma once

#include "ITrigger.hpp"

struct UpdateTrigger {
    float deltaTime;
};

namespace gbe {
    // Template specialization for UpdateTrigger
    template <>
    class ITrigger<UpdateTrigger> {
    public:
        virtual ~ITrigger() = default;

        // Direct delegate call
        virtual void OnEvent(const UpdateTrigger& event) {
            OnUpdate(event.deltaTime);
        }

        virtual void OnUpdate(float deltaTime) = 0;
    };
}