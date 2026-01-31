#include "FileIconView.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "Utilities/DragAndDrop/DragAndDropUtils.h"
#include "Utilities/FileExplorer/FileExplorerConstants.h"
#include "Utilities/FileExplorer/FileExplorerUtils.h"
#include "Utilities/FileExplorer/FileTree.h"
#include "UI/IconsMaterialDesign.h"
#include "UI/UIManager.h"

#include "imgui_impl_vulkan.h"
#include "Assets/TextureImage.hpp"
#include "Assets/Texture.hpp"
#include <From-GDGRAP2/TextureLibrary.h>
#include <UI/ButtonTexture.hpp>
#include <Utilities/FileUtils.h>

static bool newFolderPopup = false;
static bool deletePopup = false;
FileIconView* FileIconView::instance = nullptr;

FileIconView::FileIconView() : currentNode(&(FileTree::getInstance()->getRoot())), currTexId((ImTextureID)0) {

}

FileIconView* FileIconView::getInstance() {
    if (instance == nullptr) {
        instance = new FileIconView();
    }
    return instance;
}

void FileIconView::drawUI() {
    
    renderCurrentNodeChildrenIcons();

}

void FileIconView::initButtonTexture() {
    TextureLibrary::getInstance()->addTexture("folderIcon", FileUtils::getAssetsFolderPath().generic_string() + "/textures/Vulkan.png");

    Assets::TextureImage* textureimg = new Assets::TextureImage(*UIManager::getInstance()->commandPool, TextureLibrary::getInstance()->getTexture("folderIcon"));
    VkDescriptorSet vds = ImGui_ImplVulkan_AddTexture(textureimg->Sampler().Handle(), textureimg->ImageView().Handle(), VK_IMAGE_LAYOUT_GENERAL);
    currTexId = (ImTextureID)vds;
}

void FileIconView::renderCurrentNodeChildrenIcons() {

    float farLeftPosX = ImGui::GetCursorPosX();
    float currentPosX = farLeftPosX;

    int i = 0;
    for (FileTreeNode &childNode : currentNode->getChildren()) {
        ImGui::PushID(i++);

        //ImGui::PushFont(UIManager::getInstance()->GetIconFont(), 30.0f);

        ImGui::BeginGroup();

        if (ImGui::ImageButton(chooseIconCode(childNode).append("##").append(childNode.getPathString()).c_str(), currTexId, ImVec2(120, 120)))
        {
            if (childNode.isDirectory() && childNode.directoryEntryExists())
            {
                if (!childNode.getIsInitialized())
                {
                    childNode.init();
                }
                setCurrentNode(&childNode);
            }
        }

        //ImGui::PopFont();

        ImGui::PushFont(nullptr);
        DragAndDropUtils::attachFileTreeNodeSource(&childNode);
        if (childNode.isDirectory()) {
            DragAndDropUtils::attachFileMoveTarget(childNode);
        }
        
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("New Folder")) {
                newFolderPopup = true;
            }
            else if (ImGui::MenuItem("Delete")) {
                deletePopup = true;
            }
            ImGui::EndPopup();
        }
        
        ImGui::Text(childNode.getName().c_str());
        DragAndDropUtils::attachFileTreeNodeSource(&childNode);
        if (childNode.isDirectory()) {
            DragAndDropUtils::attachFileMoveTarget(childNode);
        }
        
        renderNewFolderSetupPrompt(childNode);
        renderDeleteConfirmationPrompt(childNode);
        ImGui::PopFont();

        ImGui::PopID();

        ImGui::EndGroup();

        
        if (currentPosX + 150 < ImGui::GetContentRegionAvail().x - 100)
        {
            currentPosX += 150;
            ImGui::SameLine();
        }
        else {
            currentPosX = farLeftPosX;
            ImGui::NewLine();
        }
        
    }

    //ImGui_ImplVulkan_RemoveTexture(vds);
    //delete textureimg;

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
    else if (fileExtension == "obj") {
        iconCode = ICON_MD_LANDSCAPE;
    }
    else {
        iconCode = ICON_MD_QUIZ;
    }

    return iconCode;
}

void FileIconView::setCurrentNode(FileTreeNode* node) {
    currentNode = node;
}

void FileIconView::renderDeleteConfirmationPrompt(FileTreeNode& toDelete) {
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

void FileIconView::renderNewFolderSetupPrompt(FileTreeNode& targetNode) {
    if (newFolderPopup) {
        ImGui::OpenPopup("Setup New Folder");
        newFolderPopup = false;
    }
    if (ImGui::BeginPopupModal("Setup New Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // Calculate centering offset
        float windowWidth = ImGui::GetWindowSize().x;
        float buttonWidth = 120.0f * 2 + ImGui::GetStyle().ItemSpacing.x; // Two buttons + spacing
        float offsetX = (windowWidth - buttonWidth) * 0.5f;

        static std::string folderName = "";
        ImGui::InputText("Name", &folderName);

        ImGui::SetCursorPosX(offsetX);
        if (ImGui::Button("Create Folder", ImVec2(120, 0))) {
            if (targetNode.isDirectory()) {
                FileTree::getInstance()->createDirectory(targetNode, folderName);
            }
            else {
                FileTree::getInstance()->createDirectory(*targetNode.getParent(), folderName);
            }
            folderName = "";
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}