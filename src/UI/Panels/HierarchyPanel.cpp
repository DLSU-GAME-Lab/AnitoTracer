#include "HierarchyPanel.hpp"
#include "HierarchyPanel.hpp"

namespace Diligent {

    HierarchyPanel::HierarchyPanel(const std::string& name)
        : BasePanel(name)
    {}

    void HierarchyPanel::Draw()
    {
        // Do not render if the panel is toggled off
        if (!m_IsVisible) return;
        
        // Begin the ImGui window with the panel's name and visibility state
        if (ImGui::Begin(m_Name.c_str(), &m_IsVisible))
        {
            // Retrieve the active root nodes from the singleton manager
            const auto& rootObjects = HierarchyManager::GetInstance().GetRootObjects();

            // Iterate and draw each root node
            for (const auto& root : rootObjects)
            {
                DrawNode(root.get());
            }

            if (m_pendingDraggedObject) {
                HierarchyManager::GetInstance().ReparentObject(
                    m_pendingDraggedObject, m_pendingDropParent);
                m_pendingDraggedObject = nullptr;
                m_pendingDropParent = nullptr;
            }

            if (ImGui::IsWindowHovered() &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !ImGui::IsAnyItemHovered())
            {
                SetSelectedObject(nullptr);
            }

            if (m_SelectedObject && ImGui::IsKeyPressed(ImGuiKey_Delete) &&
                !ImGui::GetIO().WantTextInput)
            {
                HierarchyManager::GetInstance().QueueObjectDeletion(m_SelectedObject);
                SetSelectedObject(nullptr);
            }

            if (!ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl &&
                ImGui::IsKeyPressed(ImGuiKey_C) && m_SelectedObject)
            {
                HierarchyManager::GetInstance().CopyObject(m_SelectedObject);
            }

            if (!ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl &&
                ImGui::IsKeyPressed(ImGuiKey_V) &&
                HierarchyManager::GetInstance().HasCopiedObject())
            {
                HierarchyObject::Ref pastedObject =
                    HierarchyManager::GetInstance().PasteObject(m_SelectedObject);
                if (pastedObject) SetSelectedObject(pastedObject);
            }
        }
        ImGui::End();
    }

    void HierarchyPanel::SetSelectedObject(HierarchyObject::Ref obj)
    {
        m_SelectedObject = obj;
    }

    void HierarchyPanel::DrawNode(HierarchyObject::Ref node)
    {
        if (!node) return;

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
        if (m_SelectedObject == node)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        // Render the node using the object's memory address as a unique ID
        bool nodeOpen = ImGui::TreeNodeEx((void*)node.GetID(), flags, "%s", node.GetPtr()->GetName().c_str());

        //For drag drop hierarchy / component references
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            // We pass the raw pointer address as the payload data
            HierarchyObject* objPtr = node.GetPtr();
            ImGui::SetDragDropPayload("DND_HIERARCHY_OBJ", &objPtr, sizeof(HierarchyObject*));

            // Show a cute tooltip while dragging!
            ImGui::Text("Assign %s", objPtr->GetName().c_str());

            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
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
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            SetSelectedObject(node);
        }

        if (ImGui::BeginPopupContextItem("ObjectContextMenu"))
        {
            if (ImGui::MenuItem("Copy Object", "Ctrl+C"))
            {
                SetSelectedObject(node);
                HierarchyManager::GetInstance().CopyObject(node);
            }

            if (ImGui::MenuItem("Paste Object", "Ctrl+V",
                false, HierarchyManager::GetInstance().HasCopiedObject()))
            {
                SetSelectedObject(node);
                HierarchyObject::Ref pastedObject =
                    HierarchyManager::GetInstance().PasteObject(node);
                if (pastedObject) SetSelectedObject(pastedObject);
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Delete Object", "Del"))
            {
                HierarchyManager::GetInstance().QueueObjectDeletion(node);
                if (m_SelectedObject == node)
                {
                    SetSelectedObject(nullptr);
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
                DrawNode(child.get());
            }
            ImGui::TreePop();
        }
    }

}