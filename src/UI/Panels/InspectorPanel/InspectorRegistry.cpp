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
            for (gbe::IAutoSerializer* prop : component->properties) {
                if (prop->m_id != "m_name")
                    prop->DrawInspector();
            }
        }
    }
}
