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

FileIconView::FileIconView() : currentNode(&(FileTree::getInstance()->getRoot())), currTexId((ImTextureID)0), iconMap({}) {

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
    TextureLibrary::getInstance()->addTexture("folderIcon", FileUtils::getAssetsFolderPath().generic_string() + "/textures/UI/Icons/folder.png");
    TextureLibrary::getInstance()->addTexture("imageIcon", FileUtils::getAssetsFolderPath().generic_string() + "/textures/UI/Icons/gallery.png");
    TextureLibrary::getInstance()->addTexture("modelIcon", FileUtils::getAssetsFolderPath().generic_string() + "/textures/UI/Icons/model.png");
    TextureLibrary::getInstance()->addTexture("textIcon", FileUtils::getAssetsFolderPath().generic_string() + "/textures/UI/Icons/note.png");

    Assets::TextureImage* folderTex = new Assets::TextureImage(*UIManager::getInstance()->commandPool, TextureLibrary::getInstance()->getTexture("folderIcon"));
    VkDescriptorSet folderVds = ImGui_ImplVulkan_AddTexture(folderTex->Sampler().Handle(), folderTex->ImageView().Handle(), VK_IMAGE_LAYOUT_GENERAL);
    iconMap.insert_or_assign("folderIcon", std::make_shared<ImTextureID>((ImTextureID)folderVds));

    Assets::TextureImage* imageTex = new Assets::TextureImage(*UIManager::getInstance()->commandPool, TextureLibrary::getInstance()->getTexture("imageIcon"));
    VkDescriptorSet imageVds = ImGui_ImplVulkan_AddTexture(imageTex->Sampler().Handle(), imageTex->ImageView().Handle(), VK_IMAGE_LAYOUT_GENERAL);
    iconMap.insert_or_assign("imageIcon", std::make_shared<ImTextureID>((ImTextureID)imageVds));

    Assets::TextureImage* modelTex = new Assets::TextureImage(*UIManager::getInstance()->commandPool, TextureLibrary::getInstance()->getTexture("modelIcon"));
    VkDescriptorSet modelVds = ImGui_ImplVulkan_AddTexture(modelTex->Sampler().Handle(), modelTex->ImageView().Handle(), VK_IMAGE_LAYOUT_GENERAL);
    iconMap.insert_or_assign("modelIcon", std::make_shared<ImTextureID>((ImTextureID)modelVds));

    Assets::TextureImage* textTex = new Assets::TextureImage(*UIManager::getInstance()->commandPool, TextureLibrary::getInstance()->getTexture("textIcon"));
    VkDescriptorSet textVds = ImGui_ImplVulkan_AddTexture(textTex->Sampler().Handle(), textTex->ImageView().Handle(), VK_IMAGE_LAYOUT_GENERAL);
    iconMap.insert_or_assign("textIcon", std::make_shared<ImTextureID>((ImTextureID)textVds));

    currTexId = *iconMap["modelIcon"];
}

void FileIconView::renderCurrentNodeChildrenIcons() {

    float iconWidth = 120;

    int i = 0;
    for (FileTreeNode &childNode : currentNode->getChildren()) {
        ImGui::PushID(i++);

        //ImGui::PushFont(UIManager::getInstance()->GetIconFont(), 30.0f);

        ImGui::BeginGroup();
        chooseIconCode(childNode);

        if (ImGui::ImageButton(childNode.getPathString().c_str(), currTexId, ImVec2(iconWidth, iconWidth)))
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
        

        std::string truncatedText = childNode.getName().c_str();
        const float textWidth = ImGui::CalcTextSize(truncatedText.c_str(), nullptr, true).x;
        if (textWidth > 120) {
            truncatedText = truncatedText.substr(0, 15) + "...";
        }
        ImGui::Text(truncatedText.c_str());
        DragAndDropUtils::attachFileTreeNodeSource(&childNode);
        if (childNode.isDirectory()) {
            DragAndDropUtils::attachFileMoveTarget(childNode);
        }
        
        renderNewFolderSetupPrompt(childNode);
        renderDeleteConfirmationPrompt(childNode);
        ImGui::PopFont();

        ImGui::PopID();

        float availableRegionX = ImGui::GetContentRegionAvail().x;

        ImGui::EndGroup();

        if (availableRegionX > iconWidth * 2.2)
        {
            ImGui::SameLine();
        }
        else {
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
            currTexId = *iconMap["folderIcon"];
        }
        else if (filename[0] == '.') {
            iconCode = ICON_MD_SHORT_TEXT;
            currTexId = *iconMap["textIcon"];
        }
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
        currTexId = *iconMap["textIcon"];
    }
    else if (fileExtension == "jpg" || fileExtension == "jpeg" || fileExtension == "png" || fileExtension == "PNG" || fileExtension == "bmp") {
        iconCode = ICON_MD_IMAGE;
        currTexId = *iconMap["imageIcon"];
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
        currTexId = *iconMap["modelIcon"];
    }
    else {
        iconCode = ICON_MD_QUIZ;
        currTexId = *iconMap["textIcon"];
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