#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <functional>
#include <algorithm>
#include <type_traits>

#include "../Organization/SingletonMacro.hpp"
#include "../IAsset.hpp"
#include "AssetDatabase.hpp"

#include "File/Parser.hpp"

namespace fs = std::filesystem;

namespace gbe {

    class BatchLoader {
        SINGLETON_MACRO_DEFAULT(BatchLoader);
    public:
        enum class MetaNamingStrategy {
            AppendToFilename,   // model.obj -> model.obj.gbe
            ReplaceExtension    // model.obj -> model.gbe
        };

        struct CategoryConfig {
            std::string name;
            std::vector<std::string> sourceExtensions;
            std::string metaSuffix;
            bool isDeferred = false;
            MetaNamingStrategy namingStrategy = MetaNamingStrategy::AppendToFilename;

            std::function<void(const fs::path& sourcePath, const fs::path& metaPath)> loader;
        };

        template <typename TMeta>
        static void RegisterCategory(
            const std::string& categoryName,
            const std::vector<std::string>& sourceExtensions,
            const std::string& metaSuffix,
            std::function<TMeta*(const fs::path& sourcePath)> loader = nullptr,
            bool isDeferred = false,
            MetaNamingStrategy namingStrategy = MetaNamingStrategy::AppendToFilename)
        {
            static_assert(std::is_base_of_v<IAsset, TMeta>, "TMeta template argument must derive from gbe::IAsset");

            CategoryConfig config;
            config.name = categoryName;
            config.sourceExtensions = sourceExtensions;
            config.metaSuffix = metaSuffix;
            config.isDeferred = isDeferred;
            config.namingStrategy = namingStrategy;

            config.loader = [loader, categoryName](const fs::path& sourcePath, const fs::path& metaPath) {
                if (!loader) {
                    return;
                }
                TMeta* newdata = loader(sourcePath);

                if(!newdata)
                {
                    std::cerr << "Loader and asset exists but loader failed to Load Asset." << std::endl;
                    return;
                }

                // Initial setup
                newdata->SetPath(sourcePath);
                newdata->SetMetaPath(metaPath);

                TMeta dummydata = {};
                Parser::PopulateClass(dummydata, metaPath);

                GUID assignedGuid = {};

                // Check if metafile exists to parse existing GUID, or generate a fresh one
                if (fs::exists(metaPath))
                    assignedGuid = dummydata.GetGUID();
                else
                    assignedGuid = GUID::Generate();

                // Register with AssetDatabase (will automatically re-key if collision occurs)
                assignedGuid = AssetDatabase::RegisterAsset(newdata, assignedGuid);

                // Save or overwrite meta file with resolved GUID
                Parser::ExportClass(*newdata, metaPath);
            };

            GetInstance().m_categories.push_back(config);
        }

        // Clean orphaned metafiles and generate missing ones
        static void GenerateMetafiles(const fs::path& directory) {
            if (!fs::exists(directory) || !fs::is_directory(directory)) return;

            auto filepaths = GetAllFilepaths(directory);

            // Phase 1: Cleanup orphaned meta files
            for (const auto& filepath : filepaths) {
                for (const auto& cat : GetInstance().m_categories) {
                    if (EndsWithPath(filepath, cat.metaSuffix)) {
                        fs::path expectedSource = GetSourcePathFromMeta(filepath, cat);
                        if (!fs::exists(expectedSource)) {
                            std::cout << "[BATCHLOADER] Removing orphaned metafile: " << filepath << std::endl;
                            fs::remove(filepath);
                        }
                    }
                }
            }
        }

        // Phase 3: Load registered assets
        static void LoadAssetsFromDirectory(const fs::path& directory, std::function<bool()> isAsyncPending = nullptr) {
            if (!fs::exists(directory) || !fs::is_directory(directory)) return;

            auto filepaths = GetAllFilepaths(directory);
            std::vector<std::pair<CategoryConfig, fs::path>> deferredLoads;

            for (const auto& filepath : filepaths) {
                std::string filename = filepath.filename().string();

                for (const auto& cat : GetInstance().m_categories) {

                    std::string ext = filepath.extension().string();
                    bool matchesExt = std::any_of(cat.sourceExtensions.begin(), cat.sourceExtensions.end(),
                        [&ext](const std::string& validExt) {
                            return EqualIgnoreCase(ext, validExt);
                        });

                    if (matchesExt) {
                        if (cat.isDeferred) {
                            deferredLoads.push_back({ cat, filepath });
                        }
                        else {
                            fs::path metaPath = BuildMetaPath(filepath, cat);
                            if (cat.loader) {
                                std::cout << "[BATCHLOADER] Loading: " << metaPath << std::endl;
                                cat.loader(filepath, metaPath);
                            }
                        }
                    }
                }
            }

            for (const auto& [cat, filepath] : deferredLoads) {
                fs::path metaPath = BuildMetaPath(filepath, cat);
                if (cat.loader) {
                    std::cout << "[BATCHLOADER] Loading: " << metaPath << std::endl;
                    cat.loader(filepath, metaPath);
                }
            }

            if (isAsyncPending) {
                while (isAsyncPending()) {}
            }
        }

        static void ReloadDirectory(const fs::path& directory)
        {
            GenerateMetafiles(directory);
            LoadAssetsFromDirectory(directory);
        }

    private:
        std::vector<CategoryConfig> m_categories;

        static std::vector<fs::path> GetAllFilepaths(const fs::path& directory) {
            std::vector<fs::path> filepaths;
            for (const auto& entry : fs::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    try {
                        // Verify the filename can be safely converted to UTF-8 std::string
                        std::string testConversion = entry.path().filename().string();
                        filepaths.push_back(entry.path());
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[BATCHLOADER] Skipping file with invalid name (likely non-UTF8): " 
                                  << entry.path().parent_path().string() << " - Error: " << e.what() << std::endl;
                    }
                }
            }
            return filepaths;
        }

        static fs::path BuildMetaPath(const fs::path& sourcePath, const CategoryConfig& cat) {
            if (cat.namingStrategy == MetaNamingStrategy::AppendToFilename) {
                fs::path result = sourcePath;
                result += cat.metaSuffix;
                return result;
            }
            else {
                return sourcePath.parent_path() / (sourcePath.stem().string() + cat.metaSuffix);
            }
        }

        static fs::path GetSourcePathFromMeta(const fs::path& metaPath, const CategoryConfig& cat) {
            if (cat.namingStrategy == MetaNamingStrategy::AppendToFilename) {
                std::string metaStr = metaPath.filename().string();
                std::string sourceStr = metaStr.substr(0, metaStr.length() - cat.metaSuffix.length());
                return metaPath.parent_path() / sourceStr;
            }
            else {
                for (const auto& ext : cat.sourceExtensions) {
                    fs::path testPath = metaPath.parent_path() / (metaPath.stem().string() + ext);
                    if (fs::exists(testPath)) return testPath;
                }
                return {};
            }
        }

        static bool EndsWith(const std::string& str, const std::string& suffix) {
            return str.size() >= suffix.size() &&
                str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
        }

        static bool EndsWithPath(const fs::path& path, const std::string& suffix) {
            std::string pathStr = path.filename().string();
            return pathStr.size() >= suffix.size() &&
                pathStr.compare(pathStr.size() - suffix.size(), suffix.size(), suffix) == 0;
        }

        static bool EqualIgnoreCase(std::string_view a, std::string_view b) {
            return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                [](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); });
        }
    };

} // namespace gbe