#include "ShaderLoader.hpp"

bgfx::ProgramHandle ShaderLoader::LoadProgram(const std::string& directory, const std::string& vsName, const std::string& fsName)
{
    bgfx::ShaderHandle vsh = LoadShader(directory + "/" + vsName + ".bin");
    bgfx::ShaderHandle fsh = LoadShader(directory + "/" + fsName + ".bin");

    if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
        std::cerr << "[ShaderLoader] Program linkage skipped due to missing shader components." << std::endl;
        return BGFX_INVALID_HANDLE;
    }

    return bgfx::createProgram(vsh, fsh, true);
}

bgfx::ShaderHandle ShaderLoader::LoadShader(const std::string& path)
{
    std::string loadPath = GetPath(path);
    std::ifstream file(loadPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[ShaderLoader] Failed to open: " << loadPath << std::endl;
        return BGFX_INVALID_HANDLE;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    const bgfx::Memory* mem = bgfx::alloc(static_cast<uint32_t>(size));
    if (file.read(reinterpret_cast<char*>(mem->data), size)) {
        return bgfx::createShader(mem);
    }

    std::cerr << "[ShaderLoader] Failed to read contents of: " << loadPath << std::endl;
    return BGFX_INVALID_HANDLE;
}

std::string ShaderLoader::GetPath(const std::string& path)
{
    return BasePath + "/" + path;
}
