#pragma once

#include "AUIScreen.h"
#include "Engine/AnimationSystem/Animation.h"
#include "Engine/CameraSystem/Camera.h"
#include "From-GDGRAP2/EventBroadcaster.h"
#include <memory>

class ExportAnimationScreen : public AUIScreen
{
public:
    ExportAnimationScreen();
    ~ExportAnimationScreen();

    void SetAnimation(std::shared_ptr<Animation> animation);
    void SetCamera(Camera* camera);
    void RefreshAnimationFrames();

private:
    virtual void drawUI() override;
    friend class UIManager;

    std::shared_ptr<Animation> m_animation;
    Camera* m_camera = nullptr;
    int m_currentFrameIndex = 0;
    int m_fpsInput = 30;
    KeyFrame* m_lastAppliedKeyFrame = nullptr;
};
