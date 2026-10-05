#pragma once

#include "IComponentUI.hpp"
#include "ScriptComponent.hpp"

#include <algorithm>
#include <imgui.h>

class ScriptComponentUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        auto* script = static_cast<ScriptComponent*>(component);
        script->Refresh();

        const std::string header = script->GetScriptName() + " (Script)";
        if (!ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            return;

        if (script->IsScriptMissing()) {
            ImGui::TextDisabled("Script '%s' not found in project Scripts folder.", script->GetScriptName().c_str());
        }

        const std::vector<std::string> hidden = script->GetHiddenProperties();
        for (gbe::IAutoSerializer* prop : script->properties) {
            if (prop->m_id == "m_name") continue;
            if (std::find(hidden.begin(), hidden.end(), prop->m_id) != hidden.end()) continue;
            prop->DrawInspector();
        }
    }
};
