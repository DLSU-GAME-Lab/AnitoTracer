#include "AssetPipeline.hpp"

#include "AssetLoading/BatchLoader.hpp"

#include "Models/ModelManager.hpp"

AssetPipeline::AssetPipeline() {
	gbe::AssetType::register_value(ASSETTYPE_MODEL);
	gbe::AssetType::register_value(ASSETTYPE_TEXTURE);

    gbe::BatchLoader::RegisterCategory<IModel>(
        ASSETTYPE_MODEL,
        { ".obj", ".fbx" },
        ".ani",
        [](const fs::path& src) { 
            return ModelManager::GetInstance().LoadModel(src.string()); //Connect asset system to asset loader
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
