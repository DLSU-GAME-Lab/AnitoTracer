#include "DragAndDropUtils.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "glm/fwd.hpp"
#include "Assets/Model.hpp"
#include "From-GDGRAP2/MaterialLibrary.h"
#include "From-GDGRAP2/ModelManager.h"
#include "Utilities/DragAndDrop/DragAndDropConstants.h"
#include "Utilities/FileExplorer/FileExplorerUtils.h"

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
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DragAndDropConstants::MODEL_PATH))
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

void DragAndDropUtils::attachModelInstantiateSource(std::string filePath, std::string fileName) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        if (FileExplorerUtils::getFileExtension(fileName) == "obj") {
            ImGui::SetDragDropPayload(DragAndDropConstants::MODEL_PATH, filePath.c_str(), (strlen(filePath.c_str()) + 1) * sizeof(char));
        }

        ImGui::Text(fileName.c_str());

        ImGui::EndDragDropSource();
    }
}