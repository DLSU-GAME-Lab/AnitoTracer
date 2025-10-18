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
    static void setRoot(const FileTreeNode& root);

    // filesystem indexing
    static void populateFileMap();
    static bool getFilesystemIndexStatus();

    // file manipulation
    static void copyNodeSelection(const FileTreeNode& node);
    static void copyFile(FileTreeNode& dest);
    static void openFile(const std::string& filePath);
    static void deleteFile(FileTreeNode& toDelete);

    // getters
    static FileTreeNode& getRoot();
    static std::unordered_map<std::string, std::vector<std::filesystem::path>>& getFileMap();

private:
    static FileTree* instance;
    FileTree() = default;

    inline static std::unordered_map<std::string, std::vector<std::filesystem::path>> fileMap;
    static FileTreeNode copiedNode;
    static FileTreeNode root;
    static bool filesystemIndexed;
};