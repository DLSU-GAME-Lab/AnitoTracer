#include "FileTree.h"

#include "FileExplorerConstants.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <imgui.h>

FileTree* FileTree::instance = nullptr;

FileTree::FileTree()
{ 
    setRoot();
}

FileTree* FileTree::getInstance() {
    if (instance == nullptr) {
        instance = new FileTree();
    }
    return instance;
}

void FileTree::setRoot() {
    directory_entry dirEnt(FileExplorerConstants::ASSETS_DIR);
    FileTreeNode root(dirEnt);
    root.init();
    FileTree::root = root;
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