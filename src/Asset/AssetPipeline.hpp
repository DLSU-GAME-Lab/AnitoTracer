#pragma once

#include "AssetLoading/BatchLoader.hpp"
#include "IAsset.hpp"

#include "Organization/SingletonMacro.hpp"

#include <filesystem>

class AssetPipeline {
	SINGLETON_MACRO_CUSTOM(AssetPipeline);

public:
	static void LoadFolder(std::filesystem::path folderpath);
};