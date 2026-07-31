#include "AssetLoader.h"
#include "../BaseAsset.h"

std::unordered_map<gbe::AssetType, gbe::IAssetCollection*> gbe::allAssetLoaders;

gbe::IAsset* gbe::GetBaseData(std::filesystem::path path) {
	for (const auto& lpair : allAssetLoaders)
	{
		const auto& assetloader = lpair.second;
		auto assetdata = assetloader->FindAssetByPath(path);

		if (assetdata == nullptr)
			continue;

		return assetdata;
	}

	return nullptr;
}

gbe::AssetType gbe::GetAssetType(std::filesystem::path path) {
	for (const auto& lpair : allAssetLoaders)
	{
		const auto& assetloader = lpair.second;
		auto assetdata = assetloader->FindAssetByPath(path);

		if (assetdata == nullptr)
			continue;

		return assetdata->GetAssetType();
	}

	return AssetType("");
}

std::string gbe::GetAssetId(std::filesystem::path path) {
	for (const auto& lpair : allAssetLoaders)
	{
		const auto& assetloader = lpair.second;
		auto assetdata = assetloader->FindAssetByPath(path);

		if (assetdata == nullptr)
			continue;

		return assetdata->Get_assetId();
	}

	return "";
}