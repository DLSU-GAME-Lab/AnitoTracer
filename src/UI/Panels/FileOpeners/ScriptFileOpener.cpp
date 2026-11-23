#include "ScriptFileOpener.hpp"

namespace Diligent::FileOpeners
{
    Opener CreateScriptFileOpener(std::function<void(const std::filesystem::path&)> openInEditor)
    {
        return [openInEditor = std::move(openInEditor)](const std::filesystem::path& path) {
            if (openInEditor && HasExtension(path, ".ascript"))
                openInEditor(path);
        };
    }
}
