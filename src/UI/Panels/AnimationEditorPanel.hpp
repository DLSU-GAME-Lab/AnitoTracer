#pragma once

#include "BasePanel.hpp"
#include "AnimationEditor/AnimationEditorDocument.hpp"
#include "HierarchyObject.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace Diligent {
    class AnimationEditorPanel final : public BasePanel {
    public:
        explicit AnimationEditorPanel(const std::string& name = "Animation Editor");
        bool OpenFile(const std::filesystem::path& path);
        void Draw() override;

    private:
        struct TransformSnapshot {
            HierarchyObject::Ref object;
            glm::vec3 position;
            glm::vec3 rotation;
            glm::vec3 scale;
        };

        struct SelectedKey {
            std::size_t track = 0;
            int group = 0;
            int channel = 0;
            std::size_t key = 0;
            bool hasSelection = false;
            bool valid = false;
        };

        void DrawToolbar();
        void DrawDopesheet();
        void DrawKeyInspector();
        void DrawCloseConfirmation();
        void PreviewAt(float time);
        void CapturePreviewState();
        void RestorePreviewState();
        void RecordCurrentTransform();
        void AddKeyForSelection();
        void AddTrackForSelectedObject();
        void BeginOpen(const std::filesystem::path& path);
        void FinishOpen(const std::filesystem::path& path);
        bool Save();
        AnimationKeyframe* GetSelectedKey();
        AnimationCurve* GetCurve(std::size_t track, int group, int channel);
        static const char* GroupName(int group);
        static const char* ChannelName(int channel);

        std::unique_ptr<AnimationEditor::EditorDocument> m_document;
        std::filesystem::path m_pendingPath;
        HierarchyObject::Ref m_target;
        std::vector<TransformSnapshot> m_previewSnapshots;
        SelectedKey m_selected;
        float m_playhead = 0.0f;
        bool m_playing = false;
        bool m_previewActive = false;
        bool m_openConfirmation = false;
        bool m_closeRequested = false;
        std::string m_status;
    };
}
