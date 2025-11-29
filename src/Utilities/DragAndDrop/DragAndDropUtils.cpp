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

void DragAndDropUtils::attachFileTreeNodeSource(FileTreeNode* srcNode) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        ImGui::SetDragDropPayload(DragAndDropConstants::MODEL_PATH, &srcNode, sizeof(FileTreeNode*));
        ImGui::Text(srcNode->getName().c_str());
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

void DragAndDropUtils::attachModelInstantiateTargetToViewport(ImGuiViewport* viewport) {
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
        if (ImGui::BeginDragDropTargetViewport(viewport))
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
}

void DragAndDropUtils::attachFileMoveTarget(FileTreeNode* destNode) {
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DragAndDropConstants::MODEL_PATH))
        {
            IM_ASSERT(payload->DataSize == sizeof(FileTreeNode*));
            FileTreeNode* srcNode = *(FileTreeNode**)payload->Data;

            fs::path fsSourcePath(srcNode->getPathString());
            fs::path fsDestDir(destNode->getPathString());
            fs::path fsNewPath = fsDestDir / fsSourcePath.filename();

            if (destNode->isDirectory()) {
                fs::rename(fsSourcePath, fsNewPath);
            }

            destNode->addChild(*srcNode);

            auto newEnd = std::remove(srcNode->getParent()->getChildren().begin(), srcNode->getParent()->getChildren().end(), *srcNode);
            srcNode->getParent()->getChildren().erase(newEnd, srcNode->getParent()->getChildren().end());
            srcNode->setParent(&*destNode);
        }

        ImGui::EndDragDropTarget();
    }
}