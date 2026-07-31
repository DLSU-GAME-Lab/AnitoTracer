#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "BaseImportData.hpp"

#include "../Organization/DynamicEnum.hpp"

namespace gbe {
	struct AssetTypeTag {};
	using AssetType = DynamicEnum<AssetTypeTag>;

	class IAsset {
	protected:
		AssetType assetType;
		std::filesystem::path assetFilepath;
		bool destroyQueued;
		BaseImportData baseImportData;
	public:
		inline std::string Get_assetId() {
			return this->baseImportData.assetId;
		}
		AssetType GetAssetType();
		inline std::filesystem::path GetAssetFilepath(bool has_ext = true) {
			if (has_ext)
				return assetFilepath;
			else {
				auto p = assetFilepath;

				while (p.has_extension()) {
					p = p.stem();
				}

				return assetFilepath.parent_path() / p;
			}
		}
	};
}