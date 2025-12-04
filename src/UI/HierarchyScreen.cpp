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

	ImGui::Begin(ICON_MD_SORT " Hierarchy", nullptr, UISettings::GlobalWindowFlags | ImGuiWindowFlags_MenuBar);

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

        if (ImGui::BeginPopupContextWindow())
        {
            HierarchyMenuPopup();
            ImGui::EndPopup();
        }
    }

	ImGui::End();
    ImGui::PopStyleColor();
}

void HierarchyScreen::HierarchyMenuPopup()
{
    bool isThereSelected = !GameObjectManager::getInstance()->GetSelectedObject();

    if (isThereSelected) //grey out and unselectable if no selected object
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    if (ImGui::Selectable("Cut"))       GameObjectManager::getInstance()->CutSelectedObject();
    if (ImGui::Selectable("Copy"))      GameObjectManager::getInstance()->CopySelectedObject();
    if (ImGui::Selectable("Paste"))     GameObjectManager::getInstance()->PasteObject();
    if (ImGui::Selectable("Duplicate")) GameObjectManager::getInstance()->DuplicateSelectedObject();
    if (ImGui::Selectable("Delete"))    GameObjectManager::getInstance()->DeleteSelectedObject();

    if (isThereSelected)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }

	ImGui::Separator();

	if (ImGui::BeginMenu("3D Objects"))
	{
        if (ImGui::MenuItem("Cube"))
        {
            CreatePrimitive(GameObject::PrimitiveType::CUBE, "Cube");
        }

		if(ImGui::MenuItem("Sphere"))
        {
            CreatePrimitive(GameObject::PrimitiveType::SPHERE, "Sphere");
        }

        if (ImGui::MenuItem("Plane"))
        {
            CreatePrimitive(GameObject::PrimitiveType::PLANE, "Plane");
        }

        if (ImGui::MenuItem("Cylinder"))
        {
            CreatePrimitive(GameObject::PrimitiveType::CYLINDER, "Cylinder");
        }

        if (ImGui::MenuItem("Capsule"))
        {
            CreatePrimitive(GameObject::PrimitiveType::CAPSULE, "Capsule");
        }

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Lights"))
	{
		ImGui::MenuItem("Point Light");
		ImGui::MenuItem("Directional Light");
		ImGui::MenuItem("Spot Light");

		ImGui::EndMenu();
	}
}

/* Helper */
void HierarchyScreen::CreatePrimitive(GameObject::PrimitiveType type, String name)
{
    CommandManager::getInstance()->executeCommand(
        new CreatePrimitiveCommand(type, name)
    );
}

void HierarchyScreen::CreateObjectPopup()
{
    if (ImGui::BeginPopup("CreateGameObjectsPopup"))
    {
        if (ImGui::BeginMenu("3D Objects"))
        {
            if (ImGui::MenuItem("Cube"))
            {
                CreatePrimitive(GameObject::PrimitiveType::CUBE, "Cube");
            }

            if (ImGui::MenuItem("Sphere"))
            {
                CreatePrimitive(GameObject::PrimitiveType::SPHERE, "Sphere");
            }

            if (ImGui::MenuItem("Plane"))
            {
                CreatePrimitive(GameObject::PrimitiveType::PLANE, "Plane");
            }

            if (ImGui::MenuItem("Cylinder"))
            {
                CreatePrimitive(GameObject::PrimitiveType::CYLINDER, "Cylinder");
            }

            if (ImGui::MenuItem("Capsule"))
            {
                CreatePrimitive(GameObject::PrimitiveType::CAPSULE, "Capsule");
            }

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
    const auto objectList = GameObjectManager::getInstance()->GetAllRootObjects();
    std::string activeCamName = CameraManager::getInstance()->getActiveCamera()->getName();
    ImGui::Text("Active Camera: %s", activeCamName.c_str());

    std::string filterStr(filter);
    std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

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
    bool hasChildren = !obj->GetChildren().empty();
	tempId++;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;

    GameObject* selectedObject = GameObjectManager::getInstance()->GetSelectedObject();

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
        GameObjectManager::getInstance()->SetSelectedObject(obj);

        // If Camera is selected, set main camera. If not, deactivate main camera.
        if (GameObjectManager::getInstance()->GetSelectedObject()->getType() == GameObject::CAMERA)
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
                draggedObj && draggedObj != obj && !obj->IsDescendantOf(draggedObj))
            {
                hasValidDropTarget = true;

                auto oldParent = draggedObj->GetParent();

                // Assign new parent
				CommandManager::getInstance()->executeCommand(
                    new ReparentCommand(
                        draggedObj, 
                        oldParent,
						oldParent ? oldParent->GetChildIndex(draggedObj) : GameObjectManager::getInstance()->GetObjectIndex(draggedObj),
                        obj,
                        obj->GetChildren().size()
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
        GameObject* selectedObject = GameObjectManager::getInstance()->GetSelectedObject();

        if (selectedObject)
        {
            auto oldParent = selectedObject->GetParent();

            if (oldParent) // if old parent == null (in root); do nothing
            {
                CommandManager::getInstance()->executeCommand(            // Assign to root
                    new ReparentCommand(
                        selectedObject,
                        oldParent,
                        oldParent->GetChildIndex(selectedObject),
                        nullptr,
                        GameObjectManager::getInstance()->GetRootCount()
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
        for (const auto& child : obj->GetChildren())
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

