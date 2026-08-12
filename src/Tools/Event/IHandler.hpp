#pragma once

namespace gbe {

    // Base Handler Interface using the CRTP/Template pattern
    template <typename TEvent>
    class IHandler {
    public:
        virtual ~IHandler() = default;
        virtual void OnEvent(const TEvent& event) = 0;
    };
}