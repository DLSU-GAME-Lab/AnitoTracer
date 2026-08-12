#pragma once

#include <iostream>
#include <unordered_map>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include "IAsset.hpp"
#include "GUID.hpp"

namespace gbe {

    class AssetDatabase {
    private:
        static std::vector<std::filesystem::path>& GetDirectories() {
            static std::vector<std::filesystem::path> instance;
            return instance;
        }

        static std::string PathToKey(const std::filesystem::path& path) {
            // 1. Attempt to localize the path relative to registered include directories
            auto localized = LocalizePath(path);
            const auto& target_path = localized.empty() ? path : localized;

            // 2. Lexically normalize separators and relative components (a/b/../c -> a/c)
            // 3. Convert to generic format (always uses '/' regardless of OS)
            auto generic_path = target_path.lexically_normal().generic_u8string();

            // 4. Force lowercase for case-insensitive matching
            std::string key(generic_path.begin(), generic_path.end());
            std::transform(key.begin(), key.end(), key.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return key;
        }

    public:
        static std::unordered_map<GUID, IAsset*>& GetGuidMap() {
            static std::unordered_map<GUID, IAsset*> instance;
            return instance;
        }

        static std::unordered_map<std::string, GUID>& GetPathMap() {
            static std::unordered_map<std::string, GUID> instance;
            return instance;
        }

        /**
         * @brief Registers a search/include directory for path localization.
         */
        static void RegisterDirectory(const std::filesystem::path& dir) {
            GetDirectories().push_back(std::filesystem::absolute(dir).lexically_normal());
        }

        /**
         * @brief Localizes an absolute path relative to the closest registered directory.
         * @return Relative path from the closest directory, or an empty path if not inside any.
         */
        static std::filesystem::path LocalizePath(const std::filesystem::path& abs_path) {
            namespace fs = std::filesystem;
            fs::path norm_target = abs_path.lexically_normal();

            fs::path best_relative;
            std::ptrdiff_t max_depth = -1;

            for (const auto& dir : GetDirectories()) {
                fs::path norm_dir = dir.lexically_normal();
                fs::path rel = fs::relative(norm_target, norm_dir);

                // Reject if empty or starts with ".." (outside of the registered directory)
                if (rel.empty() || rel.begin() == rel.end() || *rel.begin() == "..") {
                    continue;
                }

                // Check depth of this directory (higher depth = deeper/closer match)
                std::ptrdiff_t depth = std::distance(norm_dir.begin(), norm_dir.end());
                if (depth > max_depth) {
                    max_depth = depth;
                    best_relative = std::move(rel);
                }
            }

            return best_relative;
        }

        // Fast O(1) global GUID lookup
        static IAsset* GetAssetByGUID(const GUID& guid) {
            if (guid == GUID::Empty()) return nullptr;

            auto& guidMap = GetGuidMap();
            auto it = guidMap.find(guid);
            return (it != guidMap.end()) ? it->second : nullptr;
        }

        // Fast O(1) path lookup (handles both full and localized paths)
        static IAsset* GetAssetByPath(const std::filesystem::path& path) {
            auto& pathMap = GetPathMap();
            auto path_lookup = PathToKey(path);
            auto it = pathMap.find(path_lookup);
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
            pathMap[PathToKey(asset->GetPath())] = finalGuid;

            return finalGuid;
        }

        static void UnregisterAsset(const GUID& guid) {
            auto& guidMap = GetGuidMap();
            auto it = guidMap.find(guid);
            if (it != guidMap.end()) {
                GetPathMap().erase(PathToKey(it->second->GetPath()));
                guidMap.erase(it);
            }
        }

        static void Clear() {
            GetGuidMap().clear();
            GetPathMap().clear();
            GetDirectories().clear();
        }
    };

} // namespace gbe