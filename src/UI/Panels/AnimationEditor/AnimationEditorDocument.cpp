#include "AnimationEditorDocument.hpp"

#include "File/Parser.hpp"

#include <fstream>
#include <iostream>

namespace Diligent::AnimationEditor {
    bool EditorDocument::Load(const std::filesystem::path& path) {
        AnimationClipData loaded;
        if (!gbe::Parser::PopulateClass(loaded, path)) {
            std::cerr << "[AnimationEditor] Could not load " << path << '\n';
            return false;
        }
        if (loaded.version != 1) {
            std::cerr << "[AnimationEditor] Unsupported .anim version in " << path << '\n';
            return false;
        }
        if (loaded.framesPerSecond <= 0.0f) loaded.framesPerSecond = 30.0f;
        if (loaded.duration < 0.0f) loaded.duration = 0.0f;
        if (loaded.name.empty()) loaded.name = path.stem().string();

        m_path = path;
        m_clip = std::move(loaded);
        m_dirty = false;
        return true;
    }

    bool EditorDocument::Save() {
        if (m_path.empty()) return false;
        std::ofstream stream(m_path, std::ios::binary | std::ios::trunc);
        if (!stream) return false;
        stream << gbe::Parser::ExportClassStr(m_clip) << '\n';
        stream.flush();
        if (!stream) return false;

        m_dirty = false;
        AnimationClip* asset = AnimationAssetRegistry::FindByPath(m_path);
        if (asset && !asset->LoadFromFile(m_path)) {
            std::cerr << "[AnimationEditor] Saved clip could not be reloaded into the asset database\n";
        }
        return true;
    }
}
