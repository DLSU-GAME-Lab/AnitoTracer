#pragma once

#include "Panels/BasePanel.hpp"
#include "../../Objects/HierarchyManager.hpp"
#include "../../Objects/Components/EditorCamera.hpp"
#include <string>
#include <vector>
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

        // Primary (active) selection; the inspector, focus and copy features use this one.
        HierarchyObject::Ref GetSelectedObject() const { return m_SelectedObject; }
        // Replaces the whole selection with a single object (or clears it when null).
        void SetSelectedObject(HierarchyObject::Ref obj);

        // All valid selected objects, primary included.
        std::vector<HierarchyObject::Ref> GetSelectedObjects() const;
        // Selected objects with no selected ancestor; use for group transforms/deletes/reparenting.
        std::vector<HierarchyObject::Ref> GetSelectedRoots() const;
        bool IsSelected(HierarchyObject::Ref obj) const;
        // Ctrl+click behaviour: adds/removes obj without touching the rest of the selection.
        void ToggleSelectedObject(HierarchyObject::Ref obj);

    private:
        HierarchyObject::Ref m_SelectedObject = nullptr;
        std::vector<HierarchyObject::Ref> m_Selection;
        HierarchyObject::Ref m_SelectionAnchor = nullptr;

        // Nodes submitted this frame, in on-screen order, for shift range selection.
        std::vector<HierarchyObject::Ref> m_VisibleOrder;
        HierarchyObject::Ref m_PendingRangeTarget = nullptr;
        bool m_PendingRangeAdditive = false;

        void ResolvePendingRangeSelection();
        // Pastes the clipboard under parent and selects everything pasted.
        void PasteAndSelect(HierarchyObject::Ref parent);
        HierarchyObject::Ref m_pendingDraggedObject = nullptr;
        HierarchyObject::Ref m_pendingDropParent = nullptr;

        // Recursive helper function to draw tree nodes for each object
        void DrawNode(HierarchyObject::Ref node);

        bool IsEditorCameraObject(HierarchyObject::Ref obj) const;
    };

}