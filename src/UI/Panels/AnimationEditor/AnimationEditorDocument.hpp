#pragma once

#include "Types/AnimationClip.hpp"

#include <filesystem>

namespace Diligent::AnimationEditor {
    class EditorDocument {
    public:
        bool Load(const std::filesystem::path& path);
        bool Save();

        const std::filesystem::path& GetPath() const { return m_path; }
        const std::string GetDisplayName() const { return m_path.filename().string(); }
        AnimationClipData& GetClip() { return m_clip; }
        const AnimationClipData& GetClip() const { return m_clip; }
        bool IsDirty() const { return m_dirty; }
        void MarkDirty() { m_dirty = true; }

    private:
        std::filesystem::path m_path;
        AnimationClipData m_clip;
        bool m_dirty = false;
    };
}
