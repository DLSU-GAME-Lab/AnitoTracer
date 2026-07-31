#pragma once

#include <memory>
#include <string>
#include <stdexcept>
#include <type_traits>
#include "../IAsset.hpp"

namespace gbe {
	
	class IAssetCollection {
	public:
		virtual ~IAssetCollection() = default;

		/// <summary>
		/// Returns the count of remaining asynchronous load tasks.
		/// </summary>
		virtual int CheckAsynchronousTasks() = 0;
		virtual IAsset* FindAssetByPath(const std::filesystem::path& path) = 0;
		virtual IAsset* FindAssetById(const std::string& id) = 0;
		virtual std::vector<std::string> GetAllAssetIds() = 0;

		virtual void UnloadAll() = 0;
	};

	// 1. Primary Template Declaration
	template<typename EngineData, typename BackendData = void>
	class AssetLoader;

	// ------------------------------------------------------------------
	// 2. Engine-Facing Base Specialization (BackendData = void)
	// Lightweight static interface for game code / logic.
	// ------------------------------------------------------------------
	template<typename EngineData>
	class AssetLoader<EngineData, void> : public IAssetCollection {
		static_assert(
			std::is_base_of_v<IAsset, EngineData>,
			"AssetLoader error: EngineData type must derive from gbe::IAsset!"
			);

	protected:
		static AssetLoader<EngineData, void>* activeEngineInstance;

	public:
		virtual ~AssetLoader() override = default;

		// --- Engine Static Interface ---
		static EngineData* GetAssetById(const std::string& id) {
			EnsureInstance();
			return activeEngineInstance->GetEngineAssetImpl(id);
		}

		static bool LoadFileAsset(std::unique_ptr<EngineData> asset) {
			EnsureInstance();
			return activeEngineInstance->LoadFileAssetImpl(std::move(asset));
		}

		// --- Abstract Hooks Forced on Implementor ---
		virtual EngineData* GetEngineAssetImpl(const std::string& id) = 0;
		virtual bool LoadFileAssetImpl(std::unique_ptr<EngineData> asset) = 0;

	private:
		static void EnsureInstance() {
			if (!activeEngineInstance) {
				throw std::runtime_error("AssetLoader active instance has not been assigned!");
			}
		}
	};

	template<typename EngineData>
	AssetLoader<EngineData, void>* AssetLoader<EngineData, void>::activeEngineInstance = nullptr;

	// ------------------------------------------------------------------
	// 3. Backend-Facing Full Specialization (BackendData != void)
	// Extends static interface to include backend data access.
	// ------------------------------------------------------------------
	template<typename EngineData, typename BackendData>
	class AssetLoader : public AssetLoader<EngineData, void> {
		static_assert(
			std::is_base_of_v<IAsset, EngineData>,
			"AssetLoader error: EngineData type must derive from gbe::IAsset!"
			);

	public:
		virtual ~AssetLoader() override = default;

		void AssignSelfAsLoader() {
			this->activeEngineInstance = this;
		}

		// Standard pipeline that extracts ID, generates backend data, and forwards to RegisterAssetImpl
		bool LoadFileAssetImpl(std::unique_ptr<EngineData> asset) override {
			if (!asset) return false;

			const std::string id = asset->Get_assetId();
			BackendData bData = CreateBackendData(asset.get());

			// Pass ID, engine asset, and backend asset to implementor to store its own way
			return RegisterAssetImpl(id, std::move(asset), std::move(bData));
		}

		// --- Backend Static Interface ---
		static BackendData* GetBackendData(const std::string& id) {
			auto* self = static_cast<AssetLoader<EngineData, BackendData>*>(
				AssetLoader<EngineData, void>::activeEngineInstance
				);
			if (!self) return nullptr;

			return self->GetBackendAssetImpl(id);
		}

	protected:
		// --- Subclass Abstract Hooks ---

		// Create graphics/hardware representation from engine data
		virtual BackendData CreateBackendData(EngineData* asset) = 0;

		// Implementor stores the unique engine asset and backend asset in custom storage (map, vector, pool, etc.)
		virtual bool RegisterAssetImpl(const std::string& id, std::unique_ptr<EngineData> engineAsset, BackendData backendAsset) = 0;

		// Implementor retrieves backend asset from custom storage
		virtual BackendData* GetBackendAssetImpl(const std::string& id) = 0;
	};
}