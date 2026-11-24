#include "AnimationEditorPanel.hpp"

#include "Components/Animation/AnimationComponent.hpp"
#include "Components/Transform.hpp"
#include "FileOpeners/FileOpener.hpp"
#include "GUIManager.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace {
    constexpr float TrackRowHeight = 22.0f;
    constexpr float LabelWidth = 190.0f;

    void SortKeys(AnimationCurve& curve) {
        std::stable_sort(curve.keys.begin(), curve.keys.end(),
            [](const AnimationKeyframe& left, const AnimationKeyframe& right) {
                return left.time < right.time;
            });
    }

}

namespace Diligent {
    AnimationEditorPanel::AnimationEditorPanel(const std::string& name) : BasePanel(name) {}

    bool AnimationEditorPanel::OpenFile(const std::filesystem::path& path) {
        const auto normalized = FileOpeners::NormalizePath(path);
        BeginOpen(normalized);
        return !m_pendingPath.empty() || (m_document && m_document->GetPath() == normalized);
    }

    void AnimationEditorPanel::BeginOpen(const std::filesystem::path& path) {
        if (m_document && m_document->GetPath() == path) {
            Open();
            return;
        }
        if (m_document && m_document->IsDirty() &&
            m_document->GetPath() != path) {
            m_pendingPath = path;
            m_openConfirmation = true;
            Open();
            return;
        }
        FinishOpen(path);
    }

    void AnimationEditorPanel::FinishOpen(const std::filesystem::path& path) {
        RestorePreviewState();
        auto document = std::make_unique<AnimationEditor::EditorDocument>();
        if (!document->Load(path)) {
            m_status = "Failed to open " + path.filename().string();
            Open();
            return;
        }

        m_document = std::move(document);
        m_target = GUIManager::GetInstance().GetSelectedObject();
        m_playhead = 0.0f;
        m_playing = false;
        m_selected = {};
        m_status.clear();
        Open();
    }

    bool AnimationEditorPanel::Save() {
        if (!m_document || m_document->Save()) {
            m_status.clear();
            return true;
        }
        m_status = "Failed to save animation clip";
        return false;
    }

    void AnimationEditorPanel::Draw() {
        if (!m_IsVisible) return;
        if (ImGui::Begin(m_Name.c_str(), &m_IsVisible)) {
            if (!m_document) {
                ImGui::TextDisabled("Open a .anim file from the File Explorer.");
            } else {
                DrawToolbar();
                DrawDopesheet();
                DrawKeyInspector();
                if (!m_status.empty()) ImGui::TextDisabled("%s", m_status.c_str());
            }
            DrawCloseConfirmation();
        }
        if (!m_IsVisible) {
            if (m_document && m_document->IsDirty()) {
                m_IsVisible = true;
                m_closeRequested = true;
            } else {
                RestorePreviewState();
            }
        }
        ImGui::End();
    }

    void AnimationEditorPanel::DrawToolbar() {
        AnimationClipData& clip = m_document->GetClip();
        if (ImGui::Button("Save")) Save();
        ImGui::SameLine();
        if (ImGui::Button("Add Track")) AddTrackForSelectedObject();
        ImGui::SameLine();
        if (ImGui::Button("Record")) RecordCurrentTransform();
        ImGui::SameLine();
        if (ImGui::Button("Add Key")) AddKeyForSelection();
        ImGui::SameLine();
        if (ImGui::Button("Play")) {
            m_playing = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Pause")) m_playing = false;
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            m_playing = false;
            m_playhead = 0.0f;
            PreviewAt(m_playhead);
        }
        ImGui::SameLine();
        if (ImGui::Button("Exit Preview")) RestorePreviewState();

        ImGui::Separator();
        char name[128] = {};
        std::copy_n(clip.name.c_str(), std::min(clip.name.size(), sizeof(name) - 1), name);
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::InputText("Name", name, sizeof(name))) {
            clip.name = name;
            m_document->MarkDirty();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90.0f);
        if (ImGui::InputFloat("FPS", &clip.framesPerSecond, 0.0f, 0.0f, "%.1f") &&
            clip.framesPerSecond > 0.0f) {
            m_document->MarkDirty();
        }
        if (clip.framesPerSecond <= 0.0f) clip.framesPerSecond = 1.0f;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90.0f);
        if (ImGui::InputFloat("Duration", &clip.duration, 0.0f, 0.0f, "%.3f")) {
            clip.duration = std::max(0.0f, clip.duration);
            m_playhead = std::min(m_playhead, clip.duration);
            m_document->MarkDirty();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Loop", &clip.loop)) m_document->MarkDirty();

        HierarchyObject::Ref selected = GUIManager::GetInstance().GetSelectedObject();
        ImGui::TextDisabled("Target: %s", m_target.GetPtr() ? m_target.GetPtr()->GetName().c_str() : "(select a hierarchy object)");
        ImGui::SameLine();
        if (selected && selected != m_target) {
            if (ImGui::SmallButton("Use Selection")) {
                RestorePreviewState();
                m_target = selected;
                PreviewAt(m_playhead);
            }
        }

        if (m_playing && clip.duration > 0.0f) {
            m_playhead += ImGui::GetIO().DeltaTime;
            if (m_playhead > clip.duration) {
                if (clip.loop) m_playhead = std::fmod(m_playhead, clip.duration);
                else {
                    m_playhead = clip.duration;
                    m_playing = false;
                }
            }
            PreviewAt(m_playhead);
        }
    }

    void AnimationEditorPanel::DrawDopesheet() {
        AnimationClipData& clip = m_document->GetClip();
        const float duration = std::max(clip.duration, 0.001f);
        if (!ImGui::BeginChild("##animation_dopesheet", ImVec2(0, -90.0f), true,
            ImGuiWindowFlags_HorizontalScrollbar)) {
            ImGui::EndChild();
            return;
        }

        const ImVec2 timelineStart = ImGui::GetCursorScreenPos();
        const float timelineX = timelineStart.x + LabelWidth;
        const float timelineWidth = std::max(120.0f, ImGui::GetContentRegionAvail().x - LabelWidth);
        ImGui::SetCursorScreenPos(ImVec2(timelineX, timelineStart.y));
        ImGui::BeginDisabled(clip.tracks.empty());
        if (ImGui::InvisibleButton("##ruler", ImVec2(timelineWidth, 22.0f),
            ImGuiButtonFlags_MouseButtonLeft)) {
            const float relative = std::clamp((ImGui::GetIO().MousePos.x - timelineX) / timelineWidth, 0.0f, 1.0f);
            m_playhead = relative * duration;
            PreviewAt(m_playhead);
        }
        ImGui::EndDisabled();
        ImGui::SetCursorScreenPos(timelineStart);
        ImGui::Dummy(ImVec2(LabelWidth, 22.0f));
        ImGui::SameLine();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(ImVec2(timelineX, timelineStart.y),
            ImVec2(timelineX + timelineWidth, timelineStart.y + 22.0f),
            IM_COL32(38, 38, 42, 255));
        const int frameLines = std::max(1, static_cast<int>(duration * clip.framesPerSecond));
        for (int frame = 0; frame <= frameLines; ++frame) {
            const float x = timelineX + timelineWidth * (static_cast<float>(frame) / frameLines);
            drawList->AddLine(ImVec2(x, timelineStart.y), ImVec2(x, timelineStart.y + 6.0f),
                IM_COL32(130, 130, 140, 255));
        }

        for (std::size_t trackIndex = 0; trackIndex < clip.tracks.size(); ++trackIndex) {
            const AnimationTrack& track = clip.tracks[trackIndex];
            ImGui::PushID(static_cast<int>(trackIndex));
            ImGui::TextUnformatted(track.relativePath.empty() ? "." : track.relativePath.c_str());
            ImGui::Separator();
            for (int group = 0; group < 3; ++group) {
                for (int channel = 0; channel < 3; ++channel) {
                    ImGui::PushID(group * 3 + channel);
                    ImGui::Text("  %s.%s", GroupName(group), ChannelName(channel));
                    ImGui::SameLine(LabelWidth);
                    const ImVec2 rowStart = ImGui::GetCursorScreenPos();
                    const ImVec2 rowEnd(rowStart.x + timelineWidth, rowStart.y + TrackRowHeight);
                    drawList->AddRectFilled(rowStart, rowEnd,
                        (trackIndex % 2 == 0) ? IM_COL32(31, 31, 35, 255) : IM_COL32(34, 34, 39, 255));
                    ImGui::InvisibleButton("##track_row", ImVec2(timelineWidth, TrackRowHeight));
                    const ImVec2 rowCursorAfter = ImGui::GetCursorScreenPos();
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                        m_playhead = std::clamp(
                            (ImGui::GetIO().MousePos.x - rowStart.x) / timelineWidth, 0.0f, 1.0f) * duration;
                        m_selected = { trackIndex, group, channel, 0, true, false };
                        PreviewAt(m_playhead);
                    }

                    AnimationCurve* curve = GetCurve(trackIndex, group, channel);
                    if (!curve) continue;
                    for (std::size_t keyIndex = 0; keyIndex < curve->keys.size(); ++keyIndex) {
                        AnimationKeyframe& key = curve->keys[keyIndex];
                        const float x = rowStart.x + timelineWidth * std::clamp(key.time / duration, 0.0f, 1.0f);
                        const float y = rowStart.y + TrackRowHeight * 0.5f;
                        const ImVec2 points[] = {
                            {x, y - 5.0f}, {x + 5.0f, y}, {x, y + 5.0f}, {x - 5.0f, y}
                        };
                        const bool isSelected = m_selected.valid && m_selected.track == trackIndex &&
                            m_selected.group == group && m_selected.channel == channel &&
                            m_selected.key == keyIndex;
                        drawList->AddConvexPolyFilled(points, 4,
                            isSelected ? IM_COL32(255, 208, 80, 255) : IM_COL32(90, 180, 255, 255));
                        ImGui::SetCursorScreenPos(ImVec2(x - 7.0f, y - 8.0f));
                        ImGui::PushID(static_cast<int>(keyIndex));
                        ImGui::InvisibleButton("##key", ImVec2(14.0f, 16.0f));
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            m_selected = { trackIndex, group, channel, keyIndex, true, true };
                        }
                        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                            const float frame = 1.0f / std::max(1.0f, clip.framesPerSecond);
                            key.time = std::round(std::clamp(
                                (ImGui::GetIO().MousePos.x - rowStart.x) / timelineWidth, 0.0f, 1.0f) *
                                clip.duration / frame) * frame;
                            m_playhead = key.time;
                            m_document->MarkDirty();
                            PreviewAt(m_playhead);
                        }
                        if (ImGui::IsItemDeactivatedAfterEdit()) {
                            const AnimationKeyframe moved = key;
                            SortKeys(*curve);
                            const auto movedKey = std::find_if(curve->keys.begin(), curve->keys.end(),
                                [&](const AnimationKeyframe& candidate) {
                                    return std::abs(candidate.time - moved.time) < 0.0001f &&
                                        std::abs(candidate.value - moved.value) < 0.0001f &&
                                        candidate.interpolation == moved.interpolation;
                                });
                            if (movedKey != curve->keys.end()) {
                                m_selected.key = static_cast<std::size_t>(movedKey - curve->keys.begin());
                            }
                        }
                        ImGui::PopID();
                        ImGui::SetCursorScreenPos(rowCursorAfter);
                    }
                    ImGui::PopID();
                }
            }
            ImGui::PopID();
        }

        const float playheadX = timelineX + timelineWidth * std::clamp(m_playhead / duration, 0.0f, 1.0f);
        drawList->AddLine(ImVec2(playheadX, timelineStart.y),
            ImVec2(playheadX, ImGui::GetCursorScreenPos().y), IM_COL32(255, 90, 90, 220), 2.0f);
        ImGui::EndChild();
    }

    void AnimationEditorPanel::DrawKeyInspector() {
        AnimationKeyframe* key = GetSelectedKey();
        if (!key) {
            if (m_selected.hasSelection &&
                m_selected.track < m_document->GetClip().tracks.size() &&
                ImGui::Button("Delete Track")) {
                m_document->GetClip().tracks.erase(
                    m_document->GetClip().tracks.begin() + m_selected.track);
                m_selected = {};
                m_document->MarkDirty();
            }
            ImGui::TextDisabled("Select a keyframe to edit it.");
            return;
        }

        bool changed = ImGui::InputFloat("Time", &key->time, 1.0f / std::max(
            1.0f, m_document->GetClip().framesPerSecond), 0.0f);
        changed |= ImGui::InputFloat("Value", &key->value);
        int interpolation = static_cast<int>(key->interpolation);
        const char* items[] = { "Linear", "Step" };
        if (ImGui::Combo("Interpolation", &interpolation, items, IM_ARRAYSIZE(items))) {
            key->interpolation = static_cast<AnimationInterpolation>(interpolation);
            changed = true;
        }
        if (ImGui::Button("Delete Key")) {
            if (AnimationCurve* curve = GetCurve(m_selected.track, m_selected.group, m_selected.channel)) {
                if (m_selected.key < curve->keys.size()) curve->keys.erase(curve->keys.begin() + m_selected.key);
            }
            m_selected = {};
            m_document->MarkDirty();
            PreviewAt(m_playhead);
            return;
        }
        if (changed) {
            key->time = std::clamp(key->time, 0.0f, m_document->GetClip().duration);
            const AnimationKeyframe changedKey = *key;
            if (AnimationCurve* curve = GetCurve(m_selected.track, m_selected.group, m_selected.channel)) {
                SortKeys(*curve);
                const auto movedKey = std::find_if(curve->keys.begin(), curve->keys.end(),
                    [&](const AnimationKeyframe& candidate) {
                        return std::abs(candidate.time - changedKey.time) < 0.0001f &&
                            std::abs(candidate.value - changedKey.value) < 0.0001f &&
                            candidate.interpolation == changedKey.interpolation;
                    });
                if (movedKey != curve->keys.end()) {
                    m_selected.key = static_cast<std::size_t>(movedKey - curve->keys.begin());
                }
            }
            m_document->MarkDirty();
            PreviewAt(m_playhead);
        }
    }

    void AnimationEditorPanel::DrawCloseConfirmation() {
        if (!m_openConfirmation && !m_closeRequested) return;
        ImGui::OpenPopup("Unsaved Animation Clip");
        if (!ImGui::BeginPopupModal("Unsaved Animation Clip", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;
        ImGui::TextUnformatted("Save changes before switching clips?");
        if (ImGui::Button("Save")) {
            if (Save()) {
                if (m_openConfirmation) FinishOpen(m_pendingPath);
                else {
                    RestorePreviewState();
                    m_document.reset();
                    m_IsVisible = false;
                }
                m_openConfirmation = m_closeRequested = false;
                m_pendingPath.clear();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard")) {
            if (m_openConfirmation) FinishOpen(m_pendingPath);
            else {
                RestorePreviewState();
                m_document.reset();
                m_IsVisible = false;
            }
            m_openConfirmation = m_closeRequested = false;
            m_pendingPath.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_openConfirmation = m_closeRequested = false;
            m_pendingPath.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    AnimationCurve* AnimationEditorPanel::GetCurve(std::size_t trackIndex, int group, int channel) {
        if (!m_document || trackIndex >= m_document->GetClip().tracks.size() || channel < 0 || channel > 2)
            return nullptr;
        AnimationTrack& track = m_document->GetClip().tracks[trackIndex];
        if (group == 0) return &track.position[channel];
        if (group == 1) return &track.rotation[channel];
        if (group == 2) return &track.scale[channel];
        return nullptr;
    }

    AnimationKeyframe* AnimationEditorPanel::GetSelectedKey() {
        AnimationCurve* curve = GetCurve(m_selected.track, m_selected.group, m_selected.channel);
        if (!m_selected.hasSelection || !m_selected.valid || !curve ||
            m_selected.key >= curve->keys.size()) return nullptr;
        return &curve->keys[m_selected.key];
    }

    const char* AnimationEditorPanel::GroupName(int group) {
        static const char* names[] = { "Position", "Rotation", "Scale" };
        return group >= 0 && group < 3 ? names[group] : "";
    }

    const char* AnimationEditorPanel::ChannelName(int channel) {
        static const char* names[] = { "X", "Y", "Z" };
        return channel >= 0 && channel < 3 ? names[channel] : "";
    }

    void AnimationEditorPanel::CapturePreviewState() {
        if (m_previewActive || !m_target) return;
        const auto capture = [this](auto&& self, HierarchyObject::Ref object) -> void {
            if (!object) return;
            if (Transform* transform = object.GetPtr()->GetTransform()) {
                m_previewSnapshots.push_back({
                    object,
                    transform->GetLocalPosition(),
                    transform->GetEulerAnglesDegrees(),
                    transform->GetLocalScale()
                });
            }
            for (const auto& child : object.GetPtr()->GetChildren()) self(self, child.get());
        };
        capture(capture, m_target);
        m_previewActive = true;
    }

    void AnimationEditorPanel::RestorePreviewState() {
        if (!m_previewActive) return;
        for (const TransformSnapshot& snapshot : m_previewSnapshots) {
            if (Transform* transform = snapshot.object.GetPtr() ? snapshot.object.GetPtr()->GetTransform() : nullptr) {
                transform->SetPosition(snapshot.position);
                transform->SetEulerAnglesDegrees(snapshot.rotation);
                transform->SetScale(snapshot.scale);
                transform->SyncPhysics();
            }
        }
        m_previewSnapshots.clear();
        m_previewActive = false;
        m_playing = false;
    }

    void AnimationEditorPanel::PreviewAt(float time) {
        if (!m_document || !m_target) return;
        CapturePreviewState();
        RestorePreviewState();
        CapturePreviewState();
        AnimationComponent::EvaluateClip(m_document->GetClip(), time, m_target);
    }

    void AnimationEditorPanel::AddTrackForSelectedObject() {
        if (!m_document) return;
        HierarchyObject::Ref selected = GUIManager::GetInstance().GetSelectedObject();
        if (!selected || !m_target) {
            m_status = "Select an object below the animation target first.";
            return;
        }
        const std::string relativePath = AnimationComponent::GetRelativePath(m_target, selected);
        if (relativePath.empty()) {
            m_status = "Selected object is not below the current animation target.";
            return;
        }
        AnimationClipData& clip = m_document->GetClip();
        const auto found = std::find_if(clip.tracks.begin(), clip.tracks.end(),
            [&](const AnimationTrack& track) { return track.relativePath == relativePath; });
        if (found == clip.tracks.end()) {
            AnimationTrack track;
            track.relativePath = relativePath;
            track.objectGuid = selected.GetPtr()->GetGUID().ToString();
            clip.tracks.push_back(std::move(track));
            m_selected = { clip.tracks.size() - 1, 0, 0, 0, true, false };
            m_document->MarkDirty();
        } else {
            found->objectGuid = selected.GetPtr()->GetGUID().ToString();
            m_document->MarkDirty();
        }
        m_status.clear();
    }

    void AnimationEditorPanel::AddKeyForSelection() {
        if (!m_document || !m_selected.hasSelection ||
            m_selected.track >= m_document->GetClip().tracks.size()) {
            m_status = "Select a dopesheet channel before adding a key.";
            return;
        }

        AnimationCurve* curve = GetCurve(m_selected.track, m_selected.group, m_selected.channel);
        AnimationTrack& track = m_document->GetClip().tracks[m_selected.track];
        HierarchyObject* object = AnimationComponent::ResolveRelativeObject(
            m_target, track.relativePath).GetPtr();
        Transform* transform = object ? object->GetTransform() : nullptr;
        if (!curve || !transform) {
            m_status = "The selected track cannot resolve a transform.";
            return;
        }

        glm::vec3 value = m_selected.group == 0 ? transform->GetLocalPosition() :
            (m_selected.group == 1 ? transform->GetEulerAnglesDegrees() : transform->GetLocalScale());
        const auto existing = std::find_if(curve->keys.begin(), curve->keys.end(),
            [this](const AnimationKeyframe& key) {
                return std::abs(key.time - m_playhead) < 0.0001f;
            });
        if (existing == curve->keys.end()) {
            curve->keys.push_back({m_playhead, value[m_selected.channel]});
            m_selected.key = curve->keys.size() - 1;
        } else {
            existing->value = value[m_selected.channel];
            m_selected.key = static_cast<std::size_t>(existing - curve->keys.begin());
        }
        m_selected.hasSelection = true;
        m_selected.valid = true;
        SortKeys(*curve);
        const auto selectedKey = std::find_if(curve->keys.begin(), curve->keys.end(),
            [this](const AnimationKeyframe& key) {
                return std::abs(key.time - m_playhead) < 0.0001f;
            });
        if (selectedKey != curve->keys.end()) {
            m_selected.key = static_cast<std::size_t>(selectedKey - curve->keys.begin());
        }
        m_document->MarkDirty();
        m_status.clear();
    }

    void AnimationEditorPanel::RecordCurrentTransform() {
        AddTrackForSelectedObject();
        HierarchyObject::Ref selected = GUIManager::GetInstance().GetSelectedObject();
        Transform* transform = selected.GetPtr() ? selected.GetPtr()->GetTransform() : nullptr;
        if (!transform || !m_document || !m_target) return;

        const std::string relativePath = AnimationComponent::GetRelativePath(m_target, selected);
        auto& tracks = m_document->GetClip().tracks;
        auto trackIt = std::find_if(tracks.begin(), tracks.end(),
            [&](const AnimationTrack& track) { return track.relativePath == relativePath; });
        if (trackIt == tracks.end()) return;

        const glm::vec3 values[] = {
            transform->GetLocalPosition(),
            transform->GetEulerAnglesDegrees(),
            transform->GetLocalScale()
        };
        const auto write = [this](AnimationCurve& curve, float value) {
            auto key = std::find_if(curve.keys.begin(), curve.keys.end(),
                [this](const AnimationKeyframe& existing) {
                    return std::abs(existing.time - m_playhead) < 0.0001f;
                });
            if (key == curve.keys.end()) curve.keys.push_back({m_playhead, value});
            else key->value = value;
            SortKeys(curve);
        };
        for (int group = 0; group < 3; ++group) {
            for (int channel = 0; channel < 3; ++channel) {
                AnimationCurve* curve = GetCurve(
                    static_cast<std::size_t>(trackIt - tracks.begin()), group, channel);
                if (curve) write(*curve, values[group][channel]);
            }
        }
        m_document->MarkDirty();
    }
}
