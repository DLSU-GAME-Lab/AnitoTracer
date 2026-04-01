#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include "Engine/CameraSystem/Camera.h"

class AnimationFrame
{
public:
    enum RenderingState { IDLE = 0, RENDERING, COMPLETED, FAILED };

    AnimationFrame();
    ~AnimationFrame() = default;

    void AddKeyFrame(const std::string& id, const KeyFrame& keyFrame);
    void RemoveKeyFrame(const std::string& id);
    void ClearKeyFrames();

    const KeyFrame& GetKeyFrame(const std::string& id) const;
    const std::map<std::string, KeyFrame>& GetAllKeyFrames() const;
    bool HasKeyFrame(const std::string& id) const;
    size_t GetKeyFrameCount() const;

    void SetOutputBuffer(const std::vector<uint8_t>& buffer);
    const std::vector<uint8_t>& GetOutputBuffer() const;
    std::vector<uint8_t>& GetOutputBuffer();

    void SetRenderingState(RenderingState state);
    RenderingState GetRenderingState() const;
    bool IsRendering() const;

    void ApplyKeyFrameToCamera(Camera* camera, const std::string& keyFrameId);
    void ApplyAllKeyFramesToCamera(Camera* camera);

private:
    std::map<std::string, KeyFrame> m_keyFrames;
    std::vector<uint8_t> m_outputBuffer;
    RenderingState m_renderingState = IDLE;
};
