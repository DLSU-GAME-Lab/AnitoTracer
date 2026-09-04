#pragma once

#include "EventSystem.hpp"
#include "Organization/IInstanceManager.hpp"

class HierarchyObject;

// 2. Define custom EventArgs with the "name" payload
struct HierarchyObjectArgs : public gbe::EventArgs {
    gbe::IInstanceManager<HierarchyObject>::Ref ref;

    explicit HierarchyObjectArgs(gbe::IInstanceManager<HierarchyObject>::Ref _ref)
        : ref(_ref) {
    }
};