#pragma once

#include <unordered_map>
#include <typeindex>
#include <memory>
#include "Components/IComponentUI.hpp"
#include "../../../Objects/Components/ComponentBase.hpp"
#include "imgui.h"

class InspectorRegistry {
public:
    static InspectorRegistry& GetInstance() {
        static InspectorRegistry instance;
        return instance;
    }

    // Associates a specific Component type with its dedicated UI drawer.
    template<typename TComponent, typename TUI>
    void RegisterUI() {
        m_UIMap[std::type_index(typeid(TComponent))] = std::make_unique<TUI>();
    }

    // Looks up the correct UI drawer for the component and executes it.
    void DrawComponent(ComponentBase* component);

private:
    InspectorRegistry() = default;
    ~InspectorRegistry() = default;

    std::unordered_map<std::type_index, std::unique_ptr<IComponentUI>> m_UIMap;
};