#include "FileTree.h"

#include "FileExplorerConstants.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <imgui.h>

FileTree* FileTree::instance = nullptr;

FileTree::FileTree()
{
    filesystemIndexed = false;
}

FileTree* FileTree::getInstance() {
    if (instance == nullptr) {
        instance = new FileTree();
    }
    return instance;
}

void FileTree::setRoot(const FileTreeNode& root) {
    FileTree::root = root;
}


void FileTree::populateFileMap() {
    try {
        std::filesystem::directory_entry start_dir(FileExplorerConstants::ASSETS_DIR);
        for (const auto& entry : std::filesystem::recursive_directory_iterator(start_dir)) {
            fileMap[entry.path().filename().string()].push_back(entry.path());
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error accessing entry: " << e.what() << std::endl;
    }

    filesystemIndexed = true;
}

bool FileTree::getFilesystemIndexStatus() {
    return filesystemIndexed;
}

void FileTree::copyNodeSelection(const FileTreeNode& node) {
    copiedNode = node;
}

void FileTree::copyFile(FileTreeNode& dest) {
    std::string destination_directory = dest.getPathString();

    // Append the file name from the source path to construct the full destination path.
    std::string copy_source_path = copiedNode.getPathString();
    std::string paste_destination_path = destination_directory + "/" + std::filesystem::path(copy_source_path).filename().string();

    copiedNode.setDirectoryEntry(std::filesystem::directory_entry(paste_destination_path));

    dest.addChild(copiedNode);

    try
    {
        std::filesystem::copy(copy_source_path, paste_destination_path);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        ImGui::Text("Error copying file: %s", e.what());
    }
}

void FileTree::openFile(const std::string& filePath) {
    /* Opens the file in a default program. */
    std::string command = "open \"" + filePath + "\"";
    system(command.c_str());
}

void FileTree::deleteFile(FileTreeNode& toDelete) {
    path fileToDelete = toDelete.getPathString();

    try {

        // Check if the file exists.
        if (exists(fileToDelete)) {

            // Attempt to delete the file.
            if (remove(fileToDelete)) {
                FileTreeNode* parent = toDelete.getParent();
                auto& siblings = parent->getChildren();

                // Remove the 'toDelete' node from the 'siblings' list.
                auto newEnd = std::remove(siblings.begin(), siblings.end(), toDelete);
                siblings.erase(newEnd, siblings.end());

            }
            else {
                std::cout << "File could not be deleted.\n";
            }

        }
        else {
            std::cout << "File does not exist.\n";
        }

    }
    catch (const filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
    }
}

FileTreeNode& FileTree::getRoot() {
    return root;
}

std::unordered_map<std::string, std::vector<std::filesystem::path> >& FileTree::getFileMap() {
    return fileMap;
}