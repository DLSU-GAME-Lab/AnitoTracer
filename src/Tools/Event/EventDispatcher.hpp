#pragma once

#include "IHandler.hpp"
#include <type_traits>

namespace gbe {
    class EventDispatcher {
    public:
        // Overload 1: Dispatch with an existing event object
        template <typename TEvent, typename TComponent>
        static void Dispatch(TComponent* component, const TEvent& event) {
            if (!component) return;

            using RawComponent = std::decay_t<TComponent>;
            using RawEvent = std::decay_t<TEvent>;

            using TargetHandler = gbe::IHandler<RawEvent>;

            // Path A: Direct compile-time match
            if constexpr (std::is_base_of_v<TargetHandler, RawComponent>) {
                static_cast<TargetHandler*>(component)->OnEvent(event);
            }
            // Path B: Runtime check for polymorphic base pointers (e.g. BaseComponent*)
            else if constexpr (std::is_polymorphic_v<RawComponent>) {
                if (auto* handler = dynamic_cast<TargetHandler*>(component)) {
                    handler->OnEvent(event);
                }
            }
            // Path C: Non-polymorphic non-handler type -> Silent No-Op
        }

        // Overload 2: Construct the TEvent in-place on dispatch
        template <typename TEvent, typename TComponent, typename... Args>
        static void Dispatch(TComponent* component, Args&&... args) {
            Dispatch<TEvent>(component, TEvent{ std::forward<Args>(args)... });
        }
    };
}