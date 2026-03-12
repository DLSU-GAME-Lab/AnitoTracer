#include "Denoiser.hpp"
#include "From-GDGRAP2/Debug.h"
#include <iostream>

Denoiser::Denoiser(uint16_t width, uint16_t height) : m_width(width), m_height(height)
{
	m_device = oidn::newDevice(oidn::DeviceType::Default);
	m_device.commit();

	const char* errorMessage = nullptr;
	if (m_device.getError(errorMessage) != oidn::Error::None)
	{
		Debug::Log(std::string("OIDN device error: ") + (errorMessage ? errorMessage : "Unknown error"));
		return;
	}

	const size_t bufferSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 3 * sizeof(float);

	m_inputColor = m_device.newBuffer(bufferSize);
	m_output = m_device.newBuffer(bufferSize);

	if (!m_inputColor || !m_output)
	{
		Debug::Log("OIDN buffer creation failed");
		return;
	}

	m_beautyFilter = m_device.newFilter("RT");
	if (!m_beautyFilter)
	{
		Debug::Log("OIDN filter creation failed");
		return;
	}
	
	m_beautyFilter = m_device.newFilter("RT");
	m_beautyFilter.setImage("color", m_inputColor, oidn::Format::Float3, width, height);
	m_beautyFilter.setImage("output", m_output, oidn::Format::Float3, width, height);
	m_beautyFilter.set("hdr", true);
	m_beautyFilter.commit();

	m_pendingInput.resize(static_cast<size_t>(width) * height * 4);
	m_outputRGBA8.resize(static_cast<size_t>(width) * height * 4);
	m_denoiseWorker = std::make_unique<DenoiseWorker>(this);
}

bool Denoiser::RunDenoising(const void* rgba8Data)
{
	if (m_isRunning)
	{
		//Debug::Log("Denoising is already running. Please wait for the current process to finish.");
		return false;
	}

	if (!m_inputColor)
	{
		Debug::Log("Input color buffer is not initialized.");
		return false;
	}

	if (!m_denoiseWorker)
	{
		Debug::Log("Denoise worker is not initialized.");
		return false;
	}

	std::memcpy(m_pendingInput.data(), rgba8Data, static_cast<size_t>(m_width) * m_height * 4);

	m_isRunning = true;
	m_denoiseWorker->start();

	return true;
}

const void* Denoiser::GetDenoisedData()
{
	if (m_isRunning || m_outputRGBA8.empty() || !m_isFinished)
        return nullptr;

	m_isFinished = false; // Reset the finished flag for the next run
    return m_outputRGBA8.data();
}

void Denoiser::OnFinishedExecution()
{
	const auto* src = static_cast<const float*>(m_output.getData());
	const size_t pixCount = static_cast<size_t>(m_width) * m_height;

	for (size_t i = 0; i < pixCount; ++i)
	{
		m_outputRGBA8[i * 4 + 0] = LinearToSRGB(src[i * 3 + 0]); // R
		m_outputRGBA8[i * 4 + 1] = LinearToSRGB(src[i * 3 + 1]); // G
		m_outputRGBA8[i * 4 + 2] = LinearToSRGB(src[i * 3 + 2]); // B
		m_outputRGBA8[i * 4 + 3] = 255;                          // A — fully opaque
	}

	m_isRunning = false;
	m_isFinished = true;
}

void Denoiser::ExecuteDenoiseJob()
{
	const auto* src = static_cast<const uint8_t*>(m_pendingInput.data());
	auto* dst = static_cast<float*>(m_inputColor.getData());
	const size_t pixCount = static_cast<size_t>(m_width) * m_height;

	for (size_t i = 0; i < pixCount; ++i)
	{
		dst[i * 3 + 0] = SRGBToLinear(src[i * 4 + 0]);
		dst[i * 3 + 1] = SRGBToLinear(src[i * 4 + 1]);
		dst[i * 3 + 2] = SRGBToLinear(src[i * 4 + 2]);
		// Alpha channel is ignored for denoising, as OIDN expects RGB input. If needed, it can be handled separately.
	}

	m_pendingInput.clear();
	m_beautyFilter.execute();
	const char* errorMessage;
	if (m_device.getError(errorMessage) != oidn::Error::None)
		Debug::Log("OIDN execute error: " + std::string(errorMessage ? errorMessage : "Unknown"));
}

float Denoiser::SRGBToLinear(uint8_t c)
{
	const float v = c / 255.0f;
	return (v <= 0.04045f) ? v / 12.92f
		: std::pow((v + 0.055f) / 1.055f, 2.4f);
}

uint8_t Denoiser::LinearToSRGB(float v)
{
	v = std::clamp(v, 0.0f, 1.0f);
	const float encoded = (v <= 0.0031308f) ? v * 12.92f
		: 1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f;
	return static_cast<uint8_t>(encoded * 255.0f + 0.5f);
}