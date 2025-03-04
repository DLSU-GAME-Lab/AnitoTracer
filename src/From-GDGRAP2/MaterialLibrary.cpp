#include "MaterialLibrary.h"

#include "Assets/Material.hpp"

MaterialLibrary* MaterialLibrary::sharedInstance = NULL;
void MaterialLibrary::addMaterial(std::wstring materialName, std::shared_ptr<Assets::Material> material)
{
	this->materialMap[materialName] = material;
}

void MaterialLibrary::deleteMaterial(std::wstring materialName)
{
	this->materialMap.erase(materialName);
}

std::shared_ptr<Assets::Material> MaterialLibrary::getMaterial(std::wstring materialName)
{
	return this->materialMap[materialName];
}

MaterialLibrary::MaterialLibrary() {
	this->addMaterial(L"White", Assets::Material::Lambertian(glm::vec3(0.73f, 0.73f, 0.73f)));
	this->addMaterial(L"BaseMaterial", Assets::Material::Lambertian(glm::vec3(0.f, 0.f, 0.f )));
	this->addMaterial(L"Mirror", Assets::Material::Metallic(glm::vec3(0.21f, 0.43f, 0.71f), 0.0f));
}

MaterialLibrary::~MaterialLibrary()
{
	this->materialMap.clear();
}

MaterialLibrary* MaterialLibrary::getInstance() {
	return sharedInstance;
}

void MaterialLibrary::initialize() {
	sharedInstance = new MaterialLibrary();
}
void MaterialLibrary::destroy() {
	delete sharedInstance;
}
