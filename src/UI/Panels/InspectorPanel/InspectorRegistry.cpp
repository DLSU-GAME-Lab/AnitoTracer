#include "InspectorRegistry.hpp"

#include "imgui.h"

// Looks up the correct UI drawer for the component and executes it.
void InspectorRegistry::DrawComponent(ComponentBase* component) {
    if (!component) return;

    auto it = m_UIMap.find(std::type_index(typeid(*component)));
    if (it != m_UIMap.end()) {
        it->second->Draw(component);
    }
    else {
        if (ImGui::CollapsingHeader(component->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            const std::vector<std::string> hidden = component->GetHiddenProperties();
            for (gbe::IAutoSerializer* prop : component->properties) {
                if (prop->m_id == "m_name") continue;
                if (std::find(hidden.begin(), hidden.end(), prop->m_id) != hidden.end()) continue;
                    prop->DrawInspector();
            }
        }
    }
}
