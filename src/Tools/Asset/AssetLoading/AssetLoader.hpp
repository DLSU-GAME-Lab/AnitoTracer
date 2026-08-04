#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <type_traits>
#include <filesystem>

#include "IAsset.hpp"
#include "AssetDatabase.hpp"

namespace gbe {

	class IAssetCollection {
	public:
		virtual ~IAssetCollection() = default;
		virtual int CheckAsynchronousTasks() = 0;
		virtual bool RegisterAsset(IAsset* asset) = 0;
		virtual std::vector<GUID> GetAllAssetIds() = 0;
	};

	template<typename TEngineAsset>
	class AssetLoader : public IAssetCollection {
		static_assert(
			std::is_base_of_v<IAsset, TEngineAsset>,
			"AssetLoader error: TEngineAsset type must derive from gbe::IAsset!"
			);

	protected:
		static AssetLoader<TEngineAsset>* activeInstance;

	public:
		virtual ~AssetLoader() override = default;

		void AssignSelfAsLoader() {
			activeInstance = this;
		}
		
		/// <summary>
		/// brief Retrieves all GUIDs belonging specifically to assets of type TEngineAsset.
		/// return std::vector<GUID> List of matching asset GUIDs.
		/// </summary>
		/// <returns></returns>
		std::vector<GUID> GetAllAssetIds() override { //[cite: 2]
			std::vector<GUID> assetIds;
			const auto& guidMap = AssetDatabase::GetGuidMap();

			// Optional optimization: reserve capacity to minimize reallocations
			assetIds.reserve(guidMap.size());

			for (const auto& [guid, assetPtr] : guidMap) {
				// Filter global database for assets that match this loader's type
				if (dynamic_cast<TEngineAsset*>(assetPtr) != nullptr) {
					assetIds.push_back(guid);
				}
			}

			return assetIds;
		}

		bool RegisterAsset(IAsset* asset) override {
			TEngineAsset* typedAsset = dynamic_cast<TEngineAsset*>(asset);
			if (!typedAsset) return false;

			// Register through AssetDatabase with collision handling
			AssetDatabase::RegisterAsset(typedAsset, typedAsset->GetGUID());
			return true;
		}

		// Fast O(1) GUID lookup
		TEngineAsset* GetAssetByGUID(const GUID& guid) {
			return dynamic_cast<TEngineAsset*>(AssetDatabase::GetAssetByGUID(guid));
		}

		// Fast O(1) Path lookup
		TEngineAsset* GetBaseData(const std::filesystem::path& path) {
			return dynamic_cast<TEngineAsset*>(AssetDatabase::GetAssetByPath(path));
		}

		// Directory move listener
		void OnAssetMoved(const std::filesystem::path& oldPath, const std::filesystem::path& newPath) {
			IAsset* asset = AssetDatabase::GetAssetByPath(oldPath);
			if (asset) {
				AssetDatabase::GetPathMap().erase(oldPath.string());
				asset->SetPath(newPath);
				AssetDatabase::GetPathMap()[newPath.string()] = asset->GetGUID();
			}
		}

		int CheckAsynchronousTasks() override { return 0; }

	protected:
		virtual bool LoadAssetImpl(std::unique_ptr<TEngineAsset> fileAsset) = 0;
	};

	template<typename TEngineAsset>
	AssetLoader<TEngineAsset>* AssetLoader<TEngineAsset>::activeInstance = nullptr;

	// --- Standalone Global Helpers ---
	extern std::unordered_map<AssetType, IAssetCollection*> allAssetLoaders;

	IAsset* GetBaseData(const GUID& guid);
	IAsset* GetBaseDataByPath(const std::filesystem::path& path);
	AssetType GetAssetType(const GUID& guid);

} // namespace gbe