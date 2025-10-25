#pragma once

#include "FileTree.h"

#include <string>

class FileIcon {
public:
    FileIcon(float x, float y, std::string& name, FileTreeNode& node);

    void LoadTexture(const char* path);
    void loadTextureBasedOnFile(FileTreeNode& node);
    static void setIconSize(float width, float height);
    static float getWidth();
    static float getHeight();
    bool renderIconWithName(bool render);
    static bool isFileOpened();
    static bool setFileOpenedStatus(bool status);

private:
    bool renderImageButton();
    static bool fileOpened;

    static int statID;
    std::string iconId;
    float x, y;
    std::string name, truncatedName;
    static float width, height;
    FileTreeNode& node;
};