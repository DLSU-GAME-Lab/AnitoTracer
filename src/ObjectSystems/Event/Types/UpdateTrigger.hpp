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

		bool started = false;

        // Direct delegate call
        virtual void OnEvent(const UpdateTrigger& event) {
            OnUpdate(event.deltaTime);

            if(!started) {
                OnStart();
                started = true;
            }
        }

        virtual void OnUpdate(float deltaTime) = 0;
        virtual void OnStart() {};
    };
}