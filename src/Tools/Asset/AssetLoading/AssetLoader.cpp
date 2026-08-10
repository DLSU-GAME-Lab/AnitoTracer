#include "AssetLoader.hpp"

namespace gbe {

    std::unordered_map<AssetType, IAssetCollection*> allAssetLoaders;

    AssetType GetAssetType(const GUID& guid) {
        IAsset* asset = AssetDatabase::GetAssetByGUID(guid);
        return asset ? asset->GetAssetType() : AssetType("");
    }

} // namespace gbe