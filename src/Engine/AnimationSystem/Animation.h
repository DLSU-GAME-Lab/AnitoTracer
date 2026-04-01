#pragma once

#include <vector>
#include <memory>
#include "AnimationFrame.h"
#include "Engine/CameraSystem/Camera.h"

class Animation
{
public:
    Animation(int fps, float duration);
    ~Animation() = default;

    // Frame generation and management
    void GenerateFrames(Camera* camera);
    void ClearFrames();

    // Frame access
    const std::vector<std::shared_ptr<AnimationFrame>>& GetFrames() const;
    std::shared_ptr<AnimationFrame> GetFrame(size_t index);
    size_t GetFrameCount() const;

    // Properties
    void SetFPS(int fps);
    int GetFPS() const;

    void SetDuration(float duration);
    float GetDuration() const;

    float GetFrameTime() const;
    size_t CalculateFrameCount() const;

    // Frame iteration
    std::shared_ptr<AnimationFrame> GetFrameAtTime(float time);

private:
    int m_fps;
    float m_duration;
    std::vector<std::shared_ptr<AnimationFrame>> m_frames;

    float CalculateFrameInterval() const;
};
