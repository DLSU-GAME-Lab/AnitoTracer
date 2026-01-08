#include "FileListView.h"

#include "imgui.h"
#include "Utilities/DragAndDrop/DragAndDropUtils.h"
#include "Utilities/FileExplorer/FileExplorerConstants.h"
#include "Utilities/FileExplorer/FileExplorerUtils.h"
#include "Utilities/FileExplorer/FileTree.h"
#include "UI/IconsMaterialDesign.h"
#include "UI/UIManager.h"
#include "UI/FileExplorer/FileIconView.h"

static bool deletePopup = false;

FileListView* FileListView::instance = nullptr;

FileListView::FileListView() {

}

FileListView* FileListView::getInstance() {
    if (instance == nullptr) {
        instance = new FileListView();
    }
    return instance;
}

void FileListView::drawUI() {
    ImGui::PushFont(nullptr);
    renderRootNode(FileTree::getInstance()->getRoot());
    ImGui::PopFont();
}

void FileListView::renderDescendants(FileTreeNode& nodeToRender) {
    if (nodeToRender.getIsOpen()) {
        int i = 0;

        // render child nodes of nodeToRender
        for (auto& childNode : nodeToRender.getChildren()) {
            ImGui::PushID(i++);

            // 1.) Initialize the root children nodes if they are directories, and give them render flags.
            ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
            if (childNode.isDirectory() && childNode.directoryEntryExists()) {
                // If the node is a directory, initialize it - if it doesn't have children, it's a leaf.
                childNode.init();
                if (!childNode.childrenExist()) flag |= ImGuiTreeNodeFlags_Leaf;
            }
            else {
                // If the node represents a file, it's a leaf.
                flag |= ImGuiTreeNodeFlags_Leaf;
            }

            // 2.) Render root children and listen for events on those nodes.
            ImGui::PushFont(UIManager::getInstance()->GetIconFont());
            std::string iconCode = chooseIconCode(childNode);

            bool isNodeOpen = ImGui::TreeNodeEx((iconCode + " " + childNode.getName() + "##list").c_str(), flag);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && childNode.isDirectory() && childNode.directoryEntryExists()) {
                FileIconView::setCurrentNode(&childNode);
            }
            DragAndDropUtils::attachFileTreeNodeSource(&childNode);
            if (childNode.isDirectory()) {
                DragAndDropUtils::attachFileMoveTarget(&childNode);
            }
            if (isNodeOpen) {
                ImGui::PopFont();

                childNode.setIsOpen(true);

                if (childNode.isDirectory()) {
                    renderDescendants(childNode);
                }

                ImGui::TreePop();
            }
            else {
                childNode.setIsOpen(false);
                ImGui::PopFont();
            }

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete")) {
                    deletePopup = true;
                }
                ImGui::EndPopup();
            }
            renderDeleteConfirmationPrompt(childNode);

            ImGui::PopID();
        }
    }
}

void FileListView::renderRootNode(FileTreeNode& root) {
    // Renders the root node with the given driveName, initializes it when clicked, and renders the root children.
    ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;

    try {
        ImGui::PushFont(nullptr);

        bool isNodeOpen = ImGui::TreeNodeEx(root.getName().append("##list").c_str(), flag);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            FileIconView::setCurrentNode(&root);
        }
        DragAndDropUtils::attachFileMoveTarget(&root);
        if (isNodeOpen) {
            ImGui::PopFont();

            root.setIsOpen(true);
            renderDescendants(root);

            ImGui::TreePop();
        }
        else {
            root.setIsOpen(false);
            ImGui::PopFont();
        }
    }
    catch (const std::filesystem::filesystem_error&) {
        ImGui::TreePop();
    }
}

std::string FileListView::chooseIconCode(const FileTreeNode& node) {
    std::string iconCode;
    std::string filename = node.getName();

    try {
        if (node.isDirectory()) {
            iconCode = ICON_MD_FOLDER_OPEN;
        }
        else if (filename[0] == '.') iconCode = ICON_MD_SHORT_TEXT;
        else iconCode = chooseIconBasedOnExtension(filename);
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error accessing entry: " << e.what() << std::endl;
    }

    return iconCode;
}

std::string FileListView::chooseIconBasedOnExtension(const std::string& filename) {
    std::string fileExtension = FileExplorerUtils::getFileExtension(filename);

    std::string iconCode;
    if (fileExtension == "rar" || fileExtension == "zip") {
        iconCode = ICON_MD_FOLDER_ZIP;
    }
    else if (fileExtension == "exe") {
        iconCode = ICON_MD_TERMINAL;
    }
    else if (fileExtension == "txt") {
        iconCode = ICON_MD_SHORT_TEXT;
    }
    else if (fileExtension == "jpg" || fileExtension == "jpeg" || fileExtension == "png" || fileExtension == "PNG" || fileExtension == "bmp") {
        iconCode = ICON_MD_IMAGE;
    }
    else if (fileExtension == "pdf") {
        iconCode = ICON_MD_PICTURE_AS_PDF;
    }
    else if (fileExtension == "json") {
        iconCode = ICON_MD_DATA_OBJECT;
    }
    else if (fileExtension == "meta" || fileExtension == "config") {
        iconCode = ICON_MD_SETTINGS;
    }
    else if (fileExtension == "obj") {
        iconCode = ICON_MD_LANDSCAPE;
    }
    else {
        iconCode = ICON_MD_QUIZ;
    }

    return iconCode;
}

void FileListView::renderDeleteConfirmationPrompt(FileTreeNode& toDelete) {
    if (deletePopup) {
        ImGui::OpenPopup("File Delete Confirmation");
        deletePopup = false;
    }
    if (ImGui::BeginPopupModal("File Delete Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to delete this file?");
        ImGui::Text(toDelete.getPathString().c_str());
        ImGui::Spacing();

        // Calculate centering offset
        float windowWidth = ImGui::GetWindowSize().x;
        float buttonWidth = 120.0f * 2 + ImGui::GetStyle().ItemSpacing.x; // Two buttons + spacing
        float offsetX = (windowWidth - buttonWidth) * 0.5f;

        ImGui::SetCursorPosX(offsetX);
        if (ImGui::Button("Yes", ImVec2(120, 0))) {
            FileTree::getInstance()->deleteFile(toDelete);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}