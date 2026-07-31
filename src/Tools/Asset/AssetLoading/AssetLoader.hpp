#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <type_traits>

#include "IAsset.hpp"

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

    template<typename TEngineAsset>
    class AssetLoader : public IAssetCollection {
        static_assert(
            std::is_base_of_v<IAsset, TEngineAsset>,
            "AssetLoader error: TEngineAsset type must derive from gbe::IAsset!"
            );

    protected:
        static AssetLoader<TEngineAsset>* activeInstance;

        // Single map holding polymorphically derived backend objects
        std::unordered_map<std::string, std::unique_ptr<TEngineAsset>> m_assets;

    public:
        virtual ~AssetLoader() override = default;

        void AssignSelfAsLoader() {
            activeInstance = this;
        }

        // --- Engine Static Interface ---
        static TEngineAsset* GetAssetById(const std::string& id) {
            if (!activeInstance) return nullptr;
            auto it = activeInstance->m_assets.find(id);
            return (it != activeInstance->m_assets.end()) ? it->second.get() : nullptr;
        }

        static bool LoadFileAsset(std::unique_ptr<TEngineAsset> asset) {
            if (!activeInstance) throw std::runtime_error("AssetLoader instance not assigned!");
            return activeInstance->LoadAssetImpl(std::move(asset));
        }

        // --- Standard IAssetCollection Overrides ---
        void UnloadAll() override {
            // Calling clear() invokes ~TEngineAsset(), which polymorphically executes 
            // the derived backend destructor (~GLTextureAsset) automatically!
            m_assets.clear();
        }

        IAsset* FindAssetById(const std::string& id) override {
            return GetAssetById(id);
        }

        IAsset* FindAssetByPath(const std::filesystem::path& path) override {
            for (auto& [id, asset] : m_assets) {
                if (asset && asset->assetFilepath == path) {
                    return asset.get();
                }
            }
            return nullptr;
        }

        std::vector<std::string> GetAllAssetIds() override {
            std::vector<std::string> ids;
            ids.reserve(m_assets.size());
            for (const auto& [id, _] : m_assets) ids.push_back(id);
            return ids;
        }

        int CheckAsynchronousTasks() override { return 0; }

    protected:
        // Pure virtual hook where backend loader creates the upgraded backend object
        virtual bool LoadAssetImpl(std::unique_ptr<TEngineAsset> fileAsset) = 0;
    };

    template<typename TEngineAsset>
    AssetLoader<TEngineAsset>* AssetLoader<TEngineAsset>::activeInstance = nullptr;

} // namespace gbe