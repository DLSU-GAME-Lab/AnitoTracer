#pragma once

#include <fstream>
#include <windows.h>
#include <iostream>
#include <filesystem>

class FileUtils
{
public:
	static std::filesystem::path getAssetsFolderPath();
	static std::filesystem::path getSceneSavePath();
	static std::filesystem::path getExecutablePath();

	static bool getModelFilePath(std::string& filePath, std::string& fileName);
	static bool getScenePath(std::string& filePath, std::string& fileName);
	//void getModelFilePath(std::string* filePath);
};

