#pragma once

#include ANITO_SERIALIZATION_INCLUDES

#include "Meta/AssetRef_meta.hpp"
#include "Meta/IAsset_meta.hpp"
#include "Meta/GUID_meta.hpp"
#include "Meta/DynamicEnum_meta.hpp"

#include "Types/TypeConstants.hpp"

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