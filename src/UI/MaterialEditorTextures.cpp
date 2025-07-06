#include "MaterialEditorTextures.hpp"
#include "From-GDGRAP2/TextureLibrary.h"
#include "Assets/Texture.hpp"
#include "Assets/TextureImage.hpp"
#include "UIManager.h"
#include "RayTracer.hpp"

using namespace Assets;

MaterialEditorTextures* MaterialEditorTextures::sharedInstance = nullptr;

MaterialEditorTextures::MaterialEditorTextures()
{

}

MaterialEditorTextures::~MaterialEditorTextures()
{
}

void Assets::MaterialEditorTextures::initialize()
{
	if (sharedInstance == nullptr)
		sharedInstance = new MaterialEditorTextures;

	//TextureImage* textureimg;
	//textureimg = new Assets::TextureImage(*UIManager::getInstance()->commandPool, TextureLibrary::getInstance()->getTextureById(0));

	//Assets::ButtonTexture newButtonimg = Assets::ButtonTexture(textureimg);
	//sharedInstance->buttonTexture = newButtonimg;
}

MaterialEditorTextures* MaterialEditorTextures::GetInstance()
{
	if (sharedInstance == nullptr)
		sharedInstance = new MaterialEditorTextures;

	return sharedInstance;
}

ButtonTexture MaterialEditorTextures::getTexture()
{

	return this->buttonTexture;
}

void Assets::MaterialEditorTextures::setTexture(ButtonTexture* texture)
{

	this->buttonTexture = buttonTexture;
}

void Assets::MaterialEditorTextures::setTexture(int32_t textureId)
{
	TextureImage* textureimg;

	if (textureId == -1)
		textureimg = new Assets::TextureImage(*RayTracer::getInstance()->getCommandPool(), TextureLibrary::getInstance()->getTextureById(0));
	else
		textureimg = new Assets::TextureImage(*RayTracer::getInstance()->getCommandPool(), TextureLibrary::getInstance()->getTextureById(textureId));

	Assets::ButtonTexture newButtonimg = Assets::ButtonTexture(textureimg);
	this->buttonTexture = newButtonimg;

}
