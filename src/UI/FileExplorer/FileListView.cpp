#include "FileListView.h"

#include "imgui.h"
#include "Utilities/FileExplorer/FileExplorerConstants.h"
#include "Utilities/FileExplorer/FileExplorerUtils.h"
#include "Utilities/FileExplorer/FileTree.h"
#include "UI/IconsMaterialDesign.h"
#include "UI/UIManager.h"
#include "UI/FileExplorer/FileIconView.h"

FileListView::FileListView() {

}

void FileListView::drawUI() {
    ImGui::PushFont(nullptr);
    renderRootNode(FileTree::getInstance()->getRoot());
    ImGui::PopFont();
}

void FileListView::renderDescendants(FileTreeNode& root) {
    if (root.getIsOpen()) {
        for (auto& rootChild : root.getChildren()) { //root.children

            // 1.) Initialize the root children nodes if they are directories, and give them render flags.
            ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
            if (rootChild.isDirectory() && rootChild.directoryEntryExists()) {
                // If the node is a directory, initialize it - if it doesn't have children, it's a leaf.
                rootChild.init();
                if (!rootChild.childrenExist()) flag |= ImGuiTreeNodeFlags_Leaf;
            }
            else {
                // If the node represents a file, it's a leaf.
                flag |= ImGuiTreeNodeFlags_Leaf;
            }

            // 2.) Render root children and listen for events on those nodes.
            ImGui::PushFont(UIManager::getInstance()->GetIconFont());
            std::string iconCode = chooseIconCode(rootChild);

            bool isNodeOpen = ImGui::TreeNodeEx((iconCode + " " + rootChild.getName() + "##list").c_str(), flag);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && rootChild.isDirectory() && rootChild.directoryEntryExists()) {
                FileIconView::setCurrentNode(&rootChild);
            }
            if (isNodeOpen) {
                ImGui::PopFont();

                rootChild.setIsOpen(true);
                renderDescendants(rootChild);

                ImGui::TreePop();
            }
            else {
                rootChild.setIsOpen(false);
                ImGui::PopFont();
            }
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
    else {
        iconCode = ICON_MD_QUIZ;
    }

    return iconCode;
}
