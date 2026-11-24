#pragma once

#include "FileOpener.hpp"

namespace Diligent::FileOpeners {
    Opener CreateAnimationFileOpener(std::function<void(const std::filesystem::path&)> openInEditor);
}
