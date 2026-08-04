#include "AssetLoader.hpp"

namespace gbe {

    std::unordered_map<AssetType, IAssetCollection*> allAssetLoaders;

    IAsset* GetBaseData(const GUID& guid) {
        return AssetDatabase::GetAssetByGUID(guid);
    }

    IAsset* GetBaseDataByPath(const std::filesystem::path& path) {
        return AssetDatabase::GetAssetByPath(path);
    }

    AssetType GetAssetType(const GUID& guid) {
        IAsset* asset = GetBaseData(guid);
        return asset ? asset->GetAssetType() : AssetType("");
    }

} // namespace gbe