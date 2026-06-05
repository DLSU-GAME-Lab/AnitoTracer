#include "ExportAnimationScreen.h"
#include "UIManager.h"
#include "From-GDGRAP2/EventNames.h"
#include "From-GDGRAP2/Debug.h"
#include "RayTracer.hpp"
#include "Utilities/FileUtils.h"
#include "Utilities/Video/VideoExporter.h"
#include "../Utilities/FileExplorer/FileExplorerConstants.h"
#include <cmath>
#include <filesystem>

ExportAnimationScreen::ExportAnimationScreen() 
    : AUIScreen(UINames::EXPORT_ANIMATION_SCREEN)
    , m_camera(nullptr)
    , m_currentFrameIndex(0)
    , m_fpsInput(30)
{
    // Subscribe to ray rendering events
    EventBroadcaster::getInstance()->addObserver(EventNames::RAYS_START_RENDER, this);
    EventBroadcaster::getInstance()->addObserver(EventNames::RAYS_END_RENDER, this);
    EventBroadcaster::getInstance()->addObserver(EventNames::ON_SAMPLE_PROGRESS, this);
}

ExportAnimationScreen::~ExportAnimationScreen()
{
    // Unsubscribe from ray rendering events
    EventBroadcaster::getInstance()->removeObserver(EventNames::RAYS_START_RENDER);
    EventBroadcaster::getInstance()->removeObserver(EventNames::RAYS_END_RENDER);
    EventBroadcaster::getInstance()->removeObserver(EventNames::ON_SAMPLE_PROGRESS);
}

void ExportAnimationScreen::SetCamera(Camera* camera)
{
    m_camera = camera;
}

void ExportAnimationScreen::RefreshAnimationFrames()
{
    if (m_camera)
    {
        auto animation = Animation::getInstance();
        animation->SetFPS(m_fpsInput);
        animation->ClearFrames();
        animation->GenerateFrames(m_camera);
        m_currentFrameIndex = 0;
        m_lastAppliedKeyFrame = nullptr;
    }
}

void ExportAnimationScreen::drawUI()
{
    // Process delayed frame capture if batch export is active
    ProcessDelayedFrameCapture();

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

    // Target sample percentage input (convert to int for ImGui slider)
    int targetPercentage = static_cast<int>(m_batchExportTargetPercentage);
    if (ImGui::SliderInt("Target Sample Percentage##export_percentage", &targetPercentage, 1, 100, "%d%%"))
    {
        m_batchExportTargetPercentage = static_cast<uint32_t>(targetPercentage);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Wait for sample percentage to reach this value\nbefore capturing frame (1-100%%)");
    }

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
    auto animation = Animation::getInstance();
    if (animation && animation->GetFrameCount() > 0)
    {
        ImGui::Text("Current Frame: %zu / %zu", m_currentFrameIndex + 1, animation->GetFrameCount());

        ImGui::Spacing();

        bool hasFrames = animation->GetFrameCount() > 0;

		// Previous button
		if (ImGui::Button("Previous Frame", ImVec2(150, 0)))
		{
			if (m_currentFrameIndex > 0)
			{
				m_currentFrameIndex--;
				auto frame = animation->GetFrame(m_currentFrameIndex);

				int startFrameIndex = static_cast<int>(frame->GetStartFrameIndex());
				int endFrameIndex = static_cast<int>(frame->GetEndFrameIndex());
				float delta = frame->GetDelta();

				auto interpolatedFrame = m_camera->InterpolateFrames(startFrameIndex, endFrameIndex, delta);

				if (frame && m_camera)
				{
					auto interpolatedFrame = m_camera->InterpolateFrames(startFrameIndex, endFrameIndex, delta);
					EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
				}
			}
		}

		ImGui::SameLine();

		// Next button
		if (ImGui::Button("Next Frame", ImVec2(150, 0)))
		{
			if (m_currentFrameIndex < animation->GetFrameCount() - 1)
			{
				m_currentFrameIndex++;
				auto frame = animation->GetFrame(m_currentFrameIndex);

				int startFrameIndex = static_cast<int>(frame->GetStartFrameIndex());
				int endFrameIndex = static_cast<int>(frame->GetEndFrameIndex());
				float delta = frame->GetDelta();

				auto interpolatedFrame = m_camera->InterpolateFrames(startFrameIndex, endFrameIndex, delta);

				if (frame && m_camera)
				{
					auto interpolatedFrame = m_camera->InterpolateFrames(startFrameIndex, endFrameIndex, delta);
					EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
				}
			}
		}

		ImGui::Spacing();

		// Display current frame info
		auto currentFrame = animation->GetFrame(m_currentFrameIndex);
        if (currentFrame)
        {
            const auto& keyFrame = currentFrame->GetKeyFrame("camera");
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", keyFrame.position.x, keyFrame.position.y, keyFrame.position.z);

            ImGui::Text("Rendering State: %s", 
                currentFrame->GetRenderingState() == AnimationFrame::IDLE ? "Idle" :
                currentFrame->GetRenderingState() == AnimationFrame::RENDERING ? "Rendering" :
                currentFrame->GetRenderingState() == AnimationFrame::COMPLETED ? "Completed" : "Failed");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Export current frame button
            if (ImGui::Button("Export Current Frame as PNG", ImVec2(-1, 0)))
            {
                ExportCurrentFrameAsPNG();
            }

            ImGui::Spacing();

            // Export all frames button
            if (ImGui::Button("Export All Frames as PNG", ImVec2(-1, 0)))
            {
                ExportAllFramesAsPNG();
            }

            ImGui::Spacing();

            // Export video button
            if (ImGui::Button("Export Video from Frames", ImVec2(-1, 0)))
            {
                ExportVideoFromFrames();
            }
        }
    }
    else
    {
        ImGui::TextDisabled("No animation frames available");
        ImGui::TextDisabled("(Navigation buttons disabled)");
    }

    ImGui::End();

    // Export progress overlay modal
    if (m_isBatchExporting)
    {
        ImGui::OpenPopup("Exporting Frames");
    }

    // Center the modal on screen
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Exporting Frames", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::Spacing();
        ImGui::Spacing();

        // Title
        ImGui::TextWrapped("Exporting animation frames as PNG images...");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Frame progress information
        float frameProgress = (float)m_batchExportCurrentFrame / (float)m_batchExportTotalFrames;
        char frameProgressText[256];
        std::snprintf(frameProgressText, sizeof(frameProgressText), "Frame %zu / %zu", m_batchExportCurrentFrame, m_batchExportTotalFrames);

        ImGui::Text("%s", frameProgressText);
        ImGui::ProgressBar(frameProgress, ImVec2(-1, 0));

        ImGui::Spacing();

        // Sample rendering progress (current frame)
        float sampleProgress = (float)m_currentSamplePercentage / 100.0f;
        char sampleProgressText[256];
        std::snprintf(sampleProgressText, sizeof(sampleProgressText), "Rendering: %d%% (%d / %d samples)", 
            m_currentSamplePercentage, m_currentSampleCount, m_maxSampleCount);

        ImGui::Text("Current Frame Progress:");
        ImGui::ProgressBar(sampleProgress, ImVec2(-1, 0), sampleProgressText);

        ImGui::Spacing();
        ImGui::Spacing();

        // Status text
        if (!m_batchExportReadyForCapture)
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Waiting for render event...");
        }
        else if (m_currentSamplePercentage < m_batchExportTargetPercentage)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Waiting for %d%% samples...", m_batchExportTargetPercentage);
        }
        else
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Frame complete, moving to next...");
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Cancel button
        if (ImGui::Button("Cancel Export", ImVec2(-1, 0)))
        {
            m_isBatchExporting = false;
            m_currentFrameIndex = m_batchExportOriginalFrameIndex;
            m_batchExportRayTracer = nullptr;
            m_batchExportReadyForCapture = false;
            m_currentSamplePercentage = 0;
            m_currentSampleCount = 0;
            m_maxSampleCount = 0;
            Debug::Log("[ExportAnimationScreen] Export cancelled by user");
            ImGui::CloseCurrentPopup();
        }

        // Auto-close when export is complete
        if (!m_isBatchExporting)
        {
            // Reset sample progress when export completes
            m_currentSamplePercentage = 0;
            m_currentSampleCount = 0;
            m_maxSampleCount = 0;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void ExportAnimationScreen::onTriggeredEvent(std::string eventName, std::shared_ptr<Parameters> parameters)
{
    if (eventName == EventNames::RAYS_START_RENDER)
    {
        Debug::Log("[ExportAnimationScreen] RAYS_START_RENDER event triggered");
		//Reset sample progress tracking for new frame render
        m_currentSampleCount = 0;
        m_currentSamplePercentage = 0;
    }
    else if (eventName == EventNames::RAYS_END_RENDER)
    {
        Debug::Log("[ExportAnimationScreen] RAYS_END_RENDER event triggered");

        // Handle batch export frame capture
        if (m_isBatchExporting && m_batchExportRayTracer)
        {
            // Mark that we're ready to check for target percentage
            m_batchExportReadyForCapture = true;
        }
    }
    else if (eventName == EventNames::ON_SAMPLE_PROGRESS)
    {
        // Extract sample progress data
        int percentage = parameters->getIntData("percentage", 0);
        int currentSamples = parameters->getIntData("currentSamples", 0);
        int maxSamples = parameters->getIntData("maxSamples", 1);
        bool isComplete = parameters->getBoolData("isComplete", false);

        // Store progress data for UI display
        m_currentSamplePercentage = percentage;
        m_currentSampleCount = currentSamples;
        m_maxSampleCount = maxSamples;

        /*
        // Print progress
        Debug::Log("[ExportAnimationScreen] Sample Progress: " + std::to_string(percentage) + "% (" 
            + std::to_string(currentSamples) + "/" + std::to_string(maxSamples) + ")");
        */

        // If batch exporting, also show which frame we're rendering
        if (m_isBatchExporting)
        {
            Debug::Log("[ExportAnimationScreen] Frame " + std::to_string(m_batchExportCurrentFrame + 1) 
                + "/" + std::to_string(m_batchExportTotalFrames) + " - Rendering: " + std::to_string(percentage) + "%");
        }

        // Log when rendering is complete
        if (isComplete)
        {
            Debug::Log("[ExportAnimationScreen] Frame rendering complete at 100%");
        }
    }
}


void ExportAnimationScreen::ExportCurrentFrameAsPNG()
{
    auto animation = Animation::getInstance();
    if (!animation || m_currentFrameIndex >= animation->GetFrameCount())
    {
        Debug::Log("[ExportAnimationScreen] Cannot export frame: invalid animation or frame index");
        return;
    }

    try
    {
        // Create filename with frame number (zero-padded to 6 digits)
        char frameFilename[256];
        std::snprintf(frameFilename, sizeof(frameFilename), "Frames/Frame_%06zu", m_currentFrameIndex);

        // Get the RayTracer instance and take screenshot
        auto rayTracer = RayTracer::getInstance();
        if (!rayTracer)
        {
            Debug::Log("[ExportAnimationScreen] Failed to get RayTracer instance");
            return;
        }

        rayTracer->TakeScreenshot(frameFilename);

        Debug::Log("[ExportAnimationScreen] Frame " + std::to_string(m_currentFrameIndex) + " exported successfully");
    }
    catch (const std::exception& e)
    {
        Debug::Log("[ExportAnimationScreen] Error exporting frame: " + std::string(e.what()));
    }
}

void ExportAnimationScreen::ProcessDelayedFrameCapture()
{
    // Only process if batch export is active and we have a valid raytracer
    if (!m_isBatchExporting || !m_batchExportRayTracer)
    {
        return;
    }

    // Check if we're ready for capture and have reached target percentage
    if (!m_batchExportReadyForCapture || m_currentSamplePercentage < m_batchExportTargetPercentage)
    {
        // Not ready or haven't reached target percentage yet
        return;
    }

    // We've reached the target percentage, proceed with frame capture
    try
    {
        // Create filename with frame number (zero-padded to 6 digits)
        char frameFilename[256];
        std::snprintf(frameFilename, sizeof(frameFilename), "Frames/Frame_%06zu", m_batchExportCurrentFrame);

        // Take screenshot
        m_batchExportRayTracer->TakeScreenshot(frameFilename);

        Debug::Log("[ExportAnimationScreen] Exported frame " + std::to_string(m_batchExportCurrentFrame + 1) + " / " + std::to_string(m_batchExportTotalFrames));

        // Move to next frame
        m_batchExportCurrentFrame++;

        // Check if we've exported all frames
        if (m_batchExportCurrentFrame >= m_batchExportTotalFrames)
        {
            // Batch export complete
            m_isBatchExporting = false;
            m_currentFrameIndex = m_batchExportOriginalFrameIndex;
            m_batchExportRayTracer = nullptr;
            m_batchExportReadyForCapture = false;
            Debug::Log("[ExportAnimationScreen] Successfully exported all " + std::to_string(m_batchExportTotalFrames) + " frames");
        }
        else
        {
            // Prepare next frame for export
            m_currentFrameIndex = m_batchExportCurrentFrame;
            auto animation_ptr = Animation::getInstance();
            auto frame = animation_ptr->GetFrame(m_currentFrameIndex);

            if (frame && m_camera)
            {
                int startFrameIndex = static_cast<int>(frame->GetStartFrameIndex());
                int endFrameIndex = static_cast<int>(frame->GetEndFrameIndex());
                float delta = frame->GetDelta();

                auto interpolatedFrame = m_camera->InterpolateFrames(startFrameIndex, endFrameIndex, delta);

                // Mark scene as dirty to trigger rendering for next frame
                EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);

                // Reset ready flag for next frame
                m_batchExportReadyForCapture = false;
            }
        }
    }
    catch (const std::exception& e)
    {
        Debug::Log("[ExportAnimationScreen] Error exporting batch frame " + std::to_string(m_batchExportCurrentFrame) + ": " + std::string(e.what()));
        m_isBatchExporting = false;
        m_batchExportRayTracer = nullptr;
        m_batchExportReadyForCapture = false;
    }
}

void ExportAnimationScreen::PrepareFramesFolder()
{
    try
    {
        //auto fullPath = std::string(FileExplorerConstants::ASSETS_DIR) + "/" + filename;
        //auto assetsPath = FileUtils::getExecutablePath() / std::string(FileExplorerConstants::ASSETS_DIR);
        auto framesPath = FileUtils::getProjectFolderPath().string() + "/Frames";

		Debug::Log("[ExportAnimationScreen] Preparing Frames folder at: " + framesPath);

        // If the Frames folder exists, delete it
        if (std::filesystem::exists(framesPath))
        {
            Debug::Log("[ExportAnimationScreen] Frames folder exists, deleting...");
            std::filesystem::remove_all(framesPath);
            Debug::Log("[ExportAnimationScreen] Frames folder deleted");
        }

        // Create a fresh Frames folder
        std::filesystem::create_directories(framesPath);
        Debug::Log("[ExportAnimationScreen] Frames folder created at: " + framesPath);
    }
    catch (const std::exception& e)
    {
        Debug::Log("[ExportAnimationScreen] Error preparing Frames folder: " + std::string(e.what()));
    }
}

void ExportAnimationScreen::ExportAllFramesAsPNG()
{
    auto animation = Animation::getInstance();
    if (!animation || animation->GetFrameCount() == 0)
    {
        Debug::Log("[ExportAnimationScreen] Cannot export frames: invalid animation or no frames available");
        return;
    }

    auto rayTracer = RayTracer::getInstance();
    if (!rayTracer)
    {
        Debug::Log("[ExportAnimationScreen] Failed to get RayTracer instance");
        return;
    }

    try
    {
        // Prepare the Frames folder (delete and recreate)
        PrepareFramesFolder();

        // Initialize batch export state
        m_batchExportTotalFrames = animation->GetFrameCount();
        m_batchExportCurrentFrame = 0;
        m_batchExportOriginalFrameIndex = m_currentFrameIndex;
        m_batchExportRayTracer = rayTracer;
        m_batchExportReadyForCapture = false;
        m_isBatchExporting = true;

        Debug::Log("[ExportAnimationScreen] Starting event-driven export of " + std::to_string(m_batchExportTotalFrames) + " frames...");
        Debug::Log("[ExportAnimationScreen] Frames will be captured on RAYS_END_RENDER events");

        // Set to first frame
        m_currentFrameIndex = 0;
        auto frame = animation->GetFrame(m_currentFrameIndex);

        if (frame && m_camera)
        {
            int startFrameIndex = static_cast<int>(frame->GetStartFrameIndex());
            int endFrameIndex = static_cast<int>(frame->GetEndFrameIndex());
            float delta = frame->GetDelta();

            auto interpolatedFrame = m_camera->InterpolateFrames(startFrameIndex, endFrameIndex, delta);

            // Mark scene as dirty to trigger rendering for first frame
            EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
        }
    }
    catch (const std::exception& e)
    {
        Debug::Log("[ExportAnimationScreen] Error initiating batch export: " + std::string(e.what()));
        m_isBatchExporting = false;
        m_batchExportRayTracer = nullptr;
    }
}

void ExportAnimationScreen::ExportVideoFromFrames()
{
    auto animation = Animation::getInstance();
    if (!animation || animation->GetFrameCount() == 0)
    {
        Debug::Log("[ExportAnimationScreen] Cannot export video: invalid animation or no frames available");
        return;
    }

    try
    {
        // Create video output path
        auto framesPath = FileUtils::getProjectFolderPath().string() + "/Frames";
        auto videoPath = FileUtils::getProjectFolderPath().string() + "/animation.mp4";

        Debug::Log("[ExportAnimationScreen] Starting video export from: " + framesPath);
        Debug::Log("[ExportAnimationScreen] Output video: " + videoPath);

        // Use VideoExporter utility to export frames to video
        if (VideoExporter::ExportFramesToVideo(framesPath, videoPath, animation->GetFPS(), 1920, 1080))
        {
            Debug::Log("[ExportAnimationScreen] Video export completed successfully");
        }
        else
        {
            Debug::Log("[ExportAnimationScreen] Video export failed");
        }
    }
    catch (const std::exception& e)
    {
        Debug::Log("[ExportAnimationScreen] Error exporting video: " + std::string(e.what()));
    }
}

