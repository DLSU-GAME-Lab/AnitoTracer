#pragma once

#include "EventSystem.hpp"

namespace gbe {
    // Base class for objects that handle events
    class EventHandler {
    public:
        virtual ~EventHandler() {
            // Automatically unregisters all active subscriptions when this object dies
            for (const auto& sub : subscriptions_) {
                EventSystem::Instance().Unsubscribe(sub.eventName, sub.id);
            }
        }

    protected:
        using Callback = std::function<void(const std::unique_ptr<EventArgs>&)>;

        // Call this inside any derived EventHandler class
        void SubscribeTo(const std::string& eventName, Callback callback) {
            EventSystem::SubscriptionID id = EventSystem::Instance().Subscribe(eventName, std::move(callback));
            subscriptions_.push_back({ eventName, id });
        }

    private:
        struct SubscriptionRecord {
            std::string eventName;
            EventSystem::SubscriptionID id;
        };

        std::vector<SubscriptionRecord> subscriptions_;
    };
}