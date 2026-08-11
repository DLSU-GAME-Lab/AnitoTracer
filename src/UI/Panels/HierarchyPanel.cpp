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

        // Update the selected object when clicked
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            SetSelectedObject(node);
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