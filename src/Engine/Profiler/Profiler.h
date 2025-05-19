#pragma once
#include <vulkan/vulkan.h>
#include <chrono>
#include <vector>

constexpr int MAX_HISTORY = 100; // Number of frames to store per section

struct ProfilerSection {
    const char* name;
    uint32_t queryIndexStart;
    uint32_t queryIndexEnd;
    std::chrono::high_resolution_clock::time_point cpuStart;
    std::chrono::high_resolution_clock::time_point cpuEnd;
};

class GpuCpuProfiler {
public:
    GpuCpuProfiler(VkDevice device, float timestampPeriod, int maxSections);
    ~GpuCpuProfiler();

    void BeginFrame(VkCommandBuffer cmd);
    void BeginSection(const char* name, VkCommandBuffer cmd);
    void EndSection(VkCommandBuffer cmd);
    void EndFrame(VkCommandBuffer cmd);

    void FetchResults();
    void DrawImGui();

	void SetDevice(VkDevice device) { this->device = device; }

private:
    VkDevice device;
    float timestampPeriod;
    VkQueryPool queryPool;
    int maxSections;
    int currentSection = 0;

    std::vector<ProfilerSection> sections;
    std::vector<std::vector<float>> cpuHistory;
    std::vector<std::vector<float>> gpuHistory;
};
