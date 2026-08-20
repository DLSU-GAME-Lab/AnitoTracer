#pragma once

#include "EventHandler.hpp"

namespace gbe {
    // Lightweight RAII wrapper for standalone function subscriptions
    struct ScopedSubscription {
        std::string eventName;
        EventSystem::SubscriptionID id{ 0 }; // Default to 0 (invalid ID)

        // Default constructor
        ScopedSubscription() = default;

        // Main constructor taking the event name and ID
        ScopedSubscription(std::string name, EventSystem::SubscriptionID subId)
            : eventName(std::move(name)), id(subId) {}

        // Destructor automatically unsubscribes
        ~ScopedSubscription() {
            if (id != 0) {
                EventSystem::UnsubscribeFrom(eventName, id);
            }
        }

        // 1. Delete copy operations to prevent double-unsubscription
        ScopedSubscription(const ScopedSubscription&) = delete;
        ScopedSubscription& operator=(const ScopedSubscription&) = delete;

        // 2. Allow move operations for safe transfers
        ScopedSubscription(ScopedSubscription&& other) noexcept
            : eventName(std::move(other.eventName)), id(other.id) {
            other.id = 0; // Nullify the old ID so it doesn't unsubscribe on destruction
        }

        ScopedSubscription& operator=(ScopedSubscription&& other) noexcept {
            if (this != &other) {
                // Unsubscribe from our current event first if we hold one
                if (id != 0) {
                    EventSystem::UnsubscribeFrom(eventName, id);
                }

                // Steal the data from the other object
                eventName = std::move(other.eventName);
                id = other.id;
                other.id = 0;
            }
            return *this;
        }

        //Mainly for below
        template <typename TArgs, typename F>
        static ScopedSubscription Create(const std::string& name, F&& callback) {
            // Automatically registers the event and captures the ID
            auto subId = EventSystem::SubscribeTyped<TArgs>(name, std::forward<F>(callback));

            // Returns the fully constructed RAII wrapper
            return ScopedSubscription(name, subId);
        }

        //EZ sub with just the function
        template <typename TArgs, typename TClass>
        static ScopedSubscription Create(const std::string& name, void (TClass::* memberFunc)(const TArgs*), TClass* instance) {
            // Automatically wraps the member function and the instance pointer with std::bind
            // Function ref, sender, how many params in function 1 == _1
            // Usually only one due to EventArgs-
            return Create<TArgs>(name, std::bind(memberFunc, instance, std::placeholders::_1));
        }
    };
}