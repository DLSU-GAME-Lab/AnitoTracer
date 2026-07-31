#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "AssetLoading/AssetLoader.h"
#include "File/Parser.hpp"

#include "IAsset.h"

namespace fs = std::filesystem;

namespace gbe {

	template<class TFinal, class TImportData>
	class BaseAsset : public IAsset {
	protected:
		TImportData importData;
	public:
		BaseAsset(std::filesystem::path asset_path) {
			gbe::Parser::PopulateClass(this->importData, asset_path);

			this->assetFilepath = asset_path;

			std::string filename_with_ext = asset_path.filename().string();
			size_t dot_pos = filename_with_ext.find('.');
			if (dot_pos != std::string::npos)
				this->baseImportData.assetId = filename_with_ext.substr(0, dot_pos);
			else
				this->baseImportData.assetId = filename_with_ext;

			IAssetImporter<TFinal, TImportData>::LoadFileAsset(static_cast<TFinal*>(this), this->importData);
		}
		bool GetDestroyQueued() {
			return this->destroyQueued;
		}
		TImportData& GetImportData() {
			return this->importData;
		}
		inline static TFinal* GetAssetById(const std::string id) {
			return IAssetImporter<TFinal, TImportData>::GetAssetById(id);
		}
	};
}