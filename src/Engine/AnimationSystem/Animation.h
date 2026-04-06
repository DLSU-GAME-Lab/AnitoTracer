#pragma once

#include <vector>
#include <memory>
#include "AnimationFrame.h"
#include "Engine/CameraSystem/Camera.h"
#include <iostream>

class Animation
{
public:
    // Singleton access
    static Animation* getInstance();
    static void destroyInstance();

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

    // Initialize the singleton with default values
    void Initialize(int fps = 30, float duration = 1.0f);

private:
    // Private constructor/destructor for singleton pattern
    Animation(int fps, float duration);
    ~Animation() = default;

    // Prevent copying
    Animation(const Animation&) = delete;
    Animation& operator=(const Animation&) = delete;

    int m_fps;
    float m_duration;
    std::vector<std::shared_ptr<AnimationFrame>> m_frames;

    float CalculateFrameInterval() const;

    static Animation* s_instance;
};
