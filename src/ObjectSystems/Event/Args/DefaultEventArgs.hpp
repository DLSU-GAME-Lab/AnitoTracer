#pragma once

#include "EventSystem.hpp"

namespace gbe {
    // A generic, empty event argument struct for events with no payload
    struct DefaultEventArgs : public EventArgs {
        virtual ~DefaultEventArgs() = default;
    };
}