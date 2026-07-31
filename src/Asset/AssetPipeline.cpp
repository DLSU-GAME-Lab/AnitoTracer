#include "AssetPipeline.hpp"

#define ASSETTYPE_MODEL "MODEL"
#define ASSETTYPE_TEXTURE "TEXTURE"

#include "AssetLoading/BatchLoader.h"

#include "Models/ModelManager.hpp"

AssetPipeline::AssetPipeline() {
	gbe::AssetType::register_value(ASSETTYPE_MODEL);
	gbe::AssetType::register_value(ASSETTYPE_TEXTURE);

    gbe::BatchLoader::RegisterCategoryDefault(
        "Mesh",
        { ".obj", ".fbx" },
        ".gbe",
        [](const fs::path& path) {
            ModelManager::GetInstance().LoadModel(path.string());
        },
        [](gbe::BaseImportData& meta, const fs::path& src) { 
            meta.assetId = src.filename().string();
            meta.assetType = ASSETTYPE_MODEL;
        },
        false,
        gbe::BatchLoader::MetaNamingStrategy::AppendToFilename
    );
}

void AssetPipeline::LoadAssetsFolder()
{
    gbe::BatchLoader::ReloadDirectory("Assets/"); // Default Assets Auto loading
}
