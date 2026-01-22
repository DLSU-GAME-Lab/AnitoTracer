#pragma once

#include <unordered_map>

#include "FileTreeNode.h"

class FileTree
{
public:
    FileTree(const FileTree&) = delete;
    FileTree& operator=(const FileTree&) = delete;

    // singleton instancing and initialization
    static FileTree* getInstance();

    // filesystem indexing
    void setRoot();
    
    // file manipulation
    void deleteFile(FileTreeNode& toDelete);
    bool createDirectory(FileTreeNode& parentNode, std::string dirName);

    // getters
    FileTreeNode& getRoot();

private:
    static FileTree* instance;
    FileTree(FileTreeNode& rootNode);

    FileTreeNode& root;
};