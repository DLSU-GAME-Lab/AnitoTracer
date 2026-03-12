#pragma once
#include <unordered_map>
#include <vector>
#include <OpenImageDenoise/oidn.hpp>
#include "DenoiseWorker.hpp"

enum class FilterType
{
	Beauty,
	Albedo,
	Normal
};

class Denoiser
{
public:
	using FilterGroup = std::vector<oidn::FilterRef>;
	using FilterMap = std::unordered_map<std::string, FilterGroup>;

	Denoiser(uint16_t width, uint16_t height);
	~Denoiser() = default;

	bool RunDenoising(const void* rgba8Data);
	const void* GetDenoisedData();

	/* For Worker Thread */
	void OnFinishedExecution();
	void ExecuteDenoiseJob();

	bool IsRunning() const { return m_isRunning; }
	bool IsFinished() const { return m_isFinished; }

private:
	static float  SRGBToLinear(uint8_t c);
	static uint8_t LinearToSRGB(float v);

private:
	bool m_isRunning = false;
	bool m_isFinished = false;

	uint16_t m_width = 0;
	uint16_t m_height = 0;

	oidn::DeviceRef m_device = nullptr;
	oidn::BufferRef m_inputColor = nullptr;
	oidn::BufferRef m_output = nullptr;
	oidn::FilterRef m_beautyFilter = nullptr;

	std::vector<uint8_t> m_pendingInput;
	std::vector<uint8_t> m_outputRGBA8;

	std::unique_ptr<DenoiseWorker> m_denoiseWorker = nullptr;
};