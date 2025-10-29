#include "HierarchyScreen.h"

#include <imgui_internal.h>
#include "imgui.h"
#include "From-GDGRAP2/ModelManager.h"
#include "UIManager.h"
#include "Engine/CameraSystem/CameraManager.h"
#include "From-GDGRAP2/RTConfig.h"
#include "Utilities/HotkeySystem.hpp"
#include "From-GDGRAP2/EventBroadcaster.h"

HierarchyScreen::HierarchyScreen() : AUIScreen(UINames::HIERARCHY_SCREEN)
{
    HotkeySystem::getInstance()->addListener(this);
}

HierarchyScreen::~HierarchyScreen()
{
    HotkeySystem::getInstance()->removeListener(this);
}

void HierarchyScreen::OnActionPressed(Hotkey::Action action)
{
    // can be moved somewhere else or have if window focused with viewport
    if (action == Hotkey::Action::Toggle_GameObjectEnabled)
    {
        auto currentState = ModelManager::getInstance()->getSelectedObject()->isActive();
        ModelManager::getInstance()->getSelectedObject()->setActive(!currentState);
        EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
    }

    // can be moved somewhere else or have if window focused with viewport
    if (action == Hotkey::Action::Delete_GameObject)
    {
        auto currentObj = ModelManager::getInstance()->getSelectedObject();
        ModelManager::getInstance()->deleteObject(currentObj);
        ModelManager::getInstance()->clearSelectedObject();
        EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
    }

    if (action == Hotkey::Action::Hierarchy_SetAsFirstSibling)
    {
        auto currentObj = ModelManager::getInstance()->getSelectedObject();
        if (!currentObj) return;

        auto parent = currentObj->getParent();
        if (!parent) return;

        parent->removeChild(currentObj.get());
        parent->addChildFront(currentObj.get());
    }

    if (action == Hotkey::Action::Hierarchy_SetAsLastSibling)
    {
        auto currentObj = ModelManager::getInstance()->getSelectedObject();
        if (!currentObj) return;

        auto parent = currentObj->getParent();
        if (!parent) return;

        parent->removeChild(currentObj.get());
        parent->addChildLast(currentObj.get());
    }

    if (action == Hotkey::Action::Hierarchy_ToggleVisibilityWithDescendants)
    {
        auto currentObj = ModelManager::getInstance()->getSelectedObject();
        if (!currentObj) return;

        auto currentState = ModelManager::getInstance()->getSelectedObject()->isVisible();
        auto newState = !currentState;

        currentObj->setVisibility(newState);

        for (auto child : currentObj->getChildrenRecursive())
        {
            child->setVisibility(newState);
        }

        EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
    }

    if (action == Hotkey::Action::Hierarchy_TogglePickabilityWithDescendants)
    {
        auto currentObj = ModelManager::getInstance()->getSelectedObject();
        if (!currentObj) return;

        auto currentState = ModelManager::getInstance()->getSelectedObject()->isPickable();
        auto newState = !currentState;

        currentObj->setPickability(newState);

        for (auto child : currentObj->getChildrenRecursive())
        {
            child->setPickability(newState);
        }
    }
}

void HierarchyScreen::drawUI()
{
    //setWindowAlignment(ScreenAlign::TOP_RIGHT);

	ImGui::Begin("Hierarchy", nullptr, UISettings::GlobalWindowFlags);

	// Search Bar
	static char searchBuffer[128] = "";
	ImGui::InputTextWithHint("##Search", "Search objects...", searchBuffer, IM_ARRAYSIZE(searchBuffer));

	this->updateObjectList(searchBuffer);

	ImGui::End();
}

void HierarchyScreen::updateObjectList(const char* filter)
{
    const ModelManager::List objectList = ModelManager::getInstance()->getAllObjects();
    std::string activeCamName = CameraManager::getInstance()->getActiveCamera()->getName();
    ImGui::Text("Active Camera: %s", activeCamName.c_str());

    std::string filterStr(filter);
    std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

    for (const auto& obj : objectList)
    {
        if (!obj->getParent())
        {
            std::string objName = obj->getName();
            std::transform(objName.begin(), objName.end(), objName.begin(), ::tolower);

            if (filterStr.empty() || objName.find(filterStr) != std::string::npos)
            {
                drawObjectNode(obj.get());
            }
        }
    }
}

void HierarchyScreen::drawObjectNode(GameObject* obj)
{
    if (!obj) return;

    String objectName = obj->getName();
    bool hasChildren = !obj->getChildren().empty();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;

    GameObject* selectedObject = ModelManager::getInstance()->getSelectedObject().get();
    if (selectedObject == obj)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    // Keep parent node open if previously opened
    bool isNodeOpen = openNodes.count(objectName) > 0;
    if (isNodeOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

    bool open = ImGui::TreeNodeEx(objectName.c_str(), flags);

    static bool hasValidDropTarget = false;
    static bool isDragging = false;

    // Selection Logic
    if (ImGui::IsItemClicked())
    {
        ModelManager::getInstance()->setSelectedObject(objectName);

        // If Camera is selected, set main camera. If not, deactivate main camera.
        if (ModelManager::getInstance()->getSelectedObject()->getType() == GameObject::CAMERA)
        {
            const std::shared_ptr<Camera> cam = CameraManager::getInstance()->findCameraByName(objectName);
            CameraManager::getInstance()->setMainCamera(cam);
        }
        else
        {
            CameraManager::getInstance()->setMainCamera(nullptr);
        }
    }

    // Drag Source
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("OBJECT_PARENTING", &obj, sizeof(GameObject*));

        hasValidDropTarget = false;
        isDragging = true;
        ImGui::Text("Dragging %s", objectName.c_str());

        ImGui::EndDragDropSource();
    }

    // Drop Target
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("OBJECT_PARENTING"))
        {
            // Prevent dragging a parent into its own child 
            if (GameObject* draggedObj = *static_cast<GameObject**>(payload->Data);
                draggedObj && draggedObj != obj && !obj->isDescendantOf(draggedObj))
            {
                hasValidDropTarget = true;

                // If dragged object had a parent, remove it from old parent
                if (draggedObj->getParent())
                {
                    draggedObj->getParent()->removeChild(draggedObj);
                }

                // Assign new parent
                obj->addChild(draggedObj);

                // Force the node open when an object is dropped here
                openNodes.insert(objectName);

                isDragging = false; // Reset dragging state after a valid drop
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Auto-Expand When Hovered During Drag
    if (ImGui::IsDragDropActive() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    {
        openNodes.insert(objectName);
        open = true;  // Ensure the node is visually open this frame
    }

    // Handle Unparenting (Dragged to Empty Space)
    if (isDragging && ImGui::IsMouseReleased(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
    {
        GameObject* selectedObject = ModelManager::getInstance()->getSelectedObject().get();
        if (selectedObject)
        {
            selectedObject->setParent(nullptr);
        }
        isDragging = false; // Reset dragging state after unparenting
    }

    if (!ImGui::IsDragDropActive() && ImGui::IsMouseReleased(0))
    {
        isDragging = false;
    }

    if (open)
    {
        for (const auto& child : obj->getChildren())
        {
            drawObjectNode(child);
        }
        ImGui::TreePop();
    }
    else
    {
        openNodes.erase(objectName);
    }
}

