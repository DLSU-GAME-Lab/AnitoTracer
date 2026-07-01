#include "AnimationFrame.h"

AnimationFrame::AnimationFrame()
    : m_keyFrames(), m_outputBuffer(), m_renderingState(IDLE)
{
}

void AnimationFrame::AddKeyFrame(const std::string& id, const KeyFrame& keyFrame, const float delta, const size_t startFrame, const size_t endFrame)
{
    m_keyFrames[id] = keyFrame;
    m_delta = delta;
	m_start_frame_index = startFrame;
	m_end_frame_index = endFrame;
}

void AnimationFrame::RemoveKeyFrame(const std::string& id)
{
    m_keyFrames.erase(id);
}

void AnimationFrame::ClearKeyFrames()
{
    m_keyFrames.clear();
}

const KeyFrame& AnimationFrame::GetKeyFrame(const std::string& id) const
{
    static const KeyFrame emptyKeyFrame{};
    auto it = m_keyFrames.find(id);
    if (it != m_keyFrames.end())
    {
        return it->second;
    }
    return emptyKeyFrame;
}

const std::map<std::string, KeyFrame>& AnimationFrame::GetAllKeyFrames() const
{
    return m_keyFrames;
}

bool AnimationFrame::HasKeyFrame(const std::string& id) const
{
    return m_keyFrames.find(id) != m_keyFrames.end();
}

size_t AnimationFrame::GetKeyFrameCount() const
{
    return m_keyFrames.size();
}

void AnimationFrame::SetOutputBuffer(const std::vector<uint8_t>& buffer)
{
    m_outputBuffer = buffer;
}

const std::vector<uint8_t>& AnimationFrame::GetOutputBuffer() const
{
    return m_outputBuffer;
}

std::vector<uint8_t>& AnimationFrame::GetOutputBuffer()
{
    return m_outputBuffer;
}

void AnimationFrame::SetRenderingState(RenderingState state)
{
    m_renderingState = state;
}

AnimationFrame::RenderingState AnimationFrame::GetRenderingState() const
{
    return m_renderingState;
}

bool AnimationFrame::IsRendering() const
{
    return m_renderingState == RENDERING;
}

void AnimationFrame::ApplyKeyFrameToCamera(Camera* camera, const std::string& keyFrameId)
{
    if (camera != nullptr && HasKeyFrame(keyFrameId))
    {
        const KeyFrame& keyFrame = GetKeyFrame(keyFrameId);
        camera->setToKeyFrame(const_cast<KeyFrame*>(&keyFrame));
    }
}

void AnimationFrame::ApplyAllKeyFramesToCamera(Camera* camera)
{
    if (camera != nullptr && !m_keyFrames.empty())
    {
        const KeyFrame& keyFrame = m_keyFrames.begin()->second;
        camera->setToKeyFrame(const_cast<KeyFrame*>(&keyFrame));
    }
}
