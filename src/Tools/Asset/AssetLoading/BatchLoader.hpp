#pragma once

#include "BaseAsset.hpp" // Expected to contain BaseImportData

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <functional>
#include <algorithm>
#include <type_traits>

#include "../Organization/SingletonMacro.hpp"

namespace fs = std::filesystem;

namespace gbe {

    class BatchLoader {
        SINGLETON_MACRO_DEFAULT(BatchLoader);
    public:
        // Configurable meta-naming strategy options
        enum class MetaNamingStrategy {
            AppendToFilename,   // e.g., model.obj -> model.obj.gbe
            ReplaceExtension    // e.g., model.obj -> model.gbe
        };

        // Category rule definition
        struct CategoryConfig {
            std::string name;
            std::vector<std::string> sourceExtensions;
            std::string metaSuffix;
            bool isDeferred = false;
            MetaNamingStrategy namingStrategy = MetaNamingStrategy::AppendToFilename;

            // Callbacks for file operations
            std::function<void(const fs::path& sourcePath, const fs::path& metaPath)> metaGenerator;
            std::function<void(const fs::path& metaPath)> loader;
        };

        /**
         * @brief Registers a project asset category with strict inheritance validation on TMeta.
         *
         * @tparam TMeta User-defined meta class strictly inheriting from BaseImportData.
         * @param categoryName Descriptive name (e.g., "Texture", "Mesh").
         * @param sourceExtensions List of supported extensions (e.g., {".png", ".jpg"}).
         * @param metaSuffix Meta extension (e.g., ".gbe" or ".meta").
         * @param loader Callback function executing the asset loader.
         * @param metaInitializer Optional lambda to initialize TMeta values before serialization.
         * @param isDeferred If true, delays loading phase (useful for Materials/Shaders).
         * @param namingStrategy Metafile naming scheme rule.
         */
        template <typename TMeta>
        static void RegisterCategory(
            const std::string& categoryName,
            const std::vector<std::string>& sourceExtensions,
            const std::string& metaSuffix,
            std::function<void(const fs::path& metaPath)> loader,
            std::function<void(TMeta& meta, const fs::path& sourcePath)> metaInitializer = nullptr,
            bool isDeferred = false,
            MetaNamingStrategy namingStrategy = MetaNamingStrategy::AppendToFilename)
        {
            // Strict compile-time constraint: TMeta must inherit from BaseImportData
            static_assert(std::is_base_of_v<BaseImportData, TMeta>,
                "TMeta template argument must derive from gbe::BaseImportData");

            CategoryConfig config;
            config.name = categoryName;
            config.sourceExtensions = sourceExtensions;
            config.metaSuffix = metaSuffix;
            config.loader = loader;
            config.isDeferred = isDeferred;
            config.namingStrategy = namingStrategy;

            // Generate metafile factory using the allowed Parser::ExportClass interface
            config.metaGenerator = [metaInitializer, categoryName](const fs::path& sourcePath, const fs::path& metaPath) {
                TMeta newdata{};

                // Initialize default BaseImportData properties
                newdata.assetId = sourcePath.stem().string();
                newdata.assetType = categoryName;

                if (metaInitializer) {
                    metaInitializer(newdata, sourcePath);
                }

                // Internal parser dependency call
                Parser::ExportClass(newdata, metaPath);
                };

            GetInstance().m_categories.push_back(config);
        }

        static void RegisterCategoryDefault(const std::string& categoryName,
            const std::vector<std::string>& sourceExtensions,
            const std::string& metaSuffix,
            std::function<void(const fs::path& metaPath)> loader,
            std::function<void(BaseImportData& meta, const fs::path& sourcePath)> metaInitializer = nullptr,
            bool isDeferred = false,
            MetaNamingStrategy namingStrategy = MetaNamingStrategy::AppendToFilename)
        {
            RegisterCategory<BaseImportData>(
                categoryName,
                sourceExtensions,
                metaSuffix,
                loader,
                metaInitializer, 
                isDeferred, 
                namingStrategy
            );
        }

        // Phase 1 & 2: Cleanup orphaned metafiles and generate missing ones
        static void GenerateMetafiles(const fs::path& directory) {
            if (!fs::exists(directory) || !fs::is_directory(directory)) return;

            auto filepaths = GetAllFilepaths(directory);

            // Phase 1: Cleanup orphaned meta files
            for (const auto& filepath : filepaths) {
                for (const auto& cat : GetInstance().m_categories) {
                    if (EndsWith(filepath.string(), cat.metaSuffix)) {
                        fs::path expectedSource = GetSourcePathFromMeta(filepath, cat);

                        // If no corresponding source file exists for any valid extension, remove orphaned meta
                        if (!fs::exists(expectedSource)) {
                            std::cout << "[BATCHLOADER] Removing orphaned metafile: " << filepath << std::endl;
                            fs::remove(filepath);
                        }
                    }
                }
            }

            // Refresh list after cleanup
            filepaths = GetAllFilepaths(directory);

            // Phase 2: Generate missing metafiles
            for (const auto& filepath : filepaths) {
                std::string ext = filepath.extension().string();

                for (const auto& cat : GetInstance().m_categories) {
                    // Match source extension
                    bool matchesExt = std::any_of(cat.sourceExtensions.begin(), cat.sourceExtensions.end(),
                        [&ext](const std::string& validExt) {
                            return EqualIgnoreCase(ext, validExt);
                        });

                    if (matchesExt) {
                        fs::path metaPath = BuildMetaPath(filepath, cat);
                        if (!fs::exists(metaPath) && cat.metaGenerator) {
                            std::cout << "[BATCHLOADER] Generating metafile: " << metaPath << std::endl;
                            cat.metaGenerator(filepath, metaPath);
                        }
                    }
                }
            }
        }

        // Phase 3: Load registered assets from directory
        static void LoadAssetsFromDirectory(const fs::path& directory, std::function<bool()> isAsyncPending = nullptr) {
            if (!fs::exists(directory) || !fs::is_directory(directory)) return;

            auto filepaths = GetAllFilepaths(directory);
            std::vector<std::pair<CategoryConfig, fs::path>> deferredLoads;

            // Load primary assets first
            for (const auto& filepath : filepaths) {
                std::string filename = filepath.filename().string();

                for (const auto& cat : GetInstance().m_categories) {
                    if (EndsWith(filename, cat.metaSuffix)) {
                        if (cat.isDeferred) {
                            deferredLoads.push_back({ cat, filepath });
                        }
                        else {
                            std::cout << "[BATCHLOADER] Loading " << cat.name << ": " << filepath << std::endl;
                            if (cat.loader) cat.loader(filepath);
                        }
                    }
                }
            }

            // Execute deferred assets (e.g., Materials)
            for (const auto& [cat, filepath] : deferredLoads) {
                std::cout << "[BATCHLOADER] Loading Deferred " << cat.name << ": " << filepath << std::endl;
                if (cat.loader) cat.loader(filepath);
            }

            // Optional async synchronization loop
            if (isAsyncPending) {
                while (isAsyncPending()) {
                    // Wait for async task execution pool completion
                }
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
                    filepaths.push_back(entry.path());
                }
            }
            return filepaths;
        }

        static fs::path BuildMetaPath(const fs::path& sourcePath, const CategoryConfig& cat) {
            if (cat.namingStrategy == MetaNamingStrategy::AppendToFilename) {
                return sourcePath.string() + cat.metaSuffix; // e.g., model.obj -> model.obj.gbe
            }
            else {
                return sourcePath.parent_path() / (sourcePath.stem().string() + cat.metaSuffix); // e.g., model.obj -> model.gbe
            }
        }

        static fs::path GetSourcePathFromMeta(const fs::path& metaPath, const CategoryConfig& cat) {
            std::string metaStr = metaPath.string();
            if (cat.namingStrategy == MetaNamingStrategy::AppendToFilename) {
                // Strips suffix directly (e.g., model.obj.gbe -> model.obj)
                return metaStr.substr(0, metaStr.length() - cat.metaSuffix.length());
            }
            else {
                // Tries to locate source file matching stem with registered category extensions
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

        static bool EqualIgnoreCase(std::string_view a, std::string_view b) {
            return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                [](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); });
        }
    };

} // namespace gbe