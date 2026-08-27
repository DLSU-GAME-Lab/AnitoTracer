#pragma once

#include "Panels/BasePanel.hpp"
#include "../../Objects/HierarchyManager.hpp"
#include "../../Objects/Components/EditorCamera.hpp"
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
        void SetSelectedObject(HierarchyObject::Ref obj);

    private:
        HierarchyObject::Ref m_SelectedObject = nullptr;
        HierarchyObject::Ref m_pendingDraggedObject = nullptr;
        HierarchyObject::Ref m_pendingDropParent = nullptr;

        // Recursive helper function to draw tree nodes for each object
        void DrawNode(HierarchyObject::Ref node);

        bool IsEditorCameraObject(HierarchyObject::Ref obj) const;
    };

}