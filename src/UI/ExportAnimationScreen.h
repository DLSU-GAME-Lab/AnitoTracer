#pragma once

#include "AUIScreen.h"
#include "Engine/AnimationSystem/Animation.h"
#include "Engine/CameraSystem/Camera.h"
#include "From-GDGRAP2/EventBroadcaster.h"
#include <memory>

// Forward declaration
class RayTracer;

class ExportAnimationScreen : public AUIScreen, public Observer
{
public:
    ExportAnimationScreen();
    ~ExportAnimationScreen();

    void SetAnimation(std::shared_ptr<Animation> animation);
    void SetCamera(Camera* camera);
    void RefreshAnimationFrames();

private:
    virtual void drawUI() override;
    virtual void onTriggeredEvent(std::string eventName, std::shared_ptr<Parameters> parameters = nullptr) override;
    void ExportCurrentFrameAsPNG();
    void ExportAllFramesAsPNG();
    void PrepareFramesFolder();
    void ProcessDelayedFrameCapture();
    friend class UIManager;

    std::shared_ptr<Animation> m_animation;
    Camera* m_camera = nullptr;
    int m_currentFrameIndex = 0;
    int m_fpsInput = 30;
    KeyFrame* m_lastAppliedKeyFrame = nullptr;

    // Batch export state variables
    bool m_isBatchExporting = false;
    size_t m_batchExportCurrentFrame = 0;
    size_t m_batchExportTotalFrames = 0;
    int m_batchExportOriginalFrameIndex = 0;
    RayTracer* m_batchExportRayTracer = nullptr;
    double m_batchExportFrameDelay = 0.1;  // Delay in seconds before capturing frame (default 0.1s)
    double m_batchExportLastEventTime = 0.0;  // Timestamp of last RAYS_END_RENDER event
};
