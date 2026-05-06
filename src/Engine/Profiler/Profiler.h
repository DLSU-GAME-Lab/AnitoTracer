#pragma once
#include <vulkan/vulkan.h>
#include <chrono>
#include <vector>
#include "From-GDGRAP2/EventBroadcaster.h"

constexpr int MAX_HISTORY = 100; // Number of frames to store per section

struct ProfilerSection {
    const char* name;
    uint32_t queryIndexStart;
    uint32_t queryIndexEnd;
    std::chrono::high_resolution_clock::time_point cpuStart;
    std::chrono::high_resolution_clock::time_point cpuEnd;
};
struct MemoryUsageStats {
    uint64_t totalBudget = 0;
    uint64_t totalUsage = 0;
};

class GpuCpuProfiler : public Observer {
public:
    GpuCpuProfiler(VkDevice device, VkPhysicalDevice physicalDevice, float timestampPeriod, int maxSections);
    ~GpuCpuProfiler();

    void BeginFrame(VkCommandBuffer cmd);
    void BeginSection(const char* name, VkCommandBuffer cmd);
    void EndSection(VkCommandBuffer cmd);
    void EndFrame(VkCommandBuffer cmd);

    void FetchResults();
    void DrawImGui();

private:
    VkDevice device;
    float timestampPeriod;
    VkQueryPool queryPool;
    int maxSections;
    int currentSection = 0;

    std::vector<ProfilerSection> sections;
    std::vector<std::vector<float>> cpuHistory;
    std::vector<std::vector<float>> gpuHistory;

	MemoryUsageStats vramStats;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // Add this if needed

public:
    void UpdateMemoryStats();
    const MemoryUsageStats& GetMemoryStats() const { return vramStats; }

private:
    void onTriggeredEvent(std::string eventName, std::shared_ptr<Parameters> parameters = nullptr) override;

    int m_samplePercentage = 0;
    int m_currentSamples = 0;
    int m_maxSamples = 0;
};
