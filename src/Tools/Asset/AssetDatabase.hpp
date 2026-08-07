#pragma once

#include <iostream>
#include <unordered_map>
#include <filesystem>
#include <string>
#include "IAsset.hpp"
#include "GUID.hpp"

namespace gbe {

    class AssetDatabase {
    public:
        static std::unordered_map<GUID, IAsset*>& GetGuidMap() {
            static std::unordered_map<GUID, IAsset*> instance;
            return instance;
        }

        static std::unordered_map<std::string, GUID>& GetPathMap() {
            static std::unordered_map<std::string, GUID> instance;
            return instance;
        }

        // Fast O(1) global GUID lookup
        static IAsset* GetAssetByGUID(const GUID& guid) {
            if (guid == GUID::Empty()) return nullptr;

            auto& guidMap = GetGuidMap();
            auto it = guidMap.find(guid);
            return (it != guidMap.end()) ? it->second : nullptr;
        }

        // Fast O(1) path lookup
        static IAsset* GetAssetByPath(const std::filesystem::path& path) {
            auto& pathMap = GetPathMap();
            auto it = pathMap.find(path.string());
            if (it != pathMap.end()) {
                return GetAssetByGUID(it->second);
            }
            return nullptr;
        }

        /**
         * @brief Registers an asset with automatic GUID collision handling.
         * @return The final, collision-free GUID assigned to the asset.
         */
        static GUID RegisterAsset(IAsset* asset, GUID requestedGuid) {
            if (!asset) return GUID::Empty();

            auto& guidMap = GetGuidMap();
            auto& pathMap = GetPathMap();

            GUID finalGuid = requestedGuid;

            // --- Collision Handling ---
            if (finalGuid == GUID::Empty() || guidMap.find(finalGuid) != guidMap.end()) {
                if (finalGuid != GUID::Empty()) {
                    std::cerr << "[AssetDatabase Warning] GUID Collision detected ("
                        << finalGuid.ToString() << ") at path: " << asset->GetPath() << "\n";
                }

                // Re-key: Regenerate until collision is resolved
                do {
                    finalGuid = GUID::Generate();
                } while (guidMap.find(finalGuid) != guidMap.end());

                std::cout << "[AssetDatabase Info] Re-keyed asset to new GUID: "
                    << finalGuid.ToString() << "\n";
            }

            asset->SetGUID(finalGuid);
            guidMap[finalGuid] = asset;
            pathMap[asset->GetPath().string()] = finalGuid;

            return finalGuid;
        }

        static void UnregisterAsset(const GUID& guid) {
            auto& guidMap = GetGuidMap();
            auto it = guidMap.find(guid);
            if (it != guidMap.end()) {
                GetPathMap().erase(it->second->GetPath().string());
                guidMap.erase(it);
            }
        }

        static void Clear() {
            GetGuidMap().clear();
            GetPathMap().clear();
        }
    };

} // namespace gbe