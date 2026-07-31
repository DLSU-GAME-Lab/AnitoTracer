#include "AssetPipeline.hpp"
#include "AssetPipeline.hpp"

#define ASSETTYPE_MODEL "MODEL"
#define ASSETTYPE_TEXTURE "TEXTURE"

#include "AssetLoading/BatchLoader.hpp"

#include "Models/ModelManager.hpp"

AssetPipeline::AssetPipeline() {
	gbe::AssetType::register_value(ASSETTYPE_MODEL);
	gbe::AssetType::register_value(ASSETTYPE_TEXTURE);

    gbe::BatchLoader::RegisterCategoryDefault(
        ASSETTYPE_MODEL,
        { ".obj", ".fbx" },
        ".ani",
        [](const fs::path& path) {
            //Meta file preprocessing goes here, usually empty tho.
        },
        [](gbe::IAsset& meta, const fs::path& src) { 
            ModelManager::GetInstance().LoadModel(src.string()); //Connect asset system to asset loader
        },
        false,
        gbe::BatchLoader::MetaNamingStrategy::AppendToFilename
    );
}

void AssetPipeline::LoadFolder(std::filesystem::path folderpath)
{
    GetInstance();
    gbe::BatchLoader::ReloadDirectory(folderpath);
}
