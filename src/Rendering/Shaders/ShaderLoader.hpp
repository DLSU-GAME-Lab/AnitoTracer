#pragma once

#include <bgfx/bgfx.h>
#include <string>

#include <fstream>
#include <iostream>

class ShaderLoader {

public:

	//All paths are relative to here
	//Can be found along side exe desu
	inline static const std::string BasePath = "shaders";

	static bgfx::ProgramHandle LoadProgram(
		const std::string& directory,
		const std::string& vsName,
		const std::string& fsName
	);

	static bgfx::ShaderHandle LoadShader(const std::string& path);

	static std::string GetPath(const std::string& path);

};