#pragma once
#include "Dependencies/include/GuiWindow.h"
#include <functional>
#include <vector>
#include <string>

namespace gbe::editor {
    class Timeline : public GuiWindow {
        void DrawSelf() override;

    public:
        struct FloatChannel {
            std::string name;
            std::function<void(float)> setter;
            std::vector<std::pair<int, float>> keyframes; // {Frame Index, Value}
        };

        inline std::string GetWindowId() override { return "TimelineWindow"; }

        inline void AddChannel(const std::string& name, std::function<void(float)> setter) {
            channels.push_back({ name, setter });
        }

    private:
        float GetValueAtFrame(const FloatChannel& channel, int frame);

        int m_currentFrame = 0;
        int m_maxFrames = 600; // Total frames
        int m_fps = 60;
        bool m_isPlaying = false;
        float m_accumulator = 0.0f; // For smooth playback between frames

        static const int MIN_FPS = 1;
        static const int MAX_FPS = 240;
    private:

        std::vector<FloatChannel> channels = {
            {
                "Test",
                [](float) {},
                {
                }
            }
        };
    };
}