#include "ExportAnimationScreen.h"
#include "UIManager.h"
#include "From-GDGRAP2/EventNames.h"
#include "From-GDGRAP2/Debug.h"
#include "RayTracer.hpp"
#include "Utilities/FileUtils.h"
#include "../Utilities/FileExplorer/FileExplorerConstants.h"
#include <cmath>
#include <filesystem>
#include <chrono>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

ExportAnimationScreen::ExportAnimationScreen() 
    : AUIScreen(UINames::EXPORT_ANIMATION_SCREEN)
    , m_camera(nullptr)
    , m_currentFrameIndex(0)
    , m_fpsInput(30)
{
    // Subscribe to ray rendering events
    EventBroadcaster::getInstance()->addObserver(EventNames::RAYS_START_RENDER, this);
    EventBroadcaster::getInstance()->addObserver(EventNames::RAYS_END_RENDER, this);
}

ExportAnimationScreen::~ExportAnimationScreen()
{
    // Unsubscribe from ray rendering events
    EventBroadcaster::getInstance()->removeObserver(EventNames::RAYS_START_RENDER);
    EventBroadcaster::getInstance()->removeObserver(EventNames::RAYS_END_RENDER);
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

    // Frame capture delay input (convert to float for ImGui slider)
    float delayFloat = static_cast<float>(m_batchExportFrameDelay);
    if (ImGui::SliderFloat("Capture Delay (seconds)##export_delay", &delayFloat, 0.0f, 60.0f, "%.3f"))
    {
        m_batchExportFrameDelay = delayFloat;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Delay before capturing frame after RAYS_END_RENDER\n(allows additional render time if needed)");
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
}

void ExportAnimationScreen::onTriggeredEvent(std::string eventName, std::shared_ptr<Parameters> parameters)
{
    if (eventName == EventNames::RAYS_START_RENDER)
    {
        Debug::Log("[ExportAnimationScreen] RAYS_START_RENDER event triggered");
    }
    else if (eventName == EventNames::RAYS_END_RENDER)
    {
        Debug::Log("[ExportAnimationScreen] RAYS_END_RENDER event triggered");

        // Handle batch export frame capture
        if (m_isBatchExporting && m_batchExportRayTracer)
        {
            // Record the time of RAYS_END_RENDER event
            m_batchExportLastEventTime = std::chrono::duration<double>(
                std::chrono::high_resolution_clock::now().time_since_epoch()
            ).count();
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
    // Only process if batch export is active and we have a valid time
    if (!m_isBatchExporting || !m_batchExportRayTracer || m_batchExportLastEventTime == 0.0)
    {
        return;
    }

    // Get current time
    double currentTime = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();

    // Check if enough time has elapsed
    double elapsedTime = currentTime - m_batchExportLastEventTime;

    if (elapsedTime < m_batchExportFrameDelay)
    {
        // Not enough time has passed yet, return and try again next frame
        return;
    }

    // Delay has elapsed, proceed with frame capture
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
            m_batchExportLastEventTime = 0.0;
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

                // Reset event time for next frame
                m_batchExportLastEventTime = 0.0;
            }
        }
    }
    catch (const std::exception& e)
    {
        Debug::Log("[ExportAnimationScreen] Error exporting batch frame " + std::to_string(m_batchExportCurrentFrame) + ": " + std::string(e.what()));
        m_isBatchExporting = false;
        m_batchExportRayTracer = nullptr;
        m_batchExportLastEventTime = 0.0;
    }
}

void ExportAnimationScreen::PrepareFramesFolder()
{
    try
    {
        //auto fullPath = std::string(FileExplorerConstants::ASSETS_DIR) + "/" + filename;
        //auto assetsPath = FileUtils::getExecutablePath() / std::string(FileExplorerConstants::ASSETS_DIR);
        auto framesPath = std::string(FileExplorerConstants::ASSETS_DIR) + "/Frames";

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
        auto framesPath = std::string(FileExplorerConstants::ASSETS_DIR) + "/Frames";
        auto videoPath = std::string(FileExplorerConstants::ASSETS_DIR) + "/animation.mp4";

        // Check if frames folder exists
        if (!std::filesystem::exists(framesPath))
        {
            Debug::Log("[ExportAnimationScreen] Frames folder not found at: " + framesPath);
            return;
        }

        // Collect frame paths and sort them
        std::vector<std::string> framePaths;
        for (const auto& entry : std::filesystem::directory_iterator(framesPath))
        {
            if (entry.path().extension() == ".png")
            {
                framePaths.push_back(entry.path().string());
            }
        }

        if (framePaths.empty())
        {
            Debug::Log("[ExportAnimationScreen] No PNG frames found in: " + framesPath);
            return;
        }

        // Sort frame paths to ensure correct order
        std::sort(framePaths.begin(), framePaths.end());

        Debug::Log("[ExportAnimationScreen] Found " + std::to_string(framePaths.size()) + " frames for video export");

        // Find the codec
        const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
        if (!codec)
        {
            Debug::Log("[ExportAnimationScreen] libx264 codec not found - make sure FFmpeg is installed with libx264");
            return;
        }

        // Create output format context
        AVFormatContext* formatContext = nullptr;
        avformat_alloc_output_context2(&formatContext, nullptr, "mp4", videoPath.c_str());
        if (!formatContext)
        {
            Debug::Log("[ExportAnimationScreen] Failed to allocate format context");
            return;
        }

        // Allocate codec context
        AVCodecContext* codecContext = avcodec_alloc_context3(codec);
        if (!codecContext)
        {
            Debug::Log("[ExportAnimationScreen] Failed to allocate codec context");
            avformat_free_context(formatContext);
            return;
        }

        // Set default dimensions (1920x1080)
        // These will be overridden if we can read actual PNG dimensions
        int width = 1920;
        int height = 1080;

        // Make dimensions even (divisible by 2) as required for x264
        if (width % 2 != 0) width--;
        if (height % 2 != 0) height--;

        // Configure codec context
        codecContext->bit_rate = 4000000; // 4 Mbps
        codecContext->width = width;
        codecContext->height = height;
        codecContext->time_base = { 1, animation->GetFPS() };
        codecContext->framerate = { animation->GetFPS(), 1 };
        codecContext->gop_size = 10;
        codecContext->max_b_frames = 2;
        codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

        // Create video stream
        AVStream* stream = avformat_new_stream(formatContext, codec);
        if (!stream)
        {
            Debug::Log("[ExportAnimationScreen] Failed to create stream");
            avformat_free_context(formatContext);
            avcodec_free_context(&codecContext);
            return;
        }

        // Copy codec context to stream codec parameters
        avcodec_parameters_from_context(stream->codecpar, codecContext);
        stream->time_base = codecContext->time_base;

        // Open codec
        AVDictionary* opts = nullptr;
        av_dict_set(&opts, "preset", "medium", 0);
        av_dict_set(&opts, "crf", "23", 0); // Quality (lower is better, 0-51)

        if (avcodec_open2(codecContext, codec, &opts) < 0)
        {
            Debug::Log("[ExportAnimationScreen] Failed to open codec");
            av_dict_free(&opts);
            avformat_free_context(formatContext);
            avcodec_free_context(&codecContext);
            return;
        }
        av_dict_free(&opts);

        // Open output file for writing
        if (!(formatContext->oformat->flags & AVFMT_NOFILE))
        {
            if (avio_open(&formatContext->pb, videoPath.c_str(), AVIO_FLAG_WRITE) < 0)
            {
                Debug::Log("[ExportAnimationScreen] Failed to open output file: " + videoPath);
                avformat_free_context(formatContext);
                avcodec_free_context(&codecContext);
                return;
            }
        }

        // Write format header
        if (avformat_write_header(formatContext, nullptr) < 0)
        {
            Debug::Log("[ExportAnimationScreen] Failed to write header");
            if (!(formatContext->oformat->flags & AVFMT_NOFILE))
                avio_closep(&formatContext->pb);
            avformat_free_context(formatContext);
            avcodec_free_context(&codecContext);
            return;
        }

        // Allocate frame
        AVFrame* frame = av_frame_alloc();
        if (!frame)
        {
            Debug::Log("[ExportAnimationScreen] Failed to allocate frame");
            av_write_trailer(formatContext);
            if (!(formatContext->oformat->flags & AVFMT_NOFILE))
                avio_closep(&formatContext->pb);
            avformat_free_context(formatContext);
            avcodec_free_context(&codecContext);
            return;
        }

        frame->format = codecContext->pix_fmt;
        frame->width = codecContext->width;
        frame->height = codecContext->height;

        if (av_frame_get_buffer(frame, 32) < 0)
        {
            Debug::Log("[ExportAnimationScreen] Failed to allocate frame buffer");
            av_frame_free(&frame);
            av_write_trailer(formatContext);
            if (!(formatContext->oformat->flags & AVFMT_NOFILE))
                avio_closep(&formatContext->pb);
            avformat_free_context(formatContext);
            avcodec_free_context(&codecContext);
            return;
        }

        Debug::Log("[ExportAnimationScreen] Starting video encoding with " + std::to_string(framePaths.size()) + 
                   " frames at " + std::to_string(animation->GetFPS()) + " FPS (dimensions: " + 
                   std::to_string(width) + "x" + std::to_string(height) + ")");

        // Create scaler for converting RGB to YUV420P
        SwsContext* swsContext = nullptr;

        // Process each frame
        int frameIndex = 0;
        for (const auto& framePath : framePaths)
        {
            // Log the filename being processed
            auto fileName = std::filesystem::path(framePath).filename().string();
            Debug::Log("[ExportAnimationScreen] Processing frame: " + fileName);

            // Open the PNG file with FFmpeg
            AVFormatContext* inputFormatContext = nullptr;
            if (avformat_open_input(&inputFormatContext, framePath.c_str(), nullptr, nullptr) < 0)
            {
                Debug::Log("[ExportAnimationScreen] Error opening PNG file: " + framePath);
                continue;
            }

            if (avformat_find_stream_info(inputFormatContext, nullptr) < 0)
            {
                Debug::Log("[ExportAnimationScreen] Error finding stream info for: " + framePath);
                avformat_close_input(&inputFormatContext);
                continue;
            }

            // Find video stream
            int videoStreamIndex = -1;
            for (unsigned int i = 0; i < inputFormatContext->nb_streams; i++)
            {
                if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
                {
                    videoStreamIndex = i;
                    break;
                }
            }

            if (videoStreamIndex == -1)
            {
                Debug::Log("[ExportAnimationScreen] No video stream found in: " + framePath);
                avformat_close_input(&inputFormatContext);
                continue;
            }

            // Get codec and context
            AVStream* inputStream = inputFormatContext->streams[videoStreamIndex];
            const AVCodec* inputCodec = avcodec_find_decoder(inputStream->codecpar->codec_id);
            AVCodecContext* inputCodecContext = avcodec_alloc_context3(inputCodec);
            avcodec_parameters_to_context(inputCodecContext, inputStream->codecpar);

            if (avcodec_open2(inputCodecContext, inputCodec, nullptr) < 0)
            {
                Debug::Log("[ExportAnimationScreen] Error opening codec for: " + framePath);
                avcodec_free_context(&inputCodecContext);
                avformat_close_input(&inputFormatContext);
                continue;
            }

            // Allocate frame for reading
            AVFrame* inputFrame = av_frame_alloc();
            AVPacket* readPacket = av_packet_alloc();

            bool frameDecoded = false;
            while (av_read_frame(inputFormatContext, readPacket) >= 0)
            {
                if (readPacket->stream_index == videoStreamIndex)
                {
                    avcodec_send_packet(inputCodecContext, readPacket);
                    if (avcodec_receive_frame(inputCodecContext, inputFrame) == 0)
                    {
                        frameDecoded = true;
                        break;
                    }
                }
                av_packet_unref(readPacket);
            }

            if (!frameDecoded)
            {
                Debug::Log("[ExportAnimationScreen] Failed to decode PNG file: " + framePath);
                av_frame_free(&inputFrame);
                av_packet_free(&readPacket);
                avcodec_free_context(&inputCodecContext);
                avformat_close_input(&inputFormatContext);
                continue;
            }

            // Initialize or reinitialize the scaler if dimensions changed
            if (!swsContext || inputFrame->width != width || inputFrame->height != height)
            {
                if (swsContext)
                    sws_freeContext(swsContext);

                swsContext = sws_getContext(
                    inputFrame->width, inputFrame->height, static_cast<AVPixelFormat>(inputFrame->format),
                    width, height, AV_PIX_FMT_YUV420P,
                    SWS_BILINEAR, nullptr, nullptr, nullptr
                );

                if (!swsContext)
                {
                    Debug::Log("[ExportAnimationScreen] Error initializing image scaler");
                    av_frame_free(&inputFrame);
                    av_packet_free(&readPacket);
                    avcodec_free_context(&inputCodecContext);
                    avformat_close_input(&inputFormatContext);
                    continue;
                }
            }

            // Convert and scale the frame to YUV420P
            if (av_frame_make_writable(frame) < 0)
            {
                Debug::Log("[ExportAnimationScreen] Error making output frame writable");
                break;
            }

            sws_scale(swsContext,
                      inputFrame->data, inputFrame->linesize, 0, inputFrame->height,
                      frame->data, frame->linesize);

            frame->pts = frameIndex;

            // Send frame to encoder
            if (avcodec_send_frame(codecContext, frame) < 0)
            {
                Debug::Log("[ExportAnimationScreen] Error sending frame to encoder");
                av_frame_free(&inputFrame);
                av_packet_free(&readPacket);
                avcodec_free_context(&inputCodecContext);
                avformat_close_input(&inputFormatContext);
                break;
            }

            // Receive and write encoded packets
            AVPacket* pkt = av_packet_alloc();
            if (!pkt)
            {
                Debug::Log("[ExportAnimationScreen] Failed to allocate packet");
                av_frame_free(&inputFrame);
                av_packet_free(&readPacket);
                avcodec_free_context(&inputCodecContext);
                avformat_close_input(&inputFormatContext);
                break;
            }

            while (avcodec_receive_packet(codecContext, pkt) == 0)
            {
                pkt->stream_index = 0;
                av_packet_rescale_ts(pkt, codecContext->time_base, stream->time_base);
                av_interleaved_write_frame(formatContext, pkt);
                av_packet_unref(pkt);
            }
            av_packet_free(&pkt);

            // Cleanup input frame resources
            av_frame_free(&inputFrame);
            av_packet_free(&readPacket);
            avcodec_free_context(&inputCodecContext);
            avformat_close_input(&inputFormatContext);

            frameIndex++;

            if (frameIndex % 10 == 0)
            {
                Debug::Log("[ExportAnimationScreen] Encoded " + std::to_string(frameIndex) + " / " + std::to_string(framePaths.size()) + " frames");
            }
        }

        // Cleanup scaler
        if (swsContext)
            sws_freeContext(swsContext);

        // Flush encoder
        AVPacket* pkt = av_packet_alloc();
        if (pkt)
        {
            avcodec_send_frame(codecContext, nullptr);
            while (avcodec_receive_packet(codecContext, pkt) == 0)
            {
                pkt->stream_index = 0;
                av_packet_rescale_ts(pkt, codecContext->time_base, stream->time_base);
                av_interleaved_write_frame(formatContext, pkt);
                av_packet_unref(pkt);
            }
            av_packet_free(&pkt);
        }

        // Write trailer
        av_write_trailer(formatContext);

        // Cleanup
        av_frame_free(&frame);

        if (!(formatContext->oformat->flags & AVFMT_NOFILE))
            avio_closep(&formatContext->pb);

        avformat_free_context(formatContext);
        avcodec_free_context(&codecContext);

        Debug::Log("[ExportAnimationScreen] Video export completed successfully: " + videoPath);
        Debug::Log("[ExportAnimationScreen] Video contains " + std::to_string(framePaths.size()) + " frames");
    }
    catch (const std::exception& e)
    {
        Debug::Log("[ExportAnimationScreen] Error exporting video: " + std::string(e.what()));
    }
}

