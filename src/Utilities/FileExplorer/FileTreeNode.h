#pragma once

#include <filesystem>

using namespace std::filesystem;

class FileTreeNode {
public:
    FileTreeNode();

    explicit FileTreeNode(const directory_entry& directoryEntry);

    void init();

    // getters
    bool getIsOpen() const;
    bool getIsInitialized() const;
    FileTreeNode* getParent() const;
    std::vector<FileTreeNode>& getChildren();
    directory_entry& getDirectoryEntry();

    // setters
    void setIsOpen(bool isOpen);
    void setParent(FileTreeNode* parent);

    // node processes
    void addChild(const FileTreeNode& child);

    // utilities
    std::string getPathString() const;
    std::string getName() const;
    bool isDirectory() const;
    bool directoryEntryExists() const;
    bool childrenExist() const;

    // operators
    bool operator==(const FileTreeNode& other) const;

private:
    bool isOpen;
    bool isInitialized;
    FileTreeNode* parent;
    std::vector<FileTreeNode> children;
    directory_entry directoryEntry;
};
