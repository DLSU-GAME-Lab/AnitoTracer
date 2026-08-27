#pragma once

#include "ITrigger.hpp"

struct OnGUI_Release {
    float deltaTime;
};

namespace gbe {
    // Template specialization for release-mode per-frame GUI event
    template <>
    class ITrigger<OnGUI_Release> {
    public:
        virtual ~ITrigger() = default;

        // Direct delegate call
        virtual void OnEvent(const OnGUI_Release& event) {
            OnGUI_ReleaseEvent(event.deltaTime);
        }

        virtual void OnGUI_ReleaseEvent(float deltaTime) = 0;
    };
}