#include "DragAndDropUtils.h"

#include "imgui.h"
#include "glm/fwd.hpp"
#include "Assets/Model.hpp"
#include "From-GDGRAP2/MaterialLibrary.h"

void DragAndDropUtils::createFullPanelDummy() {
    ImVec2 dummyStartPos = ImGui::GetCursorScreenPos();
    ImVec2 dummySize = ImGui::GetContentRegionAvail();
    ImGui::Dummy(dummySize);
    ImGui::SetNextItemAllowOverlap();
    ImGui::SetCursorScreenPos(dummyStartPos);
}

void DragAndDropUtils::attachModelInstantiateTarget() {
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MODEL_PATH_DRAGGABLE"))
        {
            const auto i = glm::mat4(1);
            const char* path = (const char*)payload->Data;
            Assets::Model draggedModel = Assets::Model::LoadModel(path);
            std::shared_ptr<GameObject> draggedObj = std::make_shared<GameObject>(path, GameObject::PrimitiveType::MESH, std::make_shared<Assets::Model>(draggedModel));
            ModelManager::getInstance()->addObject(draggedObj);
        }
        ImGui::EndDragDropTarget();
    }
}