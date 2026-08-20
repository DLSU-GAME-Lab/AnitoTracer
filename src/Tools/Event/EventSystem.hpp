#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

namespace gbe {
    // Base class for custom event payload data
    struct EventArgs {
        virtual ~EventArgs() = default;
    };

    // Forward declaration
    class EventHandler;

    // Singleton Event System
    class EventSystem {
    public:
        // Expose types globally so free functions can store their IDs
        using SubscriptionID = uint64_t;
        using EventCallback = std::function<void(const std::unique_ptr<EventArgs>&)>;

        // Global static dispatch method - callable from anywhere
        static void DispatchTo(const std::string& eventName, std::unique_ptr<EventArgs> args) {
            Instance().DispatchInternal(eventName, std::move(args));
        }

        static EventSystem& Instance() {
            static EventSystem instance;
            return instance;
        }

        // Publicly accessible so standalone functions can subscribe without the EventHandler base class
        SubscriptionID Subscribe(const std::string& eventName, EventCallback callback) {
            std::lock_guard<std::mutex> lock(mutex_);
            SubscriptionID id = ++nextID_;
            listeners_[eventName][id] = std::move(callback);
            return id;
        }

        // Publicly accessible so standalone functions can clean up after themselves
        void Unsubscribe(const std::string& eventName, SubscriptionID id) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = listeners_.find(eventName);
            if (it != listeners_.end()) {
                it->second.erase(id);
                if (it->second.empty()) {
                    listeners_.erase(it);
                }
            }
        }

        static SubscriptionID SubscribeTo(const std::string& eventName, EventCallback callback) {
            return Instance().Subscribe(eventName, std::move(callback));
        }

        static void UnsubscribeFrom(const std::string& eventName, SubscriptionID id) {
            Instance().Unsubscribe(eventName, id);
        }

        // Helper template to auto-cast EventArgs for standalone functions/lambdas
        template <typename TArgs, typename F>
        static SubscriptionID SubscribeTyped(const std::string& eventName, F&& callback) {
            return SubscribeTo(eventName, [cb = std::forward<F>(callback)](const std::unique_ptr<EventArgs>& args) {
                if (auto* castedArgs = dynamic_cast<const TArgs*>(args.get())) {
                    cb(castedArgs); // Pass the raw pointer directly to the user's function
                }
                });
        }

    private:
        EventSystem() = default;
        ~EventSystem() = default;
        EventSystem(const EventSystem&) = delete;
        EventSystem& operator=(const EventSystem&) = delete;

        void DispatchInternal(const std::string& eventName, std::unique_ptr<EventArgs> args) {
            std::vector<EventCallback> callbacksToInvoke;

            // Snapshot callbacks under lock to stay thread-safe and avoid re-entrancy deadlocks
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = listeners_.find(eventName);
                if (it != listeners_.end()) {
                    for (const auto& [id, callback] : it->second) {
                        callbacksToInvoke.push_back(callback);
                    }
                }
            }

            // Execute callbacks safely
            for (const auto& cb : callbacksToInvoke) {
                if (cb) {
                    cb(args);
                }
            }
        }

        std::mutex mutex_;
        SubscriptionID nextID_ = 0;
        std::unordered_map<std::string, std::unordered_map<SubscriptionID, EventCallback>> listeners_;
    };
}