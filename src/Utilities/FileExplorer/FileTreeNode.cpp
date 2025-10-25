#include "FileTreeNode.h"

#include <iostream>

FileTreeNode::FileTreeNode() : isOpen(false), isInitialized(false), parent(nullptr) {}

FileTreeNode::FileTreeNode(const std::filesystem::directory_entry& directoryEntry) : isOpen(false), isInitialized(false), directoryEntry(directoryEntry), parent(nullptr) {}

bool FileTreeNode::getIsOpen() const {
    return isOpen;
}

void FileTreeNode::open() {
    isOpen = true;
}

void FileTreeNode::close() {
    isOpen = false;
}

bool FileTreeNode::getIsInitialized() const {
    return isInitialized;
}

void FileTreeNode::setInitializedStatus(const bool status) {
    isInitialized = status;
}

std::filesystem::directory_entry& FileTreeNode::getDirectoryEntry() {
    return directoryEntry;
}

void FileTreeNode::setDirectoryEntry(const directory_entry& directoryEntry) {
    this->directoryEntry = directoryEntry;
}

std::string FileTreeNode::getPathString() const {
    return directoryEntry.path().string();
}

std::string FileTreeNode::getName() const {
    return directoryEntry.path().filename().string();
}

std::vector<FileTreeNode>& FileTreeNode::getChildren() {
    return children;
}

void FileTreeNode::addChild(const FileTreeNode& child) {
    children.push_back(child);
}

bool FileTreeNode::isDirectory() const {
    return directoryEntry.is_directory();
}

bool FileTreeNode::directoryEntryExists() const {
    return directoryEntry.exists();
}

bool FileTreeNode::childrenExist() const {
    return !children.empty();
}

FileTreeNode* FileTreeNode::getParent() const {
    return parent;
}

void FileTreeNode::init() {
    if (!isInitialized) {
        try {
            std::filesystem::directory_iterator directoryIterator(directoryEntry, std::filesystem::directory_options::skip_permission_denied);

            for (const auto& dir_entry : directoryIterator) {

                if (dir_entry.path().filename() == ".Trash") continue;

                std::cout << dir_entry.path().string() << std::endl;
                FileTreeNode child(dir_entry);
                child.parent = this; // possible memory leak
                children.push_back(child);
            }

            isInitialized = true;
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error accessing entry: " << e.what() << std::endl;
        }
    }
}

bool FileTreeNode::operator==(const FileTreeNode& other) const {
    return directoryEntry == other.directoryEntry;
}