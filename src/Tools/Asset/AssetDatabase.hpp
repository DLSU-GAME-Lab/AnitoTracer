#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "IAsset.hpp"
#include "GUID.hpp"

namespace gbe {

    class AssetDatabase {
    private:
        static std::vector<std::filesystem::path>& GetDirectories() {
            static std::vector<std::filesystem::path> directories;
            return directories;
        }

        static std::filesystem::path& ProjectDirectory() {
            static std::filesystem::path directory;
            return directory;
        }

        static std::unordered_set<GUID>& ProjectAssets() {
            static std::unordered_set<GUID> assets;
            return assets;
        }

        static bool& LoadingProject() {
            static bool loading = false;
            return loading;
        }

        static std::string PathToKey(const std::filesystem::path& path) {
            auto generic = std::filesystem::absolute(path).lexically_normal().generic_u8string();
            std::string key(generic.begin(), generic.end());
#ifdef _WIN32
            std::transform(key.begin(), key.end(), key.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
#endif
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

        static void RegisterDirectory(const std::filesystem::path& dir) {
            const auto normalized = std::filesystem::absolute(dir).lexically_normal();
            const auto key = PathToKey(normalized);
            auto& dirs = GetDirectories();
            if (std::none_of(dirs.begin(), dirs.end(), [&](const auto& existing) {
                return PathToKey(existing) == key;
            })) {
                dirs.push_back(normalized);
            }
        }

        static void BeginProjectLoad(const std::filesystem::path& dir) {
            const auto previousAssets = ProjectAssets();
            for (const auto& guid : previousAssets) {
                UnregisterAsset(guid);
            }
            ProjectDirectory() = std::filesystem::absolute(dir).lexically_normal();
            LoadingProject() = true;
        }

        static void BeginProjectRescan() { LoadingProject() = true; }

        static void EndProjectLoad() { LoadingProject() = false; }

        static std::filesystem::path LocalizePath(const std::filesystem::path& path) {
            namespace fs = std::filesystem;
            if (!path.is_absolute()) return path.lexically_normal();

            const auto target = path.lexically_normal();
            fs::path best;
            std::size_t bestDepth = 0;
            auto consider = [&](const fs::path& dir) {
                if (dir.empty()) return;
                const auto rel = target.lexically_relative(dir);
                if (rel.empty() || *rel.begin() == "..") return;
                const auto depth = static_cast<std::size_t>(std::distance(dir.begin(), dir.end()));
                if (best.empty() || depth > bestDepth) {
                    best = rel;
                    bestDepth = depth;
                }
            };
            consider(ProjectDirectory());
            for (const auto& dir : GetDirectories()) consider(dir);
            return best;
        }

        static IAsset* GetAssetByGUID(const GUID& guid) {
            if (guid == GUID::Empty()) return nullptr;
            const auto it = GetGuidMap().find(guid);
            return it == GetGuidMap().end() ? nullptr : it->second;
        }

        static IAsset* GetAssetByPath(const std::filesystem::path& path) {
            if (path.empty()) return nullptr;
            auto find = [&](const std::filesystem::path& candidate) -> IAsset* {
                const auto it = GetPathMap().find(PathToKey(candidate));
                return it == GetPathMap().end() ? nullptr : GetAssetByGUID(it->second);
            };
            if (path.is_absolute()) return find(path);
            if (!ProjectDirectory().empty()) {
                if (auto* asset = find(ProjectDirectory() / path)) return asset;
            }
            if (auto* asset = find(path)) return asset;
            for (const auto& dir : GetDirectories()) {
                if (auto* asset = find(dir / path)) return asset;
            }
            return nullptr;
        }

        // A conflicting GUID is never changed automatically: scenes may already refer to it.
        static GUID RegisterAsset(IAsset* asset, GUID requestedGuid) {
            if (!asset || asset->GetPath().empty() || requestedGuid == GUID::Empty()) {
                std::cerr << "[AssetDatabase] Cannot register an asset without a path and GUID.\n";
                return GUID::Empty();
            }
            auto& guidMap = GetGuidMap();
            auto& pathMap = GetPathMap();
            const auto key = PathToKey(asset->GetPath());
            const auto guidIt = guidMap.find(requestedGuid);
            const auto pathIt = pathMap.find(key);
            if ((guidIt != guidMap.end() && guidIt->second != asset) ||
                (pathIt != pathMap.end() && pathIt->second != requestedGuid)) {
                std::cerr << "[AssetDatabase] Conflicting GUID or path for " << asset->GetPath()
                    << " (GUID " << requestedGuid << "); metadata was not changed.\n";
                return GUID::Empty();
            }
            const auto oldGuid = asset->GetGUID();
            if (oldGuid != requestedGuid) {
                auto old = guidMap.find(oldGuid);
                if (old != guidMap.end() && old->second == asset) {
                    guidMap.erase(old);
                    ProjectAssets().erase(oldGuid);
                }
            }
            const bool newlyRegistered = guidIt == guidMap.end();
            asset->SetGUID(requestedGuid);
            guidMap[requestedGuid] = asset;
            pathMap[key] = requestedGuid;
            if (LoadingProject() && newlyRegistered) ProjectAssets().insert(requestedGuid);
            return requestedGuid;
        }

        static void UnregisterAsset(const GUID& guid) {
            auto& guidMap = GetGuidMap();
            auto it = guidMap.find(guid);
            if (it == guidMap.end()) return;
            auto& paths = GetPathMap();
            const auto pathIt = paths.find(PathToKey(it->second->GetPath()));
            if (pathIt != paths.end() && pathIt->second == guid) paths.erase(pathIt);
            guidMap.erase(it);
            ProjectAssets().erase(guid);
        }

        static void OnAssetMoved(const std::filesystem::path& oldPath, const std::filesystem::path& newPath) {
            auto* asset = GetAssetByPath(oldPath);
            if (!asset) {
                std::cerr << "[AssetDatabase] Cannot move unregistered asset: " << oldPath << '\n';
                return;
            }
            const auto newKey = PathToKey(newPath);
            const auto existing = GetPathMap().find(newKey);
            if (existing != GetPathMap().end() && existing->second != asset->GetGUID()) {
                std::cerr << "[AssetDatabase] Cannot move asset to occupied path: " << newPath << '\n';
                return;
            }
            GetPathMap().erase(PathToKey(asset->GetPath()));
            asset->SetPath(std::filesystem::absolute(newPath).lexically_normal());
            GetPathMap()[newKey] = asset->GetGUID();
        }

        static void Clear() {
            GetGuidMap().clear();
            GetPathMap().clear();
            GetDirectories().clear();
            ProjectAssets().clear();
            ProjectDirectory().clear();
            LoadingProject() = false;
        }
    };

} // namespace gbe
