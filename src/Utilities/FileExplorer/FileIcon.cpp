#include "FileIcon.h"
#include "FileTree.h"

#include <string>
#include <stb_image.h>
#include <Utilities/FileUtils.h>
#include "Assets/Texture.hpp"
#include "Vulkan/Buffer.hpp"

FileIcon::FileIcon(float x, float y, std::string& name, FileTreeNode& node)
    : x(x), y(y), name(name), node(node) {

}

void FileIcon::LoadTexture(const char* path) {
    Assets::Texture iconTex = Assets::Texture::LoadTexture(FileUtils::getAssetsFolderPath().generic_string() + path, Vulkan::SamplerConfig());
}
void FileIcon::loadTextureBasedOnFile(FileTreeNode& node) {
    const std::string test = "Test";
}
void FileIcon::setIconSize(float width, float height) {
    const std::string test = "Test";
}
float FileIcon::getWidth() {
    return 0;
}
float FileIcon::getHeight() {
    return 0;
}

bool FileIcon::renderIconWithName(bool render) {
    return true;
}
bool FileIcon::isFileOpened() {
    return true;
}
bool FileIcon::setFileOpenedStatus(bool status) {
    return true;
}

bool FileIcon::renderImageButton() {
    return true;
}