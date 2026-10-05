#include "HierarchyPanel.hpp"

#include "HierarchyFeatures/PrefabFeature.hpp"

#include <algorithm>
#include <cfloat>
#include <cstdio>

namespace Diligent {

    bool HierarchyPanel::IsEditorCameraObject(HierarchyObject::Ref obj) const
    {
        return obj && obj.GetPtr()->GetComponent<EditorCamera>() != nullptr;
    }

    HierarchyPanel::HierarchyPanel(const std::string& name)
        : BasePanel(name)
    {
        // Undo/Redo rebuilds the scene tree (new instance IDs, same GUIDs), so
        // resolve the selection by GUID afterwards instead of losing it.
        gbe::UndoRedoManager::GetInstance().SetSelectionHooks(
            [this]() {
                return m_SelectedObject.GetPtr() ? m_SelectedObject.GetPtr()->GetGUID() : gbe::GUID::Empty();
            },
            [this](const gbe::GUID& guid) {
                if (!guid) {
                    SetSelectedObject(nullptr);
                    return;
                }
                if (HierarchyObject* obj = gbe::SceneRegistry::GetInstance().Resolve<HierarchyObject>(guid)) {
                    SetSelectedObject(obj->getRef());
                }
                else {
                    SetSelectedObject(nullptr);
                }
            });
    }

    void HierarchyPanel::Draw()
    {
        // Do not render if the panel is toggled off
        if (!m_IsVisible) return;
        
        // Begin the ImGui window with the panel's name and visibility state
        if (ImGui::Begin(m_Name.c_str(), &m_IsVisible))
        {
            // Retrieve the active root nodes from the singleton manager
            const auto& rootObjects = HierarchyManager::GetInstance().GetRootObjects();

            m_VisibleOrder.clear();

            // Iterate and draw each root node
            for (const auto& root : rootObjects)
            {
                if (!IsEditorCameraObject(root.get())) {
                    DrawNode(root.get());
                }
            }

            ResolvePendingRangeSelection();

            // Empty space below the tree: click deselects, dropping here unparents.
            ImVec2 emptySize = ImGui::GetContentRegionAvail();
            if (emptySize.x < 1.0f) emptySize.x = 1.0f;
            if (emptySize.y < 1.0f) emptySize.y = 1.0f;
            ImGui::InvisibleButton("##HierarchyEmptySpace", emptySize);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                SetSelectedObject(nullptr);
            }
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_HIERARCHY_OBJ"))
                {
                    auto* draggedObject = *static_cast<HierarchyObject* const*>(payload->Data);
                    if (draggedObject) {
                        m_pendingDraggedObject = draggedObject;
                        m_pendingDropParent = nullptr;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (m_pendingDraggedObject) {
                // Dragging a selected node moves the whole selection.
                if (IsSelected(m_pendingDraggedObject)) {
                    for (const auto& obj : GetSelectedRoots()) {
                        HierarchyManager::GetInstance().ReparentObject(obj, m_pendingDropParent);
                    }
                }
                else {
                    HierarchyManager::GetInstance().ReparentObject(
                        m_pendingDraggedObject, m_pendingDropParent);
                }
                m_pendingDraggedObject = nullptr;
                m_pendingDropParent = nullptr;
            }

            if (m_SelectedObject && !m_RenameTarget && ImGui::IsKeyPressed(ImGuiKey_F2) &&
                !ImGui::GetIO().WantTextInput)
            {
                m_RenameTarget = m_SelectedObject;
                m_RenameFocusPending = true;
                std::snprintf(m_RenameBuffer, sizeof(m_RenameBuffer), "%s",
                    m_SelectedObject.GetPtr()->GetName().c_str());
            }

            if (m_RenameTarget && !m_RenameTarget.IsValid()) {
                m_RenameTarget = nullptr;
            }

            if (m_SelectedObject && ImGui::IsKeyPressed(ImGuiKey_Delete) &&
                !ImGui::GetIO().WantTextInput)
            {
                for (const auto& obj : GetSelectedRoots()) {
                    HierarchyManager::GetInstance().QueueObjectDeletion(obj);
                }
                SetSelectedObject(nullptr);
            }

            if (m_SelectedObject && ImGui::IsKeyPressed(ImGuiKey_F) &&
                !ImGui::GetIO().WantTextInput)
            {
                HierarchyManager::GetInstance().GetEditorCamera()->FocusOn(m_SelectedObject.GetPtr()->GetTransform()->GetPosition());
            }

            if (!ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl &&
                ImGui::IsKeyPressed(ImGuiKey_C) && m_SelectedObject)
            {
                HierarchyManager::GetInstance().CopyObjects(GetSelectedRoots());
            }

            if (!ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl &&
                ImGui::IsKeyPressed(ImGuiKey_V) &&
                HierarchyManager::GetInstance().HasCopiedObject())
            {
                PasteAndSelect(m_SelectedObject);
            }
        }
        ImGui::End();
    }

    void HierarchyPanel::SetSelectedObject(HierarchyObject::Ref obj)
    {
        m_Selection.clear();
        if (!obj || IsEditorCameraObject(obj)) {
            m_SelectedObject = nullptr;
            m_SelectionAnchor = nullptr;
            return;
        }
        m_SelectedObject = obj;
        m_SelectionAnchor = obj;
        m_Selection.push_back(obj);
    }

    std::vector<HierarchyObject::Ref> HierarchyPanel::GetSelectedObjects() const
    {
        std::vector<HierarchyObject::Ref> result;
        result.reserve(m_Selection.size());
        for (const auto& obj : m_Selection) {
            if (obj.IsValid()) result.push_back(obj);
        }
        return result;
    }

    std::vector<HierarchyObject::Ref> HierarchyPanel::GetSelectedRoots() const
    {
        const auto selected = GetSelectedObjects();
        std::vector<HierarchyObject::Ref> roots;
        for (const auto& obj : selected) {
            bool hasSelectedAncestor = false;
            for (HierarchyObject* p = obj.GetPtr()->GetParent().GetPtr(); p != nullptr; p = p->GetParent().GetPtr()) {
                if (IsSelected(p->getRef())) { hasSelectedAncestor = true; break; }
            }
            if (!hasSelectedAncestor) roots.push_back(obj);
        }
        return roots;
    }

    bool HierarchyPanel::IsSelected(HierarchyObject::Ref obj) const
    {
        if (!obj) return false;
        for (const auto& s : m_Selection) {
            if (s == obj) return true;
        }
        return false;
    }

    void HierarchyPanel::ToggleSelectedObject(HierarchyObject::Ref obj)
    {
        if (!obj || IsEditorCameraObject(obj)) return;

        // Drop stale entries so the primary can fall back to a live object.
        m_Selection = GetSelectedObjects();

        for (auto it = m_Selection.begin(); it != m_Selection.end(); ++it) {
            if (*it == obj) {
                m_Selection.erase(it);
                if (m_SelectedObject == obj) {
                    m_SelectedObject = m_Selection.empty() ? HierarchyObject::Ref(nullptr) : m_Selection.back();
                }
                m_SelectionAnchor = m_SelectedObject;
                return;
            }
        }

        m_Selection.push_back(obj);
        m_SelectedObject = obj;
        m_SelectionAnchor = obj;
    }

    void HierarchyPanel::PasteAndSelect(HierarchyObject::Ref parent)
    {
        const auto pasted = HierarchyManager::GetInstance().PasteObjects(parent);
        if (pasted.empty()) return;

        SetSelectedObject(pasted.front());
        for (size_t i = 1; i < pasted.size(); ++i) {
            ToggleSelectedObject(pasted[i]);
        }
    }

    void HierarchyPanel::ResolvePendingRangeSelection()
    {
        const HierarchyObject::Ref target = m_PendingRangeTarget;
        m_PendingRangeTarget = nullptr;
        if (!target) return;

        int anchorIdx = -1, targetIdx = -1;
        for (int i = 0; i < (int)m_VisibleOrder.size(); ++i) {
            if (m_VisibleOrder[i] == m_SelectionAnchor) anchorIdx = i;
            if (m_VisibleOrder[i] == target) targetIdx = i;
        }
        if (targetIdx < 0) return;
        // Anchor hidden (collapsed/removed): range collapses to the clicked node.
        if (anchorIdx < 0) anchorIdx = targetIdx;

        std::vector<HierarchyObject::Ref> selection =
            m_PendingRangeAdditive ? GetSelectedObjects() : std::vector<HierarchyObject::Ref>{};

        const int lo = std::min(anchorIdx, targetIdx);
        const int hi = std::max(anchorIdx, targetIdx);
        for (int i = lo; i <= hi; ++i) {
            const auto& obj = m_VisibleOrder[i];
            if (std::find(selection.begin(), selection.end(), obj) == selection.end()) {
                selection.push_back(obj);
            }
        }

        // The anchor is kept so successive shift-clicks extend from the same origin.
        const HierarchyObject::Ref anchor = m_SelectionAnchor;
        m_Selection = std::move(selection);
        m_SelectedObject = target;
        m_SelectionAnchor = anchor ? anchor : target;
    }

    void HierarchyPanel::DrawNode(HierarchyObject::Ref node)
    {
        if (!node) return;
        if (IsEditorCameraObject(node)) return;

        // Configure default behavior for the tree nodes
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        // If the object has no children, render it as a leaf node without an expand arrow
        if (node.GetPtr()->GetChildren().empty())
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        // Highlight the node if it is the currently selected object
        if (IsSelected(node))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        // Render the node using the object's memory address as a unique ID
        const bool isPrefabInstance = node.GetPtr()->IsPrefabInstance();
        const bool renaming = (m_RenameTarget == node);
        bool nodeOpen = ImGui::TreeNodeEx(
            (void*)node.GetID(), flags, "%s%s",
            renaming ? "" : node.GetPtr()->GetName().c_str(),
            (!renaming && isPrefabInstance) ? " [Prefab]" : "");
        m_VisibleOrder.push_back(node);

        if (renaming)
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (m_RenameFocusPending) {
                ImGui::SetKeyboardFocusHere();
                m_RenameFocusPending = false;
            }
            const bool committed = ImGui::InputText("##RenameNode", m_RenameBuffer, sizeof(m_RenameBuffer),
                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
            if (committed && m_RenameBuffer[0] != '\0') {
                node.GetPtr()->SetName(m_RenameBuffer);
            }
            if (committed || ImGui::IsItemDeactivated() || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                m_RenameTarget = nullptr;
            }
        }

        //For drag drop hierarchy / component references
        if (!renaming && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            // We pass the raw pointer address as the payload data
            HierarchyObject* objPtr = node.GetPtr();
            ImGui::SetDragDropPayload("DND_HIERARCHY_OBJ", &objPtr, sizeof(HierarchyObject*));

            // Show a cute tooltip while dragging!
            ImGui::Text("Assign %s", objPtr->GetName().c_str());

            ImGui::EndDragDropSource();
        }

        if (!renaming && ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_HIERARCHY_OBJ"))
            {
                auto* draggedObject = *static_cast<HierarchyObject* const*>(payload->Data);
                if (draggedObject) {
                    m_pendingDraggedObject = draggedObject;
                    m_pendingDropParent = node;
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Update the selected object when clicked
        if (!renaming && ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            const ImGuiIO& io = ImGui::GetIO();
            if (io.KeyShift)
            {
                // Resolved after the frame's nodes are known so the range covers visible rows only.
                m_PendingRangeTarget = node;
                m_PendingRangeAdditive = io.KeyCtrl;
            }
            else if (io.KeyCtrl)
            {
                ToggleSelectedObject(node);
            }
            else
            {
                SetSelectedObject(node);
            }
        }

        if (!renaming && ImGui::BeginPopupContextItem("ObjectContextMenu"))
        {
            if (ImGui::MenuItem("Copy Object", "Ctrl+C"))
            {
                if (!IsSelected(node)) SetSelectedObject(node);
                HierarchyManager::GetInstance().CopyObjects(GetSelectedRoots());
            }

            if (ImGui::MenuItem("Paste Object", "Ctrl+V",
                false, HierarchyManager::GetInstance().HasCopiedObject()))
            {
                PasteAndSelect(node);
            }

            if (ImGui::MenuItem("Create Prefab Asset"))
            {
                PrefabFeature::GetInstance().CreatePrefabAsset(node);
                SetSelectedObject(node);
            }

            if (node.GetPtr()->IsPrefabInstance())
            {
                if (ImGui::MenuItem("Apply Prefab"))
                {
                    PrefabFeature::GetInstance().ApplyPrefabToAsset(node);
                }

                if (ImGui::MenuItem("Revert Prefab"))
                {
                    PrefabFeature::GetInstance().RevertPrefabInstance(node);
                    SetSelectedObject(node);
                }

                if (ImGui::MenuItem("Unpack Prefab"))
                {
                    PrefabFeature::GetInstance().UnpackPrefabInstance(node);
                }
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Delete Object", "Del"))
            {
                if (IsSelected(node))
                {
                    for (const auto& obj : GetSelectedRoots()) {
                        HierarchyManager::GetInstance().QueueObjectDeletion(obj);
                    }
                    SetSelectedObject(nullptr);
                }
                else
                {
                    HierarchyManager::GetInstance().QueueObjectDeletion(node);
                }
            }
            ImGui::EndPopup();
        }

        // If the tree node is expanded by the user, recursively draw its children
        if (nodeOpen)
        {
            const auto& children = node.GetPtr()->GetChildren();
            for (const auto& child : children)
            {
                if (!IsEditorCameraObject(child.get())) {
                    DrawNode(child.get());
                }
            }
            ImGui::TreePop();
        }
    }

}