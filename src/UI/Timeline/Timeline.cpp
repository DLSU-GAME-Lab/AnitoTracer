#include "Timeline.h"
#include <algorithm>

namespace gbe::editor {

    float Timeline::GetValueAtFrame(const FloatChannel& channel, int frame) {
        if (channel.keyframes.empty()) return 0.0f;

        // Sort check (ensure frames are in order for lerp)
        auto& kfs = const_cast<std::vector<std::pair<int, float>>&>(channel.keyframes);

        if (frame <= kfs.front().first) return kfs.front().second;
        if (frame >= kfs.back().first) return kfs.back().second;

        for (size_t i = 0; i < kfs.size() - 1; ++i) {
            if (frame >= kfs[i].first && frame <= kfs[i + 1].first) {
                float t = (float)(frame - kfs[i].first) / (float)(kfs[i + 1].first - kfs[i].first);
                return kfs[i].second + t * (kfs[i + 1].second - kfs[i].second);
            }
        }
        return 0.0f;
    }

    void Timeline::DrawSelf() {
        static float zoom = 2.0f; // Pixels per frame now
        ImGuiIO& io = ImGui::GetIO();

        // --- TOP CONTROLS ---
        if (ImGui::Button(m_isPlaying ? "Pause" : "Play")) m_isPlaying = !m_isPlaying;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::DragInt("FPS", &m_fps, 1, MIN_FPS, MAX_FPS);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::DragInt("Frames", &m_maxFrames, 10, 1, 10000);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::SliderFloat("Zoom", &zoom, 0.5f, 50.0f);

        // Playback Logic
        if (m_isPlaying) {
            m_accumulator += io.DeltaTime;
            float frameTime = 1.0f / (float)m_fps;
            while (m_accumulator >= frameTime) {
                m_currentFrame++;
                m_accumulator -= frameTime;
            }
            if (m_currentFrame > m_maxFrames) m_currentFrame = 0;

            for (auto& chan : channels) if (chan.setter) chan.setter(GetValueAtFrame(chan, m_currentFrame));
        }

        // --- MASTER CONTAINER ---
        ImGui::BeginChild("TimelineMaster", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 startPos = ImGui::GetCursorScreenPos();
        float labelColWidth = 180.0f;
        float rowHeight = 45.0f;
        float trackStartX = startPos.x + labelColWidth;

        for (int i = 0; i < (int)channels.size(); i++) {
            auto& channel = channels[i];
            ImVec2 rowPos = ImVec2(startPos.x, startPos.y + (i * rowHeight) + 20.0f);

            // 1. STICKY LABELS
            ImGui::SetCursorScreenPos(ImVec2(rowPos.x + ImGui::GetScrollX(), rowPos.y));
            ImGui::BeginGroup();
            ImGui::Text("%s [F:%d]", channel.name.c_str(), m_currentFrame);

            float val = GetValueAtFrame(channel, m_currentFrame);
            ImGui::SetNextItemWidth(70);
            ImGui::PushID(i);
            if (ImGui::DragFloat("##v", &val, 0.1f)) {
                auto it = std::find_if(channel.keyframes.begin(), channel.keyframes.end(),
                    [this](const std::pair<int, float>& k) { return k.first == m_currentFrame; });

                if (it != channel.keyframes.end()) it->second = val;
                else {
                    channel.keyframes.push_back({ m_currentFrame, val });
                    std::sort(channel.keyframes.begin(), channel.keyframes.end());
                }
                if (channel.setter) channel.setter(val);
            }
            ImGui::PopID();
            ImGui::EndGroup();

            // 2. TRACK BACKGROUND
            drawList->AddRectFilled(ImVec2(trackStartX, rowPos.y), ImVec2(trackStartX + (m_maxFrames * zoom), rowPos.y + rowHeight), IM_COL32(35, 35, 35, 255));

            // 3. KEYFRAMES
            for (int k = 0; k < (int)channel.keyframes.size(); k++) {
                float x = trackStartX + (channel.keyframes[k].first * zoom);
                float y = rowPos.y + (rowHeight * 0.5f);

                ImGui::PushID(k);
                ImGui::SetCursorScreenPos(ImVec2(x - 6, y - 6));
                ImGui::InvisibleButton("##kf", ImVec2(12, 12));

                // Context Menu (Delete)
                if (ImGui::BeginPopupContextItem("kf_ctx")) {
                    if (ImGui::MenuItem("Delete Keyframe")) {
                        channel.keyframes.erase(channel.keyframes.begin() + k);
                        ImGui::EndPopup();
                        ImGui::PopID();
                        break; // Exit loop to avoid iterator invalidation
                    }
                    ImGui::EndPopup();
                }

                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
                    // Calculate new frame based on mouse movement
                    int deltaFrames = (int)(io.MouseDelta.x / zoom);
                    int newFrame = std::clamp(channel.keyframes[k].first + deltaFrames, 0, m_maxFrames);

                    // Collision Prevention: Check neighbors
                    int minFrame = (k > 0) ? channel.keyframes[k - 1].first + 1 : 0;
                    int maxFrame = (k < (int)channel.keyframes.size() - 1) ? channel.keyframes[k + 1].first - 1 : m_maxFrames;

                    channel.keyframes[k].first = std::clamp(newFrame, minFrame, maxFrame);
                }

                drawList->AddRectFilled(ImVec2(x - 4, y - 4), ImVec2(x + 4, y + 4), ImGui::IsItemActive() ? IM_COL32(255, 255, 0, 255) : IM_COL32(200, 200, 200, 255), 2.0f);
                ImGui::PopID();
            }
        }

        // 4. DRAW RULERS (Snapping every 10 frames or based on zoom)
        for (int f = 0; f <= m_maxFrames; f += (zoom < 2.0f ? 10 : 1)) {
            float x = trackStartX + (f * zoom);
            drawList->AddLine(ImVec2(x, startPos.y), ImVec2(x, startPos.y + 15), IM_COL32(100, 100, 100, 255));
            if (f % 10 == 0) {
                char buf[16]; sprintf(buf, "%d", f);
                drawList->AddText(ImVec2(x + 2, startPos.y), IM_COL32(150, 150, 150, 255), buf);
            }
        }

        // 5. PLAYHEAD (Snapping)
        float playheadX = trackStartX + (m_currentFrame * zoom);
        drawList->AddLine(ImVec2(playheadX, startPos.y), ImVec2(playheadX, startPos.y + 500), IM_COL32(255, 0, 0, 255), 2.0f);

        ImGui::SetCursorScreenPos(ImVec2(playheadX - 10, startPos.y));
        ImGui::InvisibleButton("##ph", ImVec2(20, 500));
        if (ImGui::IsItemActive()) {
            // Snapping playhead to integer frame
            m_currentFrame = (int)((io.MousePos.x - trackStartX) / zoom);
            m_currentFrame = std::clamp(m_currentFrame, 0, m_maxFrames);
            for (auto& chan : channels) if (chan.setter) chan.setter(GetValueAtFrame(chan, m_currentFrame));
        }

        ImGui::EndChild();
    }
}