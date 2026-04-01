#include "Animation.h"
#include <cmath>

Animation::Animation(int fps, float duration)
    : m_fps(fps), m_duration(duration), m_frames()
{
    if (m_fps <= 0)
    {
        m_fps = 30;
    }
    if (m_duration <= 0.0f)
    {
        m_duration = 1.0f;
    }
}

void Animation::GenerateFrames(Camera* camera)
{
    if (camera == nullptr)
    {
        return;
    }

    ClearFrames();

    const std::vector<KeyFrame*>& cameraKeyFrames = camera->getKeyFrames();
    size_t frameCount = CalculateFrameCount();

    if (frameCount == 0 || cameraKeyFrames.empty())
    {
        return;
    }

    // Generate frames based on fps and duration
    for (size_t i = 0; i < frameCount; ++i)
    {
        auto frame = std::make_shared<AnimationFrame>();

        // Calculate which keyframe to use based on frame index
        size_t keyFrameIndex = (i * cameraKeyFrames.size()) / frameCount;
        if (keyFrameIndex >= cameraKeyFrames.size())
        {
            keyFrameIndex = cameraKeyFrames.size() - 1;
        }

        // Add the keyframe to the animation frame
        const KeyFrame& keyFrame = *cameraKeyFrames[keyFrameIndex];
        frame->AddKeyFrame("camera", keyFrame);

        m_frames.push_back(frame);
    }
}

void Animation::ClearFrames()
{
    m_frames.clear();
}

const std::vector<std::shared_ptr<AnimationFrame>>& Animation::GetFrames() const
{
    return m_frames;
}

std::shared_ptr<AnimationFrame> Animation::GetFrame(size_t index)
{
    if (index < m_frames.size())
    {
        return m_frames[index];
    }
    return nullptr;
}

size_t Animation::GetFrameCount() const
{
    return m_frames.size();
}

void Animation::SetFPS(int fps)
{
    if (fps > 0)
    {
        m_fps = fps;
    }
}

int Animation::GetFPS() const
{
    return m_fps;
}

void Animation::SetDuration(float duration)
{
    if (duration > 0.0f)
    {
        m_duration = duration;
    }
}

float Animation::GetDuration() const
{
    return m_duration;
}

float Animation::GetFrameTime() const
{
    return CalculateFrameInterval();
}

size_t Animation::CalculateFrameCount() const
{
    return static_cast<size_t>(std::ceil(m_fps * m_duration));
}

float Animation::CalculateFrameInterval() const
{
    if (m_fps <= 0)
    {
        return 0.0f;
    }
    return 1.0f / static_cast<float>(m_fps);
}

std::shared_ptr<AnimationFrame> Animation::GetFrameAtTime(float time)
{
    if (m_frames.empty() || m_duration <= 0.0f)
    {
        return nullptr;
    }

    // Clamp time between 0 and duration
    float clampedTime = time;
    if (clampedTime < 0.0f)
    {
        clampedTime = 0.0f;
    }
    if (clampedTime > m_duration)
    {
        clampedTime = m_duration;
    }

    // Calculate which frame corresponds to this time
    float frameProgress = clampedTime / m_duration;
    size_t frameIndex = static_cast<size_t>(frameProgress * static_cast<float>(m_frames.size()));

    if (frameIndex >= m_frames.size())
    {
        frameIndex = m_frames.size() - 1;
    }

    return m_frames[frameIndex];
}
