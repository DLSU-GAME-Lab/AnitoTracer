#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <string>
#include <system_error>
#include <utility>

namespace Diligent::FileOpeners
{
    using Opener = std::function<void(const std::filesystem::path&)>;
    using StatusReporter = std::function<void(std::string)>;

    inline std::filesystem::path NormalizePath(const std::filesystem::path& path)
    {
        std::error_code error;
        const auto absolute = std::filesystem::absolute(path, error);
        return error ? path.lexically_normal() : absolute.lexically_normal();
    }

    inline bool HasExtension(const std::filesystem::path& path, const char* extension)
    {
        std::string actualExtension = path.extension().string();
        std::transform(actualExtension.begin(), actualExtension.end(), actualExtension.begin(),
            [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        return actualExtension == extension;
    }
}
