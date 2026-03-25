#include "Screenshot.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <ctime>
#include "FileExplorer/FileExplorerConstants.h"

std::string Export::MakeTimestamp()
{
    std::time_t t = std::time(nullptr);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d_%02d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(buffer);
}
void Export::SavePNG(const std::string& baseName, uint32_t width, uint32_t height, uint32_t bytesPerPixel, const void* data)
{
    const std::string filename =
        (baseName.empty() ? "screenshot" : baseName)
        + "_" + MakeTimestamp() + ".png";

    auto fullPath = std::string(FileExplorerConstants::ASSETS_DIR) + "/" + filename;
    const uint32_t stride = width * bytesPerPixel;
    stbi_write_png(fullPath.c_str(), width, height, bytesPerPixel, data, stride);
}