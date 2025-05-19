#include "Profiler.h"
#include <imgui.h>
#include <algorithm>
#include <cstdio> // For printf debug (optional)

GpuCpuProfiler::GpuCpuProfiler(VkDevice device, float timestampPeriod, int maxSections)
    : device(device), timestampPeriod(timestampPeriod), maxSections(maxSections)
{
    VkQueryPoolCreateInfo qpi{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    qpi.queryType = VK_QUERY_TYPE_TIMESTAMP;
    qpi.queryCount = static_cast<uint32_t>(maxSections * 2);
    qpi.flags = 0;
    vkCreateQueryPool(device, &qpi, nullptr, &queryPool);

    cpuHistory.resize(maxSections);
    gpuHistory.resize(maxSections);
}

GpuCpuProfiler::~GpuCpuProfiler() {
    if (queryPool != VK_NULL_HANDLE) {
        vkDestroyQueryPool(device, queryPool, nullptr);
    }
}

void GpuCpuProfiler::BeginFrame(VkCommandBuffer cmd) {
    currentSection = 0;
    sections.clear();
    vkCmdResetQueryPool(cmd, queryPool, 0, maxSections * 2);
}

void GpuCpuProfiler::BeginSection(const char* name, VkCommandBuffer cmd) {
    if (currentSection >= maxSections) return;

    uint32_t idx = currentSection * 2;
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, idx);

    ProfilerSection section{
        name,
        idx,
        idx + 1,
        std::chrono::high_resolution_clock::now()
    };

    sections.push_back(section);
    // Debug: printf("BeginSection %s idx %d\n", name, currentSection);
    currentSection++;
}

void GpuCpuProfiler::EndSection(VkCommandBuffer cmd) {
    if (sections.empty()) return;

    ProfilerSection& section = sections.back();
    section.cpuEnd = std::chrono::high_resolution_clock::now();
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, section.queryIndexEnd);

    // Debug: printf("EndSection idx %zu\n", sections.size() - 1);
}

void GpuCpuProfiler::EndFrame(VkCommandBuffer /*cmd*/) {
    // No extra work needed here for now
}

void GpuCpuProfiler::FetchResults() {
    for (int i = 0; i < currentSection; ++i) {
        const auto& sec = sections[i];

        uint64_t timestamps[2] = {};
        VkResult result = vkGetQueryPoolResults(
            device,
            queryPool,
            sec.queryIndexStart,
            2,
            sizeof(timestamps),
            timestamps,
            sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
        );

        if (result != VK_SUCCESS) continue;

        double gpuTimeMs = (timestamps[1] - timestamps[0]) * timestampPeriod * 1e-6;
        double cpuTimeMs = std::chrono::duration<double, std::milli>(sec.cpuEnd - sec.cpuStart).count();

        if (gpuHistory[i].size() >= MAX_HISTORY) {
            gpuHistory[i].erase(gpuHistory[i].begin());
            cpuHistory[i].erase(cpuHistory[i].begin());
        }

        gpuHistory[i].push_back(static_cast<float>(gpuTimeMs));
        cpuHistory[i].push_back(static_cast<float>(cpuTimeMs));
    }
}

void GpuCpuProfiler::DrawImGui() {
    for (int i = 0; i < currentSection; ++i) {
        const char* label = sections[i].name;

        ImGui::Text("%s", label);

        if (!gpuHistory[i].empty()) {
            float minGpu = *std::min_element(gpuHistory[i].begin(), gpuHistory[i].end());
            float maxGpu = *std::max_element(gpuHistory[i].begin(), gpuHistory[i].end());

            float minCpu = *std::min_element(cpuHistory[i].begin(), cpuHistory[i].end());
            float maxCpu = *std::max_element(cpuHistory[i].begin(), cpuHistory[i].end());

            std::string gpuLabel = std::string(label) + " GPU (ms)";
            std::string cpuLabel = std::string(label) + " CPU (ms)";

            ImGui::PlotLines(gpuLabel.c_str(), gpuHistory[i].data(), gpuHistory[i].size(), 0, nullptr, minGpu, maxGpu, ImVec2(0, 50));
            ImGui::PlotLines(cpuLabel.c_str(), cpuHistory[i].data(), cpuHistory[i].size(), 0, nullptr, minCpu, maxCpu, ImVec2(0, 50));
        }
    }
}
