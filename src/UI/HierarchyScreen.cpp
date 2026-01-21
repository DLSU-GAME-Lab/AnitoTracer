 #include "HierarchyScreen.h"

#include <imgui_internal.h>
#include "imgui.h"
#include "From-GDGRAP2/ModelManager.h"
#include "From-GDGRAP2/GameObject.h"
#include "UIManager.h"
#include "Engine/CameraSystem/CameraManager.h"
#include "From-GDGRAP2/RTConfig.h"
#include "IconsMaterialDesign.h"
#include "EditorTheme.hpp"
#include "StateManagement/CommandManager.hpp"
#include "StateManagement/ConcreteCommands/HierarchyCommands.hpp"

HierarchyScreen::HierarchyScreen() : AUIScreen(UINames::HIERARCHY_SCREEN)
{
}

HierarchyScreen::~HierarchyScreen()
{
}

void HierarchyScreen::drawUI()
{
    //setWindowAlignment(ScreenAlign::TOP_RIGHT);

    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, DarkTheme.TAB_ACTIVE);

	ImGui::Begin("Hierarchy", nullptr, UISettings::GlobalWindowFlags | ImGuiWindowFlags_MenuBar);

	// Search Bar
	static char searchBuffer[128] = "";

    if (ImGui::BeginMenuBar())
    {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[3]);
        if (ImGui::MenuItem(ICON_MD_ADD ICON_MD_ARROW_DROP_DOWN))
        {
            ImGui::OpenPopup("CreateGameObjectsPopup");
        }
        ImGui::PopFont();

        CreateObjectPopup();

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[4]);
        ImGui::TextUnformatted(ICON_MD_SEARCH);
		ImGui::PopFont();
        ImGui::SameLine();

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::AlignTextToFramePadding();
        ImGui::InputTextWithHint("##Search", "Search objects...", searchBuffer, IM_ARRAYSIZE(searchBuffer));


        ImGui::EndMenuBar();
    }

    if (ImGui::CollapsingHeader("Scene Objects", ImGuiTreeNodeFlags_DefaultOpen))
    {
        this->updateObjectList(searchBuffer);
    }

	ImGui::End();
    ImGui::PopStyleColor();
}

void HierarchyScreen::CreateObjectPopup()
{
    if (ImGui::BeginPopup("CreateGameObjectsPopup"))
    {
        if (ImGui::BeginMenu("3D Objects"))
        {
			ImGui::MenuItem("Cube");
			ImGui::MenuItem("Sphere");
			ImGui::MenuItem("Plane");
			ImGui::MenuItem("Cylinder");
			ImGui::MenuItem("Capsule");

			ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Lights"))
        {
			ImGui::MenuItem("Point Light");
			ImGui::MenuItem("Directional Light");
			ImGui::MenuItem("Spot Light");

            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }
}

void HierarchyScreen::updateObjectList(const char* filter)
{
    auto objectList = ModelManager::getInstance()->getSceneGraph();
	auto camera = CameraManager::getInstance()->getActiveCamera();
    std::string activeCamName = camera->getName();
    ImGui::Text("Active Camera: %s", activeCamName.c_str());

    std::string filterStr(filter);
    std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

	objectList.insert(objectList.begin(), static_cast<GameObject*>(camera));

    for (const auto& obj : objectList)
    {
		std::string objName = obj->getName();
		std::transform(objName.begin(), objName.end(), objName.begin(), ::tolower);

		if (filterStr.empty() || objName.find(filterStr) != std::string::npos)
		{
			drawObjectNode(obj);
		}
    }

	tempId = 0; // Reset tempId for next frame
}

void HierarchyScreen::drawObjectNode(GameObject* obj)
{
    if (!obj) return;

    String objectName = obj->getName();
	String objectId = objectName + std::to_string(tempId); // Unique ID for ImGui
    bool hasChildren = !obj->getChildren().empty();
	tempId++;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;

    GameObject* selectedObject = ModelManager::getInstance()->getSelectedObject();

    if (obj->IsHierarchyNodeOpen())
    {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    if (selectedObject == obj)
    {
		ImGui::PushStyleColor(ImGuiCol_Header, DarkTheme.SCROLLBAR_GRAB_ACTIVE); //blue
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    // Keep parent node open if previously opened
    bool isNodeOpen = openNodes.count(objectName) > 0;
    if (isNodeOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

    ImGui::PushID(objectId.c_str());
    bool open = ImGui::TreeNodeEx(objectName.c_str(), flags);
    obj->SetHierarchyNodeOpen(open);

    if (selectedObject == obj)
    {
        ImGui::PopStyleColor();
    }

    static bool hasValidDropTarget = false;
    static bool isDragging = false;

    // Selection Logic
    if (ImGui::IsItemClicked())
    {
        ModelManager::getInstance()->setSelectedObject(obj);

        // If Camera is selected, set main camera. If not, deactivate main camera.
        if (ModelManager::getInstance()->getSelectedObject()->getType() == GameObject::CAMERA)
        {
            Camera* cam = CameraManager::getInstance()->findCameraByName(objectName);
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

                auto oldParent = draggedObj->getParent();

                // Assign new parent
				CommandManager::getInstance()->executeCommand(
                    new ReparentCommand(
                        draggedObj, 
                        oldParent,
						oldParent ? oldParent->getChildIndex(draggedObj) : ModelManager::getInstance()->getObjectIndex(draggedObj),
                        obj,
                        obj->getChildren().size()
                    )
				);

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
        GameObject* selectedObject = ModelManager::getInstance()->getSelectedObject();

        if (selectedObject)
        {
            auto oldParent = selectedObject->getParent();

            if (oldParent) // if old parent == null (in root); do nothing
            {
                CommandManager::getInstance()->executeCommand(            // Assign to root
                    new ReparentCommand(
                        selectedObject,
                        oldParent,
                        oldParent->getChildIndex(selectedObject),
                        nullptr,
                        ModelManager::getInstance()->getSceneGraphRootSize()
                    )
                );
            }
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

    ImGui::PopID();
}

