#pragma once

#include "../EventDefines.hpp"
#include "EventHandler.hpp"

#include "../Args/HierarchyObjectArgs.hpp"

// 3. Create an EventHandler that listens for scene load events
class Print_OnObjectAny : public gbe::EventHandler {
public:
    Print_OnObjectAny() {
        // Subscribe using the defined macro
        this->SubscribeTo(EVENT_ONOBJECTDESTROYED, [](const std::unique_ptr<gbe::EventArgs>& args) {
            // Cast base pointer to our specific event payload
            auto _args = dynamic_cast<const HierarchyObjectArgs*>(args.get());
            if (_args) {
                std::cout << "[HierarchyManager] Object Deleted: " << _args->ref.GetPtr()->GetName() << std::endl;
            }
            });
        this->SubscribeTo(EVENT_ONOBJECTCREATED, [](const std::unique_ptr<gbe::EventArgs>& args) {
            // Cast base pointer to our specific event payload
            auto _args = dynamic_cast<const HierarchyObjectArgs*>(args.get());
            if (_args) {
                std::cout << "[HierarchyManager] Object Created: " << _args->ref.GetPtr()->GetName() << std::endl;
            }
            });
    }
};