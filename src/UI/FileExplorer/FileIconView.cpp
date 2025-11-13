#include "FileIconView.h"

#include "imgui.h"
#include "Utilities/FileExplorer/FileExplorerConstants.h"
#include "Utilities/FileExplorer/FileExplorerUtils.h"
#include "Utilities/FileExplorer/FileTree.h"
#include "UI/IconsMaterialDesign.h"
#include "UI/UIManager.h"

FileTreeNode FileIconView::currentNode = FileTree::getInstance()->getRoot();

FileIconView::FileIconView() {

}

void FileIconView::drawUI() {
    ImGui::PushFont(nullptr);
    renderCurrentNode();
    ImGui::PopFont();
}

void FileIconView::renderCurrentNode() {

    for (auto& rootChild : currentNode.getChildren()) {
        if (ImGui::Button(chooseIconCode(rootChild).append("##").append(rootChild.getPathString()).c_str()))
        {
            if (rootChild.isDirectory() && rootChild.directoryEntryExists())
            {
                if (!rootChild.getIsInitialized())
                {
                    rootChild.init();
                }
                setCurrentNode(rootChild);
            }
        }
        ImGui::Text(rootChild.getName().c_str());
    }

}

std::string FileIconView::chooseIconCode(const FileTreeNode& node) {
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

std::string FileIconView::chooseIconBasedOnExtension(const std::string& filename) {
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

std::string FileIconView::getRootNodeRelPath() {
    return FileTree::getInstance()->getRoot().getPathString();
}

void FileIconView::setCurrentNode(FileTreeNode& node) {
    currentNode = node;
}
