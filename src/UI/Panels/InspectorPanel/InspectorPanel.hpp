#pragma once
#include "../BasePanel.hpp"
#include "../HierarchyPanel.hpp"
#include "InspectorRegistry.hpp"

#include <string>
#include "imgui.h"

#include "../../GUIManager.hpp"

namespace Diligent {

    class InspectorPanel : public BasePanel
    {
    public:
        // Requires a reference to the hierarchy panel to query the selected object
        InspectorPanel(HierarchyPanel* hierarchy, const std::string& name = "Inspector")
            : BasePanel(name), m_HierarchyPanel(hierarchy) {}

        ~InspectorPanel() override = default;

        void Draw() override;

    private:
        HierarchyPanel* m_HierarchyPanel;
    };

}