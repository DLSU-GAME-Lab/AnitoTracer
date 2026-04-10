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

    void SetCamera(Camera* camera);
    void RefreshAnimationFrames();

private:
    virtual void drawUI() override;
    virtual void onTriggeredEvent(std::string eventName, std::shared_ptr<Parameters> parameters = nullptr) override;
    void ExportCurrentFrameAsPNG();
    void ExportAllFramesAsPNG();
    void ExportVideoFromFrames();
    void PrepareFramesFolder();
    void ProcessDelayedFrameCapture();
    friend class UIManager;

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
    uint32_t m_batchExportTargetPercentage = 100;  // Target sample percentage to wait for before capturing frame
    bool m_batchExportReadyForCapture = false;  // Flag set when RAYS_END_RENDER is triggered

    // Sample progress tracking
    int m_currentSamplePercentage = 0;
    int m_currentSampleCount = 0;
    int m_maxSampleCount = 0;
};
