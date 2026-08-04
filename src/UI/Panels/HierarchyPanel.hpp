#pragma once

#include "Panels/BasePanel.hpp"
#include "../../Objects/HierarchyManager.hpp"
#include <string>
#include "imgui.h"

namespace Diligent {

    class HierarchyPanel : public BasePanel
    {
    public:
        // Initialize the panel with a default name
        HierarchyPanel(const std::string& name = "Hierarchy");
        ~HierarchyPanel() override = default;

        // Implementation of the abstract Draw method
        void Draw() override;

        HierarchyObject::Ref GetSelectedObject() const { return m_SelectedObject; }

    private:
        HierarchyObject::Ref m_SelectedObject = nullptr;

        // Recursive helper function to draw tree nodes for each object
        void DrawNode(HierarchyObject::Ref node);
    };

}