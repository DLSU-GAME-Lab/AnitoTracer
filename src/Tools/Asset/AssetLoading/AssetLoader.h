#pragma once

#include <functional>
#include <string>
#include <algorithm>
#include <filesystem>

#include "../IAsset.h"

namespace gbe {
	class IAsset;
	
	extern gbe::IAsset* GetBaseData(std::filesystem::path path);
	extern gbe::AssetType GetAssetType(std::filesystem::path path);
	extern std::string GetAssetId(std::filesystem::path path);

	class IAssetCollection {
	public:
		/// <summary>
		/// 
		/// </summary>
		/// <returns>The count of remaining asynchronous load tasks.</returns>
		int virtual CheckAsynchrounousTasks() = 0;
		virtual IAsset* FindAssetByPath(std::filesystem::path path) = 0;
		virtual IAsset* FindAssetById(std::string id) = 0;
		virtual std::vector<std::string> GetAllAssetIds() = 0;

		virtual void UnloadAll() = 0;
	};

	extern std::unordered_map<gbe::AssetType, IAssetCollection*> allAssetLoaders;

	template<class TAsset, class TAssetImportData>
	class IAssetImporter : public IAssetCollection {
	protected:
		static IAssetImporter* activeBaseInstance;
		std::unordered_map<std::string, std::unique_ptr<TAsset>> fileassetDictionary;

		std::function<bool(TAsset* asset, const TAssetImportData& importData)> loadFunc;
	public:
		static bool LoadFileAsset(TAsset* asset, const TAssetImportData& importData) {
			if (activeBaseInstance == nullptr)
				throw new std::runtime_error("asset loader for this particular type is not assigned!");

			return activeBaseInstance->loadFunc(asset, importData);
		}
		static TAsset* GetAssetById(std::string assetId) {
			auto it = activeBaseInstance->fileassetDictionary.find(assetId);
			if (it != activeBaseInstance->fileassetDictionary.end()) {
				return it->second;
			}

			return nullptr;
		}
		virtual std::vector<std::string> GetAllAssetIds() override {
			std::vector<std::string> ids;
			for (const auto& pair : activeBaseInstance->fileassetDictionary) {
				ids.push_back(pair.first);
			}
			return ids;
		}

		IAsset* FindAssetByPath(std::filesystem::path asset_path) override {

			auto p = asset_path;

			while (p.has_extension()) {
				p = p.stem();
			}

			p = asset_path.parent_path() / p;

			for (const auto& pair : activeBaseInstance->fileassetDictionary)
			{
				IAsset* baseasset = dynamic_cast<IAsset*>(pair.second);

				if (baseasset == nullptr)
					continue;

				if (baseasset->GetAssetFilepath(false) == p)
					return pair.second;
			}

			return nullptr;
		}

		IAsset* FindAssetById(std::string id) override {
			auto find_it = activeBaseInstance->fileassetDictionary.find(id);

			if (find_it == activeBaseInstance->fileassetDictionary.end())
				return nullptr;

			auto entry = dynamic_cast<IAsset*>(find_it->second);;
			if (entry == nullptr)
				return nullptr;

			return entry;
		}
	};

	template<class TAsset, class TAssetImportData>
	IAssetImporter<TAsset, TAssetImportData>* IAssetImporter<TAsset, TAssetImportData>::activeBaseInstance = nullptr;

	template<class TAsset, class TAssetImportData, class TAssetLoadData>
	class AssetLoader : public IAssetImporter<TAsset, TAssetImportData> {
	public:
		struct AsyncLoadTask {
			bool isDone = false;
			std::string id;
			std::string path;
			TAssetImportData importData;
		};
	private:
		std::vector<AsyncLoadTask*> asyncTasks;
	protected:
		static AssetLoader* activeInstance;

		std::unordered_map<std::string, TAssetLoadData> loadedAssets;
		virtual void LoadAsset(TAsset* asset, const TAssetImportData& importData) = 0;
		virtual void UnLoadAsset(TAssetLoadData* load_data) = 0;

	public:

		inline void RegisterAsyncTask(AsyncLoadTask* task) {
			asyncTasks.push_back(task);
		}

		inline virtual void OnAsyncTaskCompleted(AsyncLoadTask* task) = 0;

		inline int virtual CheckAsynchrounousTasks() override {
			std::vector<AsyncLoadTask*> checked;

			for (size_t i = 0; i < this->asyncTasks.size(); i++)
			{
				auto& async_task = this->asyncTasks[i];

				if (async_task->isDone) {
					OnAsyncTaskCompleted(async_task);
					checked.push_back(async_task);
				}
			}

			for (const auto& checkedptr : checked)
			{
				std::erase_if(this->asyncTasks, [checkedptr](AsyncLoadTask* x) {
					return x == checkedptr;
					});
			}

			return static_cast<int>(this->asyncTasks.size());
		}

		virtual void AssignSelfAsLoader() {
			this->activeBaseInstance = this;
			this->activeInstance = this;

			this->loadFunc = [](TAsset* asset, const TAssetImportData& importData) {
				{
					//Unload old asset data
					auto it = activeInstance->loadedAssets.find(asset->Get_assetId());
					if (it != activeInstance->loadedAssets.end()) {
						activeInstance->UnLoadAsset(&activeInstance->loadedAssets[asset->Get_assetId()]);
					}
				}

				//create new asset data
				activeInstance->LoadAsset(asset, importData);

				{
					//Delete old file class
					auto it = activeInstance->fileassetDictionary.find(asset->Get_assetId());
					if (it != activeInstance->fileassetDictionary.end()) {
						auto old_asset = activeInstance->fileassetDictionary[asset->Get_assetId()];
						delete old_asset;
					}
				}

				//Always override
				activeInstance->fileassetDictionary.insert_or_assign(asset->Get_assetId(), asset);

				return true;
				};
		}

		void UnloadAll() override {
			for (auto& pair : activeInstance->loadedAssets)
			{
				UnLoadAsset(&pair.second);
			}
			for (auto& pair : activeInstance->fileassetDictionary)
			{
				delete pair.second;
			}
		}

		static std::unordered_map<std::string, TAssetLoadData>& GetDataMap() {
			return activeInstance->loadedAssets;
		}

		static void RegisterData(std::string id, TAssetLoadData assetdata) {
			activeInstance->loadedAssets.insert_or_assign(id, assetdata);
		}

		static TAssetLoadData* GetAssetRuntimeData(std::string assetid) {
			auto it = activeInstance->loadedAssets.find(assetid);
			if (it != activeInstance->loadedAssets.end()) {
				return &it->second;
			}
			else {
				throw new std::runtime_error("Asset not found. id: \"" + assetid + "\"");
				return nullptr;
			}
		}

		static TAsset* GetAssetByPath(std::string asset_path) {
			for (const auto& pair : activeInstance->fileassetDictionary) {
				if (pair.second->GetAssetFilepath() == asset_path) {
					return pair.second;
				}
			}

			throw new std::runtime_error("Asset not found. path: \"" + asset_path + "\"");
			return nullptr;
		}

		static std::vector<IAsset*> GetAssetList() {
			std::vector<IAsset*> list;

			for (const auto& pair : activeInstance->fileassetDictionary)
			{
				list.push_back(pair.second);
			}

			return list;
		}
	};

	template<class TAsset, class TAssetImportData, class TAssetLoadData>
	AssetLoader<TAsset, TAssetImportData, TAssetLoadData>* AssetLoader<TAsset, TAssetImportData, TAssetLoadData>::activeInstance = nullptr;
}