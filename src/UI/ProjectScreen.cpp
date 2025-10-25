#include "ProjectScreen.h"

#include "imgui.h"
#include "UIManager.h"
#include "From-GDGRAP2/Debug.h"
#include "IconsMaterialDesign.h"
#include "Utilities/FileExplorer/FileTree.h"
#include "Utilities/FileExplorer/FileExplorerConstants.h"

#include <fstream>
#include <iostream>

ProjectScreen::ProjectScreen() : AUIScreen(UINames::PROJECT_SCREEN)
{
    if (create_directory(FileExplorerConstants::ASSETS_DIR))
    {
        std::cout << "Assets directory created" << std::endl;
    }
    else
    {
        if (exists(FileExplorerConstants::ASSETS_DIR) && is_directory(FileExplorerConstants::ASSETS_DIR)) {
            std::cout << "Assets directory already exists" << std::endl;
        }
        else {
            std::cerr << "Failed to create Assets directory" << std::endl;
        }
    }

    FileTree::getInstance()->populateFileMap();
    directory_entry dirEnt(FileExplorerConstants::ASSETS_DIR);
    FileTreeNode root(dirEnt);
    root.init();
    FileTree::getInstance()->setRoot(root);
}

ProjectScreen::~ProjectScreen()
= default;

void ProjectScreen::drawUI()
{
    ImGui::PushFont(nullptr);
	if (ImGui::Begin(FileExplorerConstants::PANEL_NAME, nullptr, UISettings::GlobalWindowFlags))
	{
        renderRootNode(FileTree::getInstance()->getRoot());
	}
	ImGui::End();
    ImGui::PopFont();
}

std::string ProjectScreen::getFileExtension(const std::string& filename) {
    std::stringstream ss(filename);
    std::string item;
    std::vector<std::string> result;

    // Split by '.'
    while (std::getline(ss, item, '.')) {
        result.push_back(item);
    }

    return result.back();
}

std::string ProjectScreen::chooseIconCode(const FileTreeNode& node) {
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

std::string ProjectScreen::chooseIconBasedOnExtension(const std::string& filename) {
    std::string fileExtension = getFileExtension(filename);

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

static bool deletePopup = false;

void renderDeleteConfirmationPrompt(FileTreeNode& toDelete) {
    if (deletePopup) {
        ImGui::OpenPopup("File Delete Confirmation");
        deletePopup = false;
    }
    if (ImGui::BeginPopupModal("File Delete Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to delete this file?");
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


void ProjectScreen::popupWindowNode(FileTreeNode& node) {

    if (ImGui::BeginPopup("FileTreeNodePopup")) {
        if (ImGui::MenuItem(ICON_MD_ARROW_RIGHT "Run")) {
            FileTree::getInstance()->openFile(node.getPathString());
        }
        if (ImGui::MenuItem(ICON_MD_CONTENT_COPY "Copy")) {
            FileTree::getInstance()->copyNodeSelection(node);
        }
        if (ImGui::MenuItem(ICON_MD_CONTENT_PASTE "Paste")) {
            FileTree::getInstance()->copyFile(node);
        }
        if (ImGui::MenuItem(ICON_MD_DELETE "Delete")) {
            deletePopup = true;
        }

        ImGui::EndPopup();
    }

}

void ProjectScreen::renderDescendants(FileTreeNode& root) {
    if (root.getIsOpen()) {
        for (auto& rootChild : root.getChildren()) { //root.children

            // 1.) Initialize the root children nodes if they are directories, and give them render flags.
            ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_None;
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
            if (ImGui::TreeNodeEx((iconCode + " " + rootChild.getName()).c_str(), flag)) {
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

void ProjectScreen::renderRootNode(FileTreeNode& root) {
    // Renders the root node with the given driveName, initializes it when clicked, and renders the root children.
    ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_None;

    try {
        ImGui::PushFont(nullptr);
        if (ImGui::TreeNodeEx(root.getName().c_str(), flag)) {
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