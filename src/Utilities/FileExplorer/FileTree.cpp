#include "FileTree.h"

#include "FileExplorerConstants.h"
#include "../FileUtils.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <imgui.h>

FileTree* FileTree::instance = nullptr;

FileTree::FileTree(FileTreeNode& rootNode) : root(rootNode)
{ 

}

FileTree* FileTree::getInstance() {
    if (instance == nullptr) {
        directory_entry dirEnt(FileUtils::getProjectFolderPath());
        FileTreeNode* newRoot = new FileTreeNode(dirEnt);
        newRoot->init();
        instance = new FileTree(*newRoot);
    }
    return instance;
}

void FileTree::setRoot() {
    directory_entry dirEnt(FileUtils::getProjectFolderPath());
    FileTreeNode* newRoot = new FileTreeNode(dirEnt);
    newRoot->init();
    FileTree::root = *newRoot;
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

bool FileTree::createDirectory(FileTreeNode& parentNode, std::string dirName) {
    bool status = std::filesystem::create_directory(parentNode.getDirectoryEntry().path() / dirName);

    if (status) {
        directory_entry newDir(parentNode.getDirectoryEntry().path() / dirName);
        FileTreeNode newTreeNode(newDir);

        newTreeNode.setParent(&parentNode);
        parentNode.addChild(newTreeNode);
    }

    return status;
}

FileTreeNode& FileTree::getRoot() {
    return root;
}