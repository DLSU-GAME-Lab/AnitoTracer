#include "SceneFileOpener.hpp"

#include "ProjectLoader.hpp"

namespace Diligent::FileOpeners
{
    Opener CreateSceneFileOpener()
    {
        return [](const std::filesystem::path& path) {
            if (HasExtension(path, ".ascene"))
                ProjectLoader::RequestSceneLoad(NormalizePath(path));
        };
    }
}
