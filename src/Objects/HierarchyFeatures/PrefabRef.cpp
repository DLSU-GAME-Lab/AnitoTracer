#include "PrefabRef.hpp"

#include "ProjectLoader.hpp"

#include <algorithm>

namespace
{
    std::filesystem::path GetRootDirectory()
    {
        std::filesystem::path root = ProjectLoader::GetCurrentProjectDir();
        return root.empty() ? std::filesystem::current_path() : root;
    }
}

std::filesystem::path PrefabRef::Resolve() const
{
    if (m_path.empty())
        return {};

    std::filesystem::path path(m_path);
    if (!path.is_absolute())
        path = GetRootDirectory() / path;

    std::error_code error;
    const auto absolute = std::filesystem::absolute(path, error);
    return (error ? path : absolute).lexically_normal();
}

std::string PrefabRef::MakeStorable(const std::filesystem::path &absolutePath)
{
    std::error_code error;
    const auto relative = std::filesystem::relative(absolutePath, GetRootDirectory(), error);
    if (error || relative.empty() || *relative.begin() == "..")
        return absolutePath.generic_string();
    return relative.generic_string();
}

std::vector<std::filesystem::path> PrefabRef::EnumeratePrefabs()
{
    std::vector<std::filesystem::path> result;
    std::error_code error;
    std::filesystem::recursive_directory_iterator it(
        GetRootDirectory(), std::filesystem::directory_options::skip_permission_denied, error);

    for (const std::filesystem::recursive_directory_iterator end; !error && it != end; it.increment(error))
    {
        if (it->is_regular_file(error) && it->path().extension() == ".aprefab")
            result.push_back(it->path());
    }

    std::sort(result.begin(), result.end());
    return result;
}
