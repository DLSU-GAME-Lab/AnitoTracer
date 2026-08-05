#pragma once

#include "AssetLoading/BatchLoader.hpp"
#include "IAsset.hpp"
#include "AssetRef.hpp"
#include "AutoSerializer_AssetRef.hpp"
#include "AssetLoading/AssetLoader.hpp"

#include "Organization/SingletonMacro.hpp"

#include <filesystem>

class AssetPipeline {
	SINGLETON_MACRO_CUSTOM(AssetPipeline);

public:
	static void LoadFolder(std::filesystem::path folderpath);
};