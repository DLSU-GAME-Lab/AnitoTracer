#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "../Organization/DynamicEnum.hpp"

namespace gbe {
	struct AssetTypeTag {};
	using AssetType = DynamicEnum<AssetTypeTag>;

	/// <summary>
	/// This class represents an actual file living in the disk.
	/// </summary>
	struct IAsset {
		std::filesystem::path assetFilepath;
		std::filesystem::path metaFilepath;
		std::string assetType;
		std::string assetId;

		virtual ~IAsset() = default;

		AssetType GetAssetType() {
			return AssetType(assetType);
		}
		inline std::filesystem::path GetAssetPathWithoutExt() {
			auto p = assetFilepath;

			while (p.has_extension()) {
				p = p.stem();
			}

			return assetFilepath.parent_path() / p;
		}
	};
}