#pragma once

#include <fstream>
#include <windows.h>
#include <iostream>
#include <filesystem>


class FileUtils
{
public:
	static std::filesystem::path getAssetsFolderPath();
	static std::filesystem::path getExecutablePath();
	static std::filesystem::path getProjectFolderPath();

	static bool getModelFilePath(std::string& filePath, std::string& fileName);
	static bool getTextureFilePath(std::string& filePath, std::string& fileName);
	static bool getLayoutFilePath(std::string& filePath, std::string& fileName);
	static bool getSceneFilePath(std::string& filePath, std::string& fileName); 


	//void getModelFilePath(std::string* filePath);
};

