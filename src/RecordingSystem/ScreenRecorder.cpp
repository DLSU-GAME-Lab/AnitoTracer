#include "ScreenRecorder.hpp"
#include "Utilities/FileExplorer/FileExplorerConstants.h"
#include "Utilities/Screenshot.hpp" //has timestamp func
#include <From-GDGRAP2/Debug.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <filesystem>


static std::string AvErr(int err)
{
	char buf[AV_ERROR_MAX_STRING_SIZE];
	av_strerror(err, buf, sizeof(buf));
	return std::string(buf);
}

ScreenRecorder::ScreenRecorder()
{
	m_pathPrefix = std::string(FileExplorerConstants::ASSETS_DIR) + +"/recordings/";
	std::filesystem::create_directories(m_pathPrefix);
}

ScreenRecorder::~ScreenRecorder()
{
	Stop();
}

bool ScreenRecorder::Start(const std::string& fileName, const Config& config)
{
	Stop();

	if (config.width <= 0 || config.height <= 0 || config.targetFps <= 0.0) return false;

	m_config = config;
	
	std::string stem = fileName;
	std::string ext = ".mp4";

	auto dot = fileName.rfind('.');
	if (dot != std::string::npos)
	{
		stem = fileName.substr(0, dot);
		ext = fileName.substr(dot);
	}

	m_path = m_pathPrefix + stem + "_" + Export::MakeTimestamp() + ext;

	m_interval = std::chrono::duration<double>(1.0 / m_config.targetFps);

	m_hasNextAccept = false;
	m_frameIndex = 0;

	if (!InitPipeline()) 
	{
		Debug::Log("Failed to initialize libav for screen recording.");
		ResetPipeline();
		return false;
	}

	m_started = true;
	return true;
}

void ScreenRecorder::Stop()
{
	if (!m_started) return;

	FlushEncoder();
	if (m_container) av_write_trailer(m_container.get());

	ResetPipeline();
	m_started = false;
}

void ScreenRecorder::PushFrame(const uint8_t* src, size_t srcStrideBytes, TimePoint timePoint)
{
	if (!m_started || !src) return;
	if (!ShouldAccept(timePoint)) return;
	
	CopyIntoInputFrame(src, srcStrideBytes);	// Copy host buffer into input AVFrame
	EncodeAndMuxOne();							// Convert + encode + mux
}

bool ScreenRecorder::IsRecording() const
{
	return m_started;
}

bool ScreenRecorder::InitPipeline()
{
	// Create MP4 output context
	AVFormatContext* raw = nullptr;
	auto res = avformat_alloc_output_context2(&raw, nullptr, "mp4", m_path.c_str());

	if (res < 0)
	{
		Debug::Log("Failed to allocate output context: " + AvErr(res));
		return false;
	}

	m_container.reset(raw);

	if (!m_container)
	{
		Debug::Log("Output context allocation returned null");
		return false;
	}

	Debug::Log("Output context created (format= " + std::string(m_container->oformat->name) + ")");

	// Find encoder (h264_nvenc)
	const AVCodec* codec = avcodec_find_encoder_by_name("h264_nvenc");

	if (!codec)
	{
		Debug::Log("h264_nvenc encoder not found");
		return false;
	}

	Debug::Log("Encoder found: " +  std::string(codec->long_name));

	// Create stream 
	m_videoStream = avformat_new_stream(m_container.get(), nullptr);
	if (!m_videoStream)
	{
		Debug::Log("Failed to create video stream");
		return false;
	}

	// Create encoder context
	m_videoEncoder.reset(avcodec_alloc_context3(codec));

	if (!m_videoEncoder)
	{
		Debug::Log("Failed to allocate AVCodecContext");
		return false;
	}

	// Time base
	const int fpsInt = (int)std::llround(m_config.targetFps);

	if (fpsInt <= 0)
	{
		Debug::Log("Invalid targetFps after rounding: " + std::to_string(fpsInt));
		return false;
	}

	AVRational tb{ 1, fpsInt };

	m_videoEncoder->codec_type = AVMEDIA_TYPE_VIDEO;
	m_videoEncoder->codec_id = codec->id;
	m_videoEncoder->width = m_config.width & ~1;
	m_videoEncoder->height = m_config.height & ~1;
	m_videoEncoder->time_base = tb;
	m_videoEncoder->framerate = AVRational{ fpsInt, 1 };
	m_videoEncoder->pix_fmt = AV_PIX_FMT_NV12;
	m_videoEncoder->gop_size = m_config.gopSize;
	m_videoEncoder->max_b_frames = m_config.maxBFrames;


	if (m_container->oformat->flags & AVFMT_GLOBALHEADER)
	{
		m_videoEncoder->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		Debug::Log("Container requires GLOBAL_HEADER");
	}

	// Encoder options
	av_opt_set(m_videoEncoder->priv_data, "rc", "vbr", 0);
	av_opt_set_int(m_videoEncoder->priv_data, "cq", m_config.cq, 0);

	res = avcodec_open2(m_videoEncoder.get(), codec, nullptr);
	if (res < 0)
	{
		Debug::Log("avcodec_open2 failed: " + AvErr(res));
		return false;
	}

	// Stream parameters
	m_videoStream->time_base = m_videoEncoder->time_base;
	res = avcodec_parameters_from_context(m_videoStream->codecpar, m_videoEncoder.get());
	if (res < 0)
	{
		Debug::Log("avcodec_parameters_from_context failed: " + AvErr(res));
		return false;
	}

	// Open output file
	if (!(m_container->oformat->flags & AVFMT_NOFILE))
	{
		res = avio_open(&m_container->pb, m_path.c_str(), AVIO_FLAG_WRITE);
		if (res < 0)
		{
			Debug::Log("avio_open failed: " + AvErr(res));
			return false;
		}
	}

	// Muxer options
	AVDictionary* muxOpts = nullptr;
	if (m_config.fastStart)
	{
		av_dict_set(&muxOpts, "movflags", "+faststart", 0);
		Debug::Log("movflags=+faststart enabled");
	}

	res = avformat_write_header(m_container.get(), &muxOpts);
	if (res < 0)
	{
		Debug::Log("avformat_write_header failed: " + AvErr(res));
		av_dict_free(&muxOpts);
		return false;
	}

	av_dict_free(&muxOpts);

	// Allocate frames + packet
	m_inputFrame.reset(av_frame_alloc());
	m_encodeFrame.reset(av_frame_alloc());
	m_encodedPacket.reset(av_packet_alloc());

	if (!m_inputFrame || !m_encodeFrame || !m_encodedPacket)
	{
		Debug::Log("Failed to allocate AVFrame/AVPacket");
		return false;
	}

	const AVPixelFormat inFmt = AV_PIX_FMT_RGBA;

	m_inputFrame->format = inFmt;
	m_inputFrame->width = m_videoEncoder->width;
	m_inputFrame->height = m_videoEncoder->height;

	m_encodeFrame->format = m_videoEncoder->pix_fmt;
	m_encodeFrame->width = m_videoEncoder->width;
	m_encodeFrame->height = m_videoEncoder->height;

	res = av_frame_get_buffer(m_inputFrame.get(), 32);
	if (res < 0)
	{
		Debug::Log("av_frame_get_buffer(input) failed: " + AvErr(res));
		return false;
	}

	res = av_frame_get_buffer(m_encodeFrame.get(), 32);
	if (res < 0)
	{
		Debug::Log("av_frame_get_buffer(encode) failed: " + AvErr(res));
		return false;
	}

	// swscale
	m_colorConverter.reset(sws_getContext(
		m_videoEncoder->width, m_videoEncoder->height, inFmt,
		m_videoEncoder->width, m_videoEncoder->height, m_videoEncoder->pix_fmt,
		SWS_BILINEAR, nullptr, nullptr, nullptr
	));

	if (!m_colorConverter)
	{
		Debug::Log("sws_getContext failed (returned null)");
		return false;
	}

	return true;
}

void ScreenRecorder::ResetPipeline()
{
	m_colorConverter.reset();
	m_encodedPacket.reset();
	m_encodeFrame.reset();
	m_inputFrame.reset();
	m_videoEncoder.reset();
	m_videoStream = nullptr; 
	m_container.reset();
}

// Limiter to targetFps by timestamping incoming frames and dropping those that come in too soon. 
// This is necessary because the encoding pipeline can be too slow to keep up with the renderer's frame rate, 
// and we don't want a growing backlog of frames to encode.
bool ScreenRecorder::ShouldAccept(TimePoint timeStamp)
{
	if (!m_hasNextAccept) 
	{
		m_nextAccept = timeStamp; 
		m_hasNextAccept = true;
		return true;
	}

	if (timeStamp < m_nextAccept) return false;

	m_nextAccept += std::chrono::duration_cast<Clock::duration>(m_interval);
	const auto maxLag = std::chrono::duration_cast<Clock::duration>(m_interval * 2.0);

	if (timeStamp > m_nextAccept + maxLag) 
	{
		m_nextAccept = timeStamp + std::chrono::duration_cast<Clock::duration>(m_interval);
	}

	return true;
}

void ScreenRecorder::CopyIntoInputFrame(const uint8_t* src, size_t srcStrideBytes)
{
	const int bpp = 4; // bytes per pixel (RGBA)
	const size_t tightStride = size_t(m_config.width) * size_t(bpp);
	if (srcStrideBytes == 0) srcStrideBytes = tightStride;

	// Ensure writable
	av_frame_make_writable(m_inputFrame.get());

	if (!m_config.flipY) 
	{
		for (int y = 0; y < m_config.height; ++y)
		{
			std::memcpy(m_inputFrame->data[0] + y * m_inputFrame->linesize[0], src + size_t(y) * srcStrideBytes, tightStride);
		}
	}
	else {
		for (int y = 0; y < m_config.height; ++y) 
		{
			const int sy = (m_config.height - 1) - y;
			std::memcpy(m_inputFrame->data[0] + y * m_inputFrame->linesize[0], src + size_t(sy) * srcStrideBytes, tightStride);
		}
	}
}

bool ScreenRecorder::EncodeAndMuxOne()
{
	// Convert input -> YUV420P
	int ret = av_frame_make_writable(m_encodeFrame.get());
	if (ret < 0)
	{
		Debug::Log("av_frame_make_writable(encodeFrame) failed: " + AvErr(ret));
		return false;
	}

	int scaled = sws_scale(
		m_colorConverter.get(),
		m_inputFrame->data,
		m_inputFrame->linesize,
		0,
		m_videoEncoder->height,
		m_encodeFrame->data,
		m_encodeFrame->linesize
	);

	// sws_scale returns number of output lines on success, 0 or negative on failure
	if (scaled <= 0)
	{
		Debug::Log("sws_scale failed or produced no output lines. scaled=" + std::to_string(scaled));
		return false;
	}

	m_encodeFrame->pts = m_frameIndex++;

	ret = avcodec_send_frame(m_videoEncoder.get(), m_encodeFrame.get());
	if (ret < 0)
	{
		Debug::Log("avcodec_send_frame failed: " + AvErr(ret));
		return false;
	}

	while (true)
	{
		ret = avcodec_receive_packet(m_videoEncoder.get(), m_encodedPacket.get());

		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break; // No packet available right now, or stream done — both are normal

		if (!WritePacket(m_encodedPacket.get()))
		{
			Debug::Log("WritePacket failed");
			av_packet_unref(m_encodedPacket.get());
			return false;
		}

		av_packet_unref(m_encodedPacket.get());
	}
	return true;
}

bool ScreenRecorder::WritePacket(AVPacket* pkt)
{
	av_packet_rescale_ts(pkt, m_videoEncoder->time_base, m_videoStream->time_base);
	pkt->stream_index = m_videoStream->index;

	int ret = av_interleaved_write_frame(m_container.get(), pkt);
	if (ret < 0)
	{
		Debug::Log("av_interleaved_write_frame failed: " + AvErr(ret));
		return false;
	}
	return true;
}

bool ScreenRecorder::FlushEncoder()
{
	if (!m_videoEncoder) return true;

	int ret = avcodec_send_frame(m_videoEncoder.get(), nullptr);

	if (ret < 0)
	{
		Debug::Log("avcodec_send_frame(nullptr) failed: " + AvErr(ret));
		return false;
	}

	while (true)
	{
		ret = avcodec_receive_packet(m_videoEncoder.get(), m_encodedPacket.get());

		if (ret == AVERROR_EOF)	break;

		if (!WritePacket(m_encodedPacket.get()))
		{
			Debug::Log("Flush: WritePacket failed");
			av_packet_unref(m_encodedPacket.get());
			return false;
		}

		av_packet_unref(m_encodedPacket.get());
	}

	return true;
}
