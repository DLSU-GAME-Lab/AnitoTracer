#pragma once

#include "FileOpener.hpp"

namespace Diligent::FileOpeners
{
    /// Creates an opener that forwards `.ascript` files to `openInEditor` (e.g. ScriptEditorPanel::OpenFile).
    Opener CreateScriptFileOpener(std::function<void(const std::filesystem::path&)> openInEditor);
}
