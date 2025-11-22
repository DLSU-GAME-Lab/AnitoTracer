#include "DragAndDropUtils.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "glm/fwd.hpp"
#include "Assets/Model.hpp"
#include "From-GDGRAP2/MaterialLibrary.h"
#include "From-GDGRAP2/ModelManager.h"
#include "Utilities/DragAndDrop/DragAndDropConstants.h"

#include <filesystem>
#include "Utilities/FileExplorer/FileExplorerUtils.h"

namespace fs = std::filesystem;

void DragAndDropUtils::createFullPanelDummy() {
    ImVec2 dummyStartPos = ImGui::GetCursorScreenPos();
    ImVec2 dummySize = ImGui::GetContentRegionAvail();
    ImGui::Dummy(dummySize);
    ImGui::SetNextItemAllowOverlap();
    ImGui::SetCursorScreenPos(dummyStartPos);
}

void DragAndDropUtils::attachModelInstantiateSource(std::string sourcePath) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        ImGui::SetDragDropPayload(DragAndDropConstants::MODEL_PATH, sourcePath.c_str(), (strlen(sourcePath.c_str()) + 1) * sizeof(char));
        ImGui::Text(fs::path(sourcePath).filename().string().c_str());
        ImGui::EndDragDropSource();
    }
}

void DragAndDropUtils::attachModelInstantiateTarget() {
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DragAndDropConstants::MODEL_PATH))
        {
            const auto i = glm::mat4(1);
            const char* path = (const char*)payload->Data;
            if (fs::path(path).extension() == ".obj") {
                Assets::Model draggedModel = Assets::Model::LoadModel(path);
                std::shared_ptr<GameObject> draggedObj = std::make_shared<GameObject>(path, GameObject::PrimitiveType::MESH, std::make_shared<Assets::Model>(draggedModel));
                ModelManager::getInstance()->addObject(draggedObj);
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void DragAndDropUtils::attachFileMoveTarget(std::string destPath) {
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DragAndDropConstants::MODEL_PATH))
        {
            const auto i = glm::mat4(1);
            const char* sourcePath = (const char*)payload->Data;

            fs::path fsSourcePath(sourcePath);
            fs::path fsDestDir(destPath);
            fs::path fsNewPath = fsDestDir / fsSourcePath.filename();

            if (fs::is_directory(fs::path(destPath))) {
                fs::rename(fsSourcePath, fsNewPath);
            }
        }

        ImGui::EndDragDropTarget();
    }
}