#include "VideoExporter.h"
#include "From-GDGRAP2/Debug.h"
#include <filesystem>
#include <algorithm>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

bool VideoExporter::CollectFramePaths(
    const std::string& frameDirectory,
    std::vector<std::string>& outFramePaths)
{
    try
    {
        if (!std::filesystem::exists(frameDirectory))
        {
            Debug::Log("[VideoExporter] Frames directory not found: " + frameDirectory);
            return false;
        }

        for (const auto& entry : std::filesystem::directory_iterator(frameDirectory))
        {
            if (entry.path().extension() == ".png")
            {
                outFramePaths.push_back(entry.path().string());
            }
        }

        if (outFramePaths.empty())
        {
            Debug::Log("[VideoExporter] No PNG frames found in: " + frameDirectory);
            return false;
        }

        // Sort to ensure correct order
        std::sort(outFramePaths.begin(), outFramePaths.end());
        return true;
    }
    catch (const std::exception& e)
    {
        Debug::Log("[VideoExporter] Error collecting frame paths: " + std::string(e.what()));
        return false;
    }
}

bool VideoExporter::ExportFramesToVideo(
    const std::string& frameDirectory,
    const std::string& outputVideoPath,
    int fps,
    int targetWidth,
    int targetHeight)
{
    try
    {
        // Collect frame paths
        std::vector<std::string> framePaths;
        if (!CollectFramePaths(frameDirectory, framePaths))
        {
            return false;
        }

        Debug::Log("[VideoExporter] Found " + std::to_string(framePaths.size()) + " frames for video export");

        // Find the codec
        const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
        if (!codec)
        {
            Debug::Log("[VideoExporter] libx264 codec not found - make sure FFmpeg is installed with libx264");
            return false;
        }

        // Create output format context
        AVFormatContext* formatContext = nullptr;
        avformat_alloc_output_context2(&formatContext, nullptr, "mp4", outputVideoPath.c_str());
        if (!formatContext)
        {
            Debug::Log("[VideoExporter] Failed to allocate format context");
            return false;
        }

        // Allocate codec context
        AVCodecContext* codecContext = avcodec_alloc_context3(codec);
        if (!codecContext)
        {
            Debug::Log("[VideoExporter] Failed to allocate codec context");
            avformat_free_context(formatContext);
            return false;
        }

        // Make dimensions even (divisible by 2) as required for x264
        int width = targetWidth;
        int height = targetHeight;
        if (width % 2 != 0) width--;
        if (height % 2 != 0) height--;

        // Configure codec context
        codecContext->bit_rate = 4000000; // 4 Mbps
        codecContext->width = width;
        codecContext->height = height;
        codecContext->time_base = { 1, fps };
        codecContext->framerate = { fps, 1 };
        codecContext->gop_size = 10;
        codecContext->max_b_frames = 2;
        codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

        // Create video stream
        AVStream* stream = avformat_new_stream(formatContext, codec);
        if (!stream)
        {
            Debug::Log("[VideoExporter] Failed to create stream");
            avformat_free_context(formatContext);
            avcodec_free_context(&codecContext);
            return false;
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
            Debug::Log("[VideoExporter] Failed to open codec");
            av_dict_free(&opts);
            avformat_free_context(formatContext);
            avcodec_free_context(&codecContext);
            return false;
        }
        av_dict_free(&opts);

        // Open output file for writing
        if (!(formatContext->oformat->flags & AVFMT_NOFILE))
        {
            if (avio_open(&formatContext->pb, outputVideoPath.c_str(), AVIO_FLAG_WRITE) < 0)
            {
                Debug::Log("[VideoExporter] Failed to open output file: " + outputVideoPath);
                avformat_free_context(formatContext);
                avcodec_free_context(&codecContext);
                return false;
            }
        }

        // Write format header
        if (avformat_write_header(formatContext, nullptr) < 0)
        {
            Debug::Log("[VideoExporter] Failed to write header");
            if (!(formatContext->oformat->flags & AVFMT_NOFILE))
                avio_closep(&formatContext->pb);
            avformat_free_context(formatContext);
            avcodec_free_context(&codecContext);
            return false;
        }

        // Allocate output frame
        AVFrame* frame = av_frame_alloc();
        if (!frame)
        {
            Debug::Log("[VideoExporter] Failed to allocate frame");
            av_write_trailer(formatContext);
            if (!(formatContext->oformat->flags & AVFMT_NOFILE))
                avio_closep(&formatContext->pb);
            avformat_free_context(formatContext);
            avcodec_free_context(&codecContext);
            return false;
        }

        frame->format = codecContext->pix_fmt;
        frame->width = codecContext->width;
        frame->height = codecContext->height;

        if (av_frame_get_buffer(frame, 32) < 0)
        {
            Debug::Log("[VideoExporter] Failed to allocate frame buffer");
            av_frame_free(&frame);
            av_write_trailer(formatContext);
            if (!(formatContext->oformat->flags & AVFMT_NOFILE))
                avio_closep(&formatContext->pb);
            avformat_free_context(formatContext);
            avcodec_free_context(&codecContext);
            return false;
        }

        Debug::Log("[VideoExporter] Starting video encoding with " + std::to_string(framePaths.size()) + 
                   " frames at " + std::to_string(fps) + " FPS (dimensions: " + 
                   std::to_string(width) + "x" + std::to_string(height) + ")");

        // Create scaler for converting RGB to YUV420P
        SwsContext* swsContext = nullptr;

        // Process each frame
        int frameIndex = 0;
        for (const auto& framePath : framePaths)
        {
            // Log the filename being processed
            auto fileName = std::filesystem::path(framePath).filename().string();
            Debug::Log("[VideoExporter] Processing frame: " + fileName);

            // Open the PNG file with FFmpeg
            AVFormatContext* inputFormatContext = nullptr;
            if (avformat_open_input(&inputFormatContext, framePath.c_str(), nullptr, nullptr) < 0)
            {
                Debug::Log("[VideoExporter] Error opening PNG file: " + framePath);
                continue;
            }

            if (avformat_find_stream_info(inputFormatContext, nullptr) < 0)
            {
                Debug::Log("[VideoExporter] Error finding stream info for: " + framePath);
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
                Debug::Log("[VideoExporter] No video stream found in: " + framePath);
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
                Debug::Log("[VideoExporter] Error opening codec for: " + framePath);
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
                Debug::Log("[VideoExporter] Failed to decode PNG file: " + framePath);
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
                    Debug::Log("[VideoExporter] Error initializing image scaler");
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
                Debug::Log("[VideoExporter] Error making output frame writable");
                av_frame_free(&inputFrame);
                av_packet_free(&readPacket);
                avcodec_free_context(&inputCodecContext);
                avformat_close_input(&inputFormatContext);
                break;
            }

            sws_scale(swsContext,
                      inputFrame->data, inputFrame->linesize, 0, inputFrame->height,
                      frame->data, frame->linesize);

            frame->pts = frameIndex;

            // Send frame to encoder
            if (avcodec_send_frame(codecContext, frame) < 0)
            {
                Debug::Log("[VideoExporter] Error sending frame to encoder");
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
                Debug::Log("[VideoExporter] Failed to allocate packet");
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
                Debug::Log("[VideoExporter] Encoded " + std::to_string(frameIndex) + " / " + std::to_string(framePaths.size()) + " frames");
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

        Debug::Log("[VideoExporter] Video export completed successfully: " + outputVideoPath);
        Debug::Log("[VideoExporter] Video contains " + std::to_string(framePaths.size()) + " frames");
        return true;
    }
    catch (const std::exception& e)
    {
        Debug::Log("[VideoExporter] Error exporting video: " + std::string(e.what()));
        return false;
    }
}
