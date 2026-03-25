#pragma once
#include <cstdio>
#include <string>

namespace Export
{
    std::string MakeTimestamp();
    void SavePNG(const std::string& baseName, uint32_t width, uint32_t height, uint32_t bytesPerPixel, const void* data);
}
