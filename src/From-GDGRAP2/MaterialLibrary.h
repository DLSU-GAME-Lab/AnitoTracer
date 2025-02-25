#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace Assets
{
	struct Material;
}

class MaterialLibrary
{
	// TODO: Need Text Files for created materials.
private:
	typedef std::unordered_map <std::wstring, std::shared_ptr<Assets::Material>> MaterialMap;

	MaterialMap materialMap;

public:
	void addMaterial(std::wstring materialName, std::shared_ptr<Assets::Material> material);
	void deleteMaterial(std::wstring materialName);
	std::shared_ptr<Assets::Material> getMaterial(std::wstring materialName);

private:
	static MaterialLibrary* sharedInstance;

private:
	MaterialLibrary();
	~MaterialLibrary();
	MaterialLibrary(MaterialLibrary const&);
	MaterialLibrary& operator= (MaterialLibrary const&);

public:
	static MaterialLibrary* getInstance();
	static void initialize();
	static void destroy();
};