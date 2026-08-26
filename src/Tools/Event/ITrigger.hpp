#pragma once

namespace gbe {

    // Base Handler Interface using the CRTP/Template pattern
    template <typename TTrigger>
    class ITrigger {
    public:
        virtual ~ITrigger() = default;
        virtual void OnTrigger(const TTrigger& trigger) = 0;
		virtual void OnEvent(const TTrigger& trigger) { OnTrigger(trigger); } // Default to OnTrigger
    };
}