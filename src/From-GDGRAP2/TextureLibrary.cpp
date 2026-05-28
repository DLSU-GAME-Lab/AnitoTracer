#include "TextureLibrary.h"

#include <stb_image.h>

#include "Assets/Texture.hpp"
#include "Utilities/Exception.hpp"
#include "Utilities/FileUtils.h"
#include "Debug.h"

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

Assets::Texture TextureLibrary::getTextureById(int textureId)
{
	std::vector<Assets::Texture> textureList;
	for (auto& texture : this->textureList) {
		textureList.push_back(*texture);
	}

	return textureList[textureId];
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

bool TextureLibrary::loadTextureFromFile(int& textureId)
{
	std::string texturePath;
	std::string fileName;

	if (!FileUtils::getTextureFilePath(texturePath, fileName))
	{
		Debug::Log("Cancelled loading texture from path: " + texturePath);

		return false;
	}

	if (!texturePath.empty()) {
		Debug::Log("Loading texture from path: " + texturePath);
	}

	this->addTexture(fileName, texturePath);

	textureId = this->getTextureId(fileName);
	return true;
	

}

void TextureLibrary::loadTextureLibrary(TextureMap textureMap, TextureList textureList)
{
	this->textureMap = textureMap;
	this->textureList = textureList;
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
}

TextureLibrary::~TextureLibrary()
{
}

TextureLibrary* TextureLibrary::getInstance()
{
	return sharedInstance;
}

// Save: produce JSON array of textures as name + path
//json texturesJson = json::array();
//for (auto& kv : texLib->textureMap) {
//    json t;
//    t["name"] = kv.first;
//    t["filePath"] = kv.second ? kv.second->GetSourcePath() : "";
//    texturesJson.push_back(t);
//}
//scene["textures"] = texturesJson;
//
// Load: reconstruct containers
//TextureLibrary::TextureMap newMap;
//TextureLibrary::TextureList newList;
//for (auto& t : scene["textures"]) {
//    std::string name = t["name"];
//    std::string path = t["filePath"];
//    auto texPtr = std::make_shared<Assets::Texture>(Assets::Texture::LoadTexture(path, Vulkan::SamplerConfig()));
//    newMap.emplace(name, texPtr);
//    newList.push_back(texPtr);
//}
//texLib->loadTextureLibrary(std::move(newMap), std::move(newList));
