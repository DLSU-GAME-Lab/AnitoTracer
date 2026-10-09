#pragma once

#include "IComponentUI.hpp"
#include "ScriptComponent.hpp"
#include "ScriptRegistry.hpp"

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

        if (const ScriptDescriptor* descriptor = ScriptRegistry::GetInstance().Find(script->GetScriptName())) {
            DrawErrors(descriptor->path);
        }

        const std::vector<std::string> hidden = script->GetHiddenProperties();
        for (gbe::IAutoSerializer* prop : script->properties) {
            if (prop->m_id == "m_name") continue;
            if (std::find(hidden.begin(), hidden.end(), prop->m_id) != hidden.end()) continue;
            prop->DrawInspector();
        }
    }

private:
    static void DrawErrors(const std::filesystem::path& path) {
        const auto& errors = ScriptRegistry::GetInstance().GetErrors(path);
        if (errors.empty()) return;

        const std::string label = std::to_string(errors.size()) + " script error(s)##" + path.string();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        const bool open = ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
        ImGui::PopStyleColor();
        if (!open) return;

        const std::string prefix = path.string();
        for (const std::string& error : errors) {
            const bool hasPrefix = error.compare(0, prefix.size(), prefix) == 0;
            const std::string text = hasPrefix ? error.substr(prefix.size()) : error;
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "%s", text.c_str());
            ImGui::PopTextWrapPos();
        }
        ImGui::TreePop();
    }
};
