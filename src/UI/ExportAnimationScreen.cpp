#include "ExportAnimationScreen.h"
#include "UIManager.h"
#include <cmath>

ExportAnimationScreen::ExportAnimationScreen() 
    : AUIScreen(UINames::EXPORT_ANIMATION_SCREEN)
    , m_animation(nullptr)
    , m_camera(nullptr)
    , m_currentFrameIndex(0)
    , m_fpsInput(30)
{
}

ExportAnimationScreen::~ExportAnimationScreen()
{
}

void ExportAnimationScreen::SetAnimation(std::shared_ptr<Animation> animation)
{
    m_animation = animation;
    m_currentFrameIndex = 0;
    m_lastAppliedKeyFrame = nullptr;
}

void ExportAnimationScreen::SetCamera(Camera* camera)
{
    m_camera = camera;
}

void ExportAnimationScreen::RefreshAnimationFrames()
{
    if (m_animation && m_camera)
    {
        m_animation->SetFPS(m_fpsInput);
        m_animation->ClearFrames();
        m_animation->GenerateFrames(m_camera);
        m_currentFrameIndex = 0;
        m_lastAppliedKeyFrame = nullptr;
    }
}

void ExportAnimationScreen::drawUI()
{
    ImGui::Begin("Export Animation", 0, UISettings::GlobalWindowFlags);
    ImGui::SetWindowSize(ImVec2(400, 300));

    ImGui::Text("Animation Export Settings");
    ImGui::Separator();

    // FPS input
    ImGui::InputInt("FPS##export_fps", &m_fpsInput, 1, 10);
    if (m_fpsInput < 1) m_fpsInput = 1;
    if (m_fpsInput > 240) m_fpsInput = 240;

    ImGui::Spacing();

    // Duration display (from camera)
    float duration = m_camera ? m_camera->getDuration() : 1.0f;
    ImGui::Text("Duration: %.2f seconds", duration);

    // Calculate and display frame count
    size_t frameCount = static_cast<size_t>(std::ceil(m_fpsInput * duration));
    ImGui::Text("Frames to Generate: %zu", frameCount);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Refresh button
    if (ImGui::Button("Refresh Animation Frames", ImVec2(-1, 0)))
    {
        RefreshAnimationFrames();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Frame navigation controls
    if (m_animation && m_animation->GetFrameCount() > 0)
    {
        ImGui::Text("Current Frame: %zu / %zu", m_currentFrameIndex + 1, m_animation->GetFrameCount());

        ImGui::Spacing();

        bool hasFrames = m_animation->GetFrameCount() > 0;

        // Previous button
        if (ImGui::Button("Previous Frame", ImVec2(150, 0)))
        {
            if (m_currentFrameIndex > 0)
            {
                m_currentFrameIndex--;
                auto frame = m_animation->GetFrame(m_currentFrameIndex);

				int startFrameIndex = static_cast<int>(frame->GetStartFrameIndex());
				int endFrameIndex = static_cast<int>(frame->GetEndFrameIndex());
				float delta = frame->GetDelta();

				auto interpolatedFrame = m_camera->InterpolateFrames(startFrameIndex, endFrameIndex, delta);

                if (frame && m_camera)
                {
                    auto interpolatedFrame = m_camera->InterpolateFrames(startFrameIndex, endFrameIndex, delta);
                    //Breaks the ui currently
                    //EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
                }
            }
        }

        ImGui::SameLine();

        // Next button
        if (ImGui::Button("Next Frame", ImVec2(150, 0)))
        {
            if (m_currentFrameIndex < m_animation->GetFrameCount() - 1)
            {
                m_currentFrameIndex++;
                auto frame = m_animation->GetFrame(m_currentFrameIndex);

                int startFrameIndex = static_cast<int>(frame->GetStartFrameIndex());
                int endFrameIndex = static_cast<int>(frame->GetEndFrameIndex());
                float delta = frame->GetDelta();

                auto interpolatedFrame = m_camera->InterpolateFrames(startFrameIndex, endFrameIndex, delta);

                if (frame && m_camera)
                {
                    auto interpolatedFrame = m_camera->InterpolateFrames(startFrameIndex, endFrameIndex, delta);
                    //EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
                }
            }
        }

        ImGui::Spacing();

        // Display current frame info
        auto currentFrame = m_animation->GetFrame(m_currentFrameIndex);
        if (currentFrame)
        {
            const auto& keyFrame = currentFrame->GetKeyFrame("camera");
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", keyFrame.position.x, keyFrame.position.y, keyFrame.position.z);

            ImGui::Text("Rendering State: %s", 
                currentFrame->GetRenderingState() == AnimationFrame::IDLE ? "Idle" :
                currentFrame->GetRenderingState() == AnimationFrame::RENDERING ? "Rendering" :
                currentFrame->GetRenderingState() == AnimationFrame::COMPLETED ? "Completed" : "Failed");
        }
    }
    else
    {
        ImGui::TextDisabled("No animation frames available");
        ImGui::TextDisabled("(Navigation buttons disabled)");
    }

    ImGui::End();
}
