#pragma once

#include "../BaseAsset.h"

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace gbe {
    class BatchLoader {
    private:
        // Recursively finds all file paths within a given directory and its subdirectories.
        inline static void GetAllFilepaths(const fs::path& directory_path, std::vector<fs::path>& filepaths) {
            // Iterate through all entries (files and subdirectories) in the given directory.
            for (const auto& entry : fs::directory_iterator(directory_path)) {
                // Check if the current entry is a regular file.
                if (fs::is_regular_file(entry.status())) {
                    // If it's a file, add its path to our vector.
                    filepaths.push_back(entry.path());
                }
                // Check if the current entry is a directory.
                else if (fs::is_directory(entry.status())) {
                    // If it's a directory, recursively call the function on it.
                    GetAllFilepaths(entry.path(), filepaths);
                }
            }
        }
        inline static bool IsFileExtension(const std::string& filename, const std::string& extension) {
            // If the filename is shorter than the extension, it can't possibly match.
            if (filename.length() < extension.length()) {
                return false;
            }

            // Compare the end of the filename with the extension.
            return filename.compare(filename.length() - extension.length(), extension.length(), extension) == 0;
        }
    public:
        inline static void GenerateMetafiles(std::filesystem::path _directory) {
            std::vector<fs::path> filepaths;
            GetAllFilepaths(_directory, filepaths);

            // Phase 1: Cleanup orphaned meta files
            for (const auto& filepath : filepaths) {
                const auto& directory = filepath.parent_path();
                const auto& filename_ext = filepath.filename().string();
                const auto& filename_only = filepath.stem().stem().string(); // Get name before .obj.gbe or .img.gbe

                // Check for orphaned Mesh Metafiles
                if (IsFileExtension(filename_ext, ".obj.gbe")) {
                    // Check if corresponding .obj or .fbx exists
                    if (!std::filesystem::exists(directory / (filename_only + ".obj")) &&
                        !std::filesystem::exists(directory / (filename_only + ".fbx"))) {
                        std::filesystem::remove(filepath);
                    }
                }
                // Check for orphaned Texture Metafiles
                else if (IsFileExtension(filename_ext, ".img.gbe")) {
                    // Check if corresponding .png, .jpg, or .dds exists
                    if (!std::filesystem::exists(directory / (filename_only + ".png")) &&
                        !std::filesystem::exists(directory / (filename_only + ".jpg")) &&
                        !std::filesystem::exists(directory / (filename_only + ".dds"))) {
                        std::filesystem::remove(filepath);
                    }
                }
            }

            // Phase 2: Generate missing metafiles
            // We can reuse the existing 'filepaths' list if it's updated, 
            // or simply check if the metafile already exists during generation.
            for (const auto& filepath : filepaths) {
                const auto& directory = filepath.parent_path();
                const auto& filename_ext = filepath.filename().string();
                const auto& filename_only = filepath.stem().string();

                // Handle Meshes
                if (IsFileExtension(filename_ext, ".obj") || IsFileExtension(filename_ext, ".fbx")) {
                    auto meta_path = directory / (filename_only + ".obj.gbe");
                    if (!std::filesystem::exists(meta_path)) {
                        auto newdata = MeshImportData{ .path = filename_ext };
                        Parser::ExportClass(newdata, meta_path);
                    }
                }
                // Handle Textures
                else if (IsFileExtension(filename_ext, ".png") || IsFileExtension(filename_ext, ".jpg") || IsFileExtension(filename_ext, ".dds")) {
                    auto meta_path = directory / (filename_only + ".img.gbe");
                    if (!std::filesystem::exists(meta_path)) {
                        auto newdata = TextureImportData{ .path = filename_ext };
                        Parser::ExportClass(newdata, meta_path);
                    }
                }
            }
        }

        inline static void LoadAssetsFromDirectory(std::filesystem::path directory) {
            std::vector<fs::path> filepaths;
            GetAllFilepaths(directory, filepaths);

            std::vector<fs::path> filepaths_material;

            for (size_t i = 0; i < filepaths.size(); i++)
            {
                const auto& filepath = filepaths[i];
                const auto& filename = filepath.filename().string();

                if (IsFileExtension(filename, ".obj.gbe")) {
                    std::cout << "[BATCHLOADER] Loading Mesh: \"" << filepath << "\"" << std::endl;
                    new Mesh(filepath);
                }
                else if (IsFileExtension(filename, ".shader.gbe")) {
                    std::cout << "[BATCHLOADER] Loading Shader: \"" << filepath << "\"" << std::endl;
                    new Shader(filepath);
                }
                else if (IsFileExtension(filename, ".mat.gbe")) {
                    filepaths_material.push_back(filepath); // Defer material loading
                }
                else if (IsFileExtension(filename, ".img.gbe")) {
                    std::cout << "[BATCHLOADER] Loading Texture: \"" << filepath << "\"" << std::endl;
                    new Texture(filepath);
                }
                else if (IsFileExtension(filename, ".gbe")) {
                    std::cout << "[BATCHLOADER] Unknown Asset Type in: \"" << filepath << "\"" << std::endl;
                }
            }

            for (const auto& fp_mat : filepaths_material)
            {
                std::cout << "[BATCHLOADER] Loading Material: \"" << fp_mat << "\"" << std::endl;
                new Material(fp_mat);
            }

            //Wait here for all async tasks to finish
            bool batchload_done = false;
            while (!batchload_done)
            {
                batchload_done = true;

                for (const auto& lpair : gbe::allAssetLoaders)
                {
                    const auto& loader = lpair.second;

                    if (loader->CheckAsynchrounousTasks() > 0) {
                        batchload_done = false;
                    }
                }
            }
        }

        static void ReloadDirectory(std::filesystem::path directory);
    };
}