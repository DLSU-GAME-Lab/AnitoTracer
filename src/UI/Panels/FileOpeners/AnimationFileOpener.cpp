#include "AnimationFileOpener.hpp"

namespace Diligent::FileOpeners {
    Opener CreateAnimationFileOpener(std::function<void(const std::filesystem::path&)> openInEditor) {
        return [openInEditor = std::move(openInEditor)](const std::filesystem::path& path) {
            if (openInEditor && HasExtension(path, ".anim")) openInEditor(path);
        };
    }
}
