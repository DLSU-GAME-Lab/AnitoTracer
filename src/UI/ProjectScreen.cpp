#include "ProjectScreen.h"

#include "imgui.h"
#include "UIManager.h"
#include "From-GDGRAP2/Debug.h"
#include "IconsMaterialDesign.h"
#include "Utilities/FileExplorer/FileTree.h"
#include "Utilities/FileExplorer/FileExplorerConstants.h"
#include "Utilities/FileExplorer/FileExplorerUtils.h"
#include "UI/FileExplorer/FileListView.h"
#include "UI/FileExplorer/FileIconView.h"

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
    FileIconView::setCurrentNode(&root);
}

ProjectScreen::~ProjectScreen()
= default;

void ProjectScreen::drawUI()
{
    if (ImGui::Begin(FileExplorerConstants::PANEL_NAME, nullptr, UISettings::GlobalWindowFlags))
    {
        if (ImGui::BeginTable("MyTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {

            ImGui::TableSetupColumn("Project Files");
            ImGui::TableSetupColumn(FileIconView::getRootNodeRelPath().c_str());

            ImGui::TableHeadersRow(); // Display table headers

            ImGui::TableNextRow(); // Only 1 row

            ImGui::TableSetColumnIndex(0);
            FileListView::drawUI();

            ImGui::TableSetColumnIndex(1);
            FileIconView::drawUI();

            ImGui::EndTable();
        }
    }
    ImGui::End();
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