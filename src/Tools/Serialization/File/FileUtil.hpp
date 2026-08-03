#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace gbe {
	class FileUtil {
	public:
		inline static bool ValidateDirectory(std::filesystem::path directory) {
			// Get the parent path (which is the folder/directory)
			std::filesystem::path folderPath = directory.parent_path();

			if (folderPath.empty())
				return true;

			return std::filesystem::create_directories(folderPath);
		}

		inline static void Copy(std::filesystem::path from, std::filesystem::path to) {
			ValidateDirectory(to);

			std::ifstream src(from, std::ios::binary);
			std::ofstream dst(to, std::ios::binary);

			if (!src.is_open()) {
				std::cerr << "Error: Could not open source file: " << from << std::endl;
			}
			if (!dst.is_open()) {
				std::cerr << "Error: Could not open destination file: " << to << std::endl;
			}

			dst << src.rdbuf();

			src.close();
			dst.close();
		}
	};
}