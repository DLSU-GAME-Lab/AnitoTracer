#pragma once
#include <chrono>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

struct FrameDeleter {
	void operator()(AVFrame* f) const noexcept { av_frame_free(&f); }
};

struct PacketDeleter {
	void operator()(AVPacket* p) const noexcept { av_packet_free(&p); }
};

struct CodecCtxDeleter {
	void operator()(AVCodecContext* c) const noexcept { avcodec_free_context(&c); }
};

struct ScalerDeleter {
	void operator()(SwsContext* s) const noexcept { sws_freeContext(s); }
};

struct FormatCtxDeleter
{
	void operator()(AVFormatContext* f) const noexcept
	{
		if (!f) return;
		if (!(f->oformat->flags & AVFMT_NOFILE) && f->pb)	avio_closep(&f->pb);
		avformat_free_context(f);
	}
};

class ScreenRecorder
{
public:
	using Frame = std::unique_ptr<AVFrame, FrameDeleter>;
	using Packet = std::unique_ptr<AVPacket, PacketDeleter>;
	using CodecCtx = std::unique_ptr<AVCodecContext, CodecCtxDeleter>;
	using Scaler = std::unique_ptr<SwsContext, ScalerDeleter>;
	using FormatCtx = std::unique_ptr<AVFormatContext, FormatCtxDeleter>;
	using Clock = std::chrono::steady_clock;
	using TimePoint = Clock::time_point;

	struct Config {
		int width = 0;
		int height = 0;
		double targetFps = 24.0;
		int crf = 18;
		std::string preset = "p1";
		int gopSize = 60;
		int maxBFrames = 0;
		bool fastStart = true;
		bool flipY = false;
	};

	ScreenRecorder();
	~ScreenRecorder();

	bool Start(const std::string& fileName, const Config& config);
	void Stop();

	void PushFrame(const uint8_t* src, size_t srcStrideBytes, TimePoint timePoint);

	bool IsRecording() const;

private:
	bool InitPipeline();
	void ResetPipeline();

	bool ShouldAccept(TimePoint ts);
	void CopyIntoInputFrame(const uint8_t* src, size_t srcStrideBytes);

	bool EncodeAndMuxOne();
	bool WritePacket(AVPacket* pkt);
	bool FlushEncoder();

private:
	struct RawFrame
	{
		TimePoint time;
		std::vector<uint8_t> bytes;
	};

	Config m_config{};
	std::string m_pathPrefix;
	std::string m_path;
	bool m_started = false;

	// Rate limiter
	bool m_hasNextAccept = false;
	TimePoint m_nextAccept{};
	std::chrono::duration<double> m_interval{ 1.0 / 30.0 };

	// ---- Container (MP4 muxer) ----
	FormatCtx m_container;
	AVStream* m_videoStream = nullptr; // non-owning

	// ---- Encoder ----
	CodecCtx m_videoEncoder;
	Frame    m_inputFrame;     // renderer pixels
	Frame    m_encodeFrame;    // encoder-ready frame
	Packet   m_encodedPacket;

	// ---- Conversion ----
	Scaler   m_colorConverter;

	int64_t m_frameIndex = 0;
};