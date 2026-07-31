#pragma once

// 1. Define NOMINMAX for any Windows headers included after this point
#ifndef NOMINMAX
#define NOMINMAX
#endif

// 2. Strip existing max/min macros if Windows headers were included BEFORE this point
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include <glaze/glaze.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

#include "FileUtil.hpp"

namespace gbe {
	class Parser {
	public:

		template<class TImportData>
		static void PopulateClassStr(TImportData& target, std::string data) {
			TImportData datareceiver;
			auto ec = glz::read < glz::opts{ .error_on_unknown_keys = false } > (datareceiver, data);
			target = datareceiver;
			std::cout << ec << std::endl;
		}

		template<class TExportData>
		static std::string ExportClassStr(const TExportData& target) {
			std::string out_string;
			glz::write < glz::opts{ .error_on_unknown_keys = false } > (target, out_string);
			return out_string;
		}

		template<class TImportData>
		static bool PopulateClass(TImportData& target, std::filesystem::path asset_path) {
			std::string buffer;
			TImportData datareceiver;
			auto ec = glz::read_file_json < glz::opts{ .error_on_unknown_keys = false } > (datareceiver, asset_path.string(), buffer);
			target = datareceiver;

			if (ec) {
				std::cout << ec << std::endl;
				return false;
			}

			return true;
		}

		template<class TExportData>
		static void ExportClass(const TExportData& target, std::filesystem::path asset_path) {
			FileUtil::ValidateDirectory(asset_path);
			std::ofstream file(asset_path);

			std::string out_string;
			glz::write < glz::opts{ .error_on_unknown_keys = false } > (target, out_string);

			file << out_string;

			file.close();
		}
	};
}