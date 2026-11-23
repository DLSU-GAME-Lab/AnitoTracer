#include "AssetPipeline.hpp"

#include "AssetLoading/BatchLoader.hpp"

#include "Models/ModelManager.hpp"

#include "AudioManager.hpp"

AssetPipeline::AssetPipeline() {
	gbe::AssetType::register_value(ASSETTYPE_MODEL);
	gbe::AssetType::register_value(ASSETTYPE_TEXTURE);
    gbe::AssetType::register_value(ASSETTYPE_AUDIO);

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

    gbe::BatchLoader::RegisterCategory<IAudioClip>(
        ASSETTYPE_AUDIO,
        { ".wav" },
        ".ani",
        [](const fs::path& src) { 
            return AudioManager::GetInstance().LoadClip(src.string());
        },
        false,
        gbe::BatchLoader::MetaNamingStrategy::AppendToFilename
    );
}

void AssetPipeline::IncludeFolder(std::filesystem::path folderpath)
{
    GetInstance();
    folderpath = std::filesystem::absolute(folderpath).lexically_normal();
    gbe::AssetDatabase::RegisterDirectory(folderpath);
    gbe::BatchLoader::ReloadDirectory(folderpath);
}

void AssetPipeline::IncludeProjectFolder(std::filesystem::path folderpath)
{
    GetInstance();
    folderpath = std::filesystem::absolute(folderpath).lexically_normal();
    gbe::AssetDatabase::BeginProjectLoad(folderpath);
    struct ProjectLoadScope {
        ~ProjectLoadScope() { gbe::AssetDatabase::EndProjectLoad(); }
    } scope;
    gbe::BatchLoader::ReloadDirectory(folderpath);
}
