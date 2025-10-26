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

    void setRoot(const FileTreeNode& root);

    // filesystem indexing
    void populateFileMap();
    bool getFilesystemIndexStatus();

    // file manipulation
    void copyNodeSelection(const FileTreeNode& node);
    void copyFile(FileTreeNode& dest);
    void openFile(const std::string& filePath);
    void deleteFile(FileTreeNode& toDelete);

    // getters
    FileTreeNode& getRoot();
    std::unordered_map<std::string, std::vector<std::filesystem::path>>& getFileMap();

private:
    static FileTree* instance;
    FileTree();

    inline static std::unordered_map<std::string, std::vector<std::filesystem::path>> fileMap;
    FileTreeNode copiedNode;
    FileTreeNode root;
    bool filesystemIndexed;
};