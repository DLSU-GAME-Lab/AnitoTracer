#include "TextureLibrary.h"

#include <stb_image.h>

#include "Assets/Texture.hpp"
#include "Utilities/Exception.hpp"
#include "Utilities/FileUtils.h"

TextureLibrary* TextureLibrary::sharedInstance = nullptr;
void TextureLibrary::addTexture(const std::string& textureName, const std::string& fileName)
{
	std::shared_ptr<Assets::Texture> texture = std::make_shared<Assets::Texture>(Assets::Texture::LoadTexture(fileName, Vulkan::SamplerConfig()));

	this->textureMap.insert(std::make_pair(textureName, texture));
	this->textureList.push_back(texture);
}

void TextureLibrary::deleteTexture(std::string textureName)
{
	std::shared_ptr<Assets::Texture> texture = this->textureMap[textureName];

	int index = -1;
	for (int i = 0; i < this->textureList.size(); i++) {
		if (this->textureList[i] == texture) {
			index = i;
			break;
		}
	}

	if (index != -1) {
		this->textureList.erase(this->textureList.begin() + index);
	}

	this->textureMap.erase(textureName);
}

Assets::Texture TextureLibrary::getTexture(std::string textureName)
{
	return *this->textureMap[textureName];
}

int TextureLibrary::getTextureId(std::string textureName)
{
	std::shared_ptr<Assets::Texture> texture = this->textureMap[textureName];

	int index = -1;
	for (int i = 0; i < this->textureList.size(); i++) {
		if (this->textureList[i] == texture) {
			index = i;
			break;
		}
	}

	return index;
}

std::vector<Assets::Texture> TextureLibrary::getTextureLibraryList()
{
	std::vector<Assets::Texture> textureList;
	for (auto& texture : this->textureList) {
		textureList.push_back(*texture);
	}
	return textureList;
}

bool TextureLibrary::doesTextureExist(std::string textureName)
{
	auto search = this->textureMap.find(textureName);
	if (search != this->textureMap.end())
		return true;
	else
		return false;
	
}

void TextureLibrary::initialize()
{
	sharedInstance = new TextureLibrary();
}

void TextureLibrary::destroy()
{
	delete sharedInstance;
}

TextureLibrary::TextureLibrary()
{
	this->addTexture("white", FileUtils::getAssetsFolderPath().generic_string() + "/textures/white.png");
	this->addTexture("2k_mars", FileUtils::getAssetsFolderPath().generic_string() + "/textures/2k_mars.jpg");
	this->addTexture("2k_moon", FileUtils::getAssetsFolderPath().generic_string() + "/textures/2k_moon.jpg");
	this->addTexture("land_ocean_ice_cloud_2048", FileUtils::getAssetsFolderPath().generic_string() + "/textures/land_ocean_ice_cloud_2048.png");
	this->addTexture("checker", FileUtils::getAssetsFolderPath().generic_string() + "/textures/checker.jpg");
	this->addTexture("earthmap", FileUtils::getAssetsFolderPath().generic_string() + "/textures/earthmap.jpg");
	this->addTexture("file", FileUtils::getAssetsFolderPath().generic_string() + "/textures/file.png");
	this->addTexture("folder", FileUtils::getAssetsFolderPath().generic_string() + "/textures/folder.png");
	this->addTexture("ini", FileUtils::getAssetsFolderPath().generic_string() + "/textures/ini.png");

}

TextureLibrary::~TextureLibrary()
{
}

TextureLibrary* TextureLibrary::getInstance()
{
	return sharedInstance;
}
