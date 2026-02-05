#include "DragAndDropUtils.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "glm/fwd.hpp"
#include "Assets/Model.hpp"
#include "Assets/GameObjectFactory.hpp"
#include "Assets/Material.hpp"
#include "Assets/Texture.hpp"
#include "From-GDGRAP2/MaterialLibrary.h"
#include "From-GDGRAP2/ModelManager.h"
#include "From-GDGRAP2/TextureLibrary.h"
#include "Utilities/DragAndDrop/DragAndDropConstants.h"

#include <filesystem>
#include "Utilities/FileExplorer/FileExplorerUtils.h"
#include "Utilities/FileExplorer/FileExplorerConstants.h"
#include "Utilities/FileExplorer/FileTree.h"

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
            IM_ASSERT(payload->DataSize == sizeof(FileTreeNode*));
            FileTreeNode* srcNode = *(FileTreeNode**)payload->Data;
            fs::path path(srcNode->getPathString());

            if (path.extension() == ".obj") {
                loadObject(path.string(), path.stem().string());
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
                IM_ASSERT(payload->DataSize == sizeof(FileTreeNode*));
                FileTreeNode* srcNode = *(FileTreeNode**)payload->Data;
                fs::path path(srcNode->getPathString());

                if (path.extension() == ".obj") {
                    loadObject(path.string(), path.stem().string());
                }
            }
            ImGui::EndDragDropTarget();
        }
    }
}

void DragAndDropUtils::attachFileMoveTarget(FileTreeNode& destNode) {
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DragAndDropConstants::MODEL_PATH))
        {
            IM_ASSERT(payload->DataSize == sizeof(FileTreeNode*));
            FileTreeNode* srcNode = *(FileTreeNode**)payload->Data;

            fs::path fsSourcePath(srcNode->getPathString());
            fs::path fsDestDir(destNode.getPathString());
            fs::path fsNewPath = fsDestDir / fsSourcePath.filename();

            if (destNode.isDirectory()) {
                fs::rename(fsSourcePath, fsNewPath);
                srcNode->getDirectoryEntry().assign(fsNewPath);

                auto& siblings = srcNode->getParent()->getChildren();

                // add to new path
                srcNode->setParent(&destNode);
                destNode.addChild(*srcNode);

                // remove from original path
                auto newEnd = std::remove(siblings.begin(), siblings.end(), *srcNode);
                siblings.erase(newEnd, siblings.end());

                
            }
        }

        ImGui::EndDragDropTarget();
    }
}

void DragAndDropUtils::copyFileToAssetsRoot(std::string sourcePathString) {
    fs::path fsSourcePath(sourcePathString);
    fs::path fsDestPath(FileExplorerConstants::ASSETS_DIR);

    directory_entry dirEntSource(fsSourcePath);
    if (dirEntSource.is_directory()) {
        fs::copy(fsSourcePath, fsDestPath / fsSourcePath.stem(), fs::copy_options::recursive);
    } else {
        fs::copy(fsSourcePath, fsDestPath, fs::copy_options::recursive);
    }

    directory_entry newEntry(fsDestPath / fsSourcePath.filename());
    FileTreeNode newTreeNode(newEntry);
    newTreeNode.setParent(&(FileTree::getInstance()->getRoot()));
    FileTree::getInstance()->getRoot().addChild(newTreeNode);

    newTreeNode.init();
}

void DragAndDropUtils::loadObject(std::string path, std::string name) {
    auto gameObject = GameObjectFactory::CreateFromModelFile(path, name);
    ModelManager::getInstance()->addObject(std::move(gameObject));

    std::vector<GameObject*> models = ModelManager::getInstance()->getAllActiveObjects();
    std::vector<Assets::Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
    std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();
}