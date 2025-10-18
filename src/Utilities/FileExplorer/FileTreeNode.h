#pragma once

#include <filesystem>

#include "FileIcon.h"

using namespace std::filesystem;

class FileTreeNode {
public:
    FileTreeNode();

    explicit FileTreeNode(const directory_entry& directoryEntry);

    void init();

    // getters
    bool isOpen() const;
    bool isInitialized() const;
    bool isIconInitialized() const;
    FileTreeNode* getParent() const;
    std::vector<FileTreeNode>& getChildren();
    directory_entry& getDirectoryEntry();
    FileIcon* getIcon() const;

    // setters
    void open();
    void close();
    void setInitializedStatus(bool status);
    void setDirectoryEntry(const directory_entry& directoryEntry);
    void setIcon(FileIcon* icon);

    // node processes
    void initializeIcon();
    void addChild(const FileTreeNode& child);

    // utilities
    std::string getPathString() const;
    std::string getName() const;
    bool isDirectory() const;
    bool direntExists() const;
    bool childrenExist() const;

private:
    bool isOpen;
    bool isInitialized;
    bool isIconInitialized;
    FileTreeNode* parent;
    std::vector<FileTreeNode> children;
    directory_entry directoryEntry;
    FileIcon* icon;
};
