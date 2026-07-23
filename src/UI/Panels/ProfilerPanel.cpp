#include "ProfilerPanel.hpp"

#include "imgui.h"
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

#include <vulkan/vulkan.h>
// Include Diligent's Vulkan Backend Interface & standard Vulkan headers
#include "Graphics/GraphicsEngineVulkan/interface/RenderDeviceVk.h"


namespace Diligent {

    ProfilerPanel::ProfilerPanel(IRenderDevice* pDevice, const std::string& name)
        : BasePanel(name), m_pDevice(pDevice)
    {}

    void ProfilerPanel::Draw()
    {
        if (!m_IsVisible) return;

        if (ImGui::Begin(m_Name.c_str(), &m_IsVisible))
        {
            // Sample system metrics periodically to keep progress bars smooth
            UpdateSystemMetrics();

            // ------------------------------------------------------------------
            // Frame Rate & Timing
            // ------------------------------------------------------------------
            float fps = ImGui::GetIO().Framerate;
            float frameTimeMs = (fps > 0.0f) ? (1000.0f / fps) : 0.0f;

            ImGui::Text("Performance");
            ImGui::Text("FPS: %.1f", fps);
            ImGui::SameLine();
            ImGui::TextDisabled("(%.2f ms)", frameTimeMs);

            ImGui::Separator();

            // ------------------------------------------------------------------
            // CPU Usage Bar
            // ------------------------------------------------------------------
            char cpuText[32];
            snprintf(cpuText, sizeof(cpuText), "%.1f%%", m_CpuUsagePct * 100.0f);

            ImGui::Text("CPU Usage");
            ImGui::ProgressBar(m_CpuUsagePct, ImVec2(-1.0f, 0.0f), cpuText);

            ImGui::Spacing();

            // ------------------------------------------------------------------
            // RAM Usage Bar
            // ------------------------------------------------------------------
            char ramText[64];
            snprintf(ramText, sizeof(ramText), "%.1f MB / %.1f MB (%.1f%%)",
                m_RamUsedMB, m_RamTotalMB, m_RamUsagePct * 100.0f);

            ImGui::Text("RAM Usage (System)");
            ImGui::ProgressBar(m_RamUsagePct, ImVec2(-1.0f, 0.0f), ramText);

            ImGui::Spacing();

            // ------------------------------------------------------------------
            // GPU / VRAM Usage Bar
            // ------------------------------------------------------------------
            char vramText[64];
            if (m_VramTotalMB > 0.0f)
            {
                snprintf(vramText, sizeof(vramText), "%.1f MB / %.1f MB (%.1f%%)",
                    m_VramUsedMB, m_VramTotalMB, m_VramUsagePct * 100.0f);
            }
            else
            {
                snprintf(vramText, sizeof(vramText), "N/A");
            }

            ImGui::Text("VRAM / GPU Memory");
            ImGui::ProgressBar(m_VramUsagePct, ImVec2(-1.0f, 0.0f), vramText);
        }
        ImGui::End();
    }

    void ProfilerPanel::UpdateSystemMetrics()
    {
        double currentTime = ImGui::GetTime();
        if (currentTime - m_LastSampleTime < m_SampleInterval)
        {
            return; // Skip sampling until interval elapses
        }
        m_LastSampleTime = currentTime;

#ifdef _WIN32
        // 1. RAM Usage Query (Windows)
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memStatus))
        {
            m_RamTotalMB = static_cast<float>(memStatus.ullTotalPhys) / (1024.0f * 1024.0f);
            m_RamUsedMB = static_cast<float>(memStatus.ullTotalPhys - memStatus.ullAvailPhys) / (1024.0f * 1024.0f);
            m_RamUsagePct = static_cast<float>(memStatus.dwMemoryLoad) / 100.0f;
        }

        // 2. CPU Usage Query (Simplified Process / System Time Sample)
        static FILETIME prevIdleTime{}, prevKernelTime{}, prevUserTime{};
        FILETIME idleTime, kernelTime, userTime;

        if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
        {
            auto FileTimeToUint64 = [](const FILETIME& ft) -> ULONGLONG {
                return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
                };

            ULONGLONG idle = FileTimeToUint64(idleTime) - FileTimeToUint64(prevIdleTime);
            ULONGLONG kernel = FileTimeToUint64(kernelTime) - FileTimeToUint64(prevKernelTime);
            ULONGLONG user = FileTimeToUint64(userTime) - FileTimeToUint64(prevUserTime);

            ULONGLONG total = kernel + user;
            if (total > 0)
            {
                m_CpuUsagePct = static_cast<float>(total - idle) / static_cast<float>(total);
            }

            prevIdleTime = idleTime;
            prevKernelTime = kernelTime;
            prevUserTime = userTime;
        }
#endif

        // ------------------------------------------------------------------
        // VRAM Query (Vulkan Native via Diligent Engine)
        // ------------------------------------------------------------------
        if (m_pDevice && m_pDevice->GetDeviceInfo().Type == RENDER_DEVICE_TYPE_VULKAN)
        {
            // Cast generic IRenderDevice to IRenderDeviceVk
            RefCntAutoPtr<IRenderDeviceVk> pDeviceVk(m_pDevice, IID_RenderDeviceVk);

            if (pDeviceVk)
            {
                // Retrieve the underlying Vulkan physical device
                VkPhysicalDevice vkPhysicalDevice = (VkPhysicalDevice)(size_t)pDeviceVk->GetVkPhysicalDevice();

                PFN_vkGetPhysicalDeviceMemoryProperties2 pfnGetMemProps2 = nullptr;

#ifdef _WIN32
                // Safely load the function pointer directly from the Vulkan DLL
                // This bypasses any uninitialized module-level dispatchers
                HMODULE hVulkan = GetModuleHandleW(L"vulkan-1.dll");
                if (hVulkan)
                {
                    pfnGetMemProps2 = (PFN_vkGetPhysicalDeviceMemoryProperties2)GetProcAddress(hVulkan, "vkGetPhysicalDeviceMemoryProperties2");
                    if (!pfnGetMemProps2)
                    {
                        pfnGetMemProps2 = (PFN_vkGetPhysicalDeviceMemoryProperties2)GetProcAddress(hVulkan, "vkGetPhysicalDeviceMemoryProperties2KHR");
                    }
                }
#endif

                if (pfnGetMemProps2)
                {
                    VkPhysicalDeviceMemoryBudgetPropertiesEXT memoryBudget = {};
                    memoryBudget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
                    memoryBudget.pNext = nullptr;

                    // Setup the parent properties struct with zero-initialization {} and link pNext
                    VkPhysicalDeviceMemoryProperties2 memoryProps2 = {};
                    memoryProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
                    memoryProps2.pNext = &memoryBudget; // Safely chain the budget struct

                    // Query properties safely using the resolved function pointer
                    pfnGetMemProps2(vkPhysicalDevice, &memoryProps2);

                    uint64_t totalVram = 0;
                    uint64_t usedVram = 0;

                    // Accumulate totals across all DEVICE_LOCAL (VRAM) memory heaps
                    for (uint32_t i = 0; i < memoryProps2.memoryProperties.memoryHeapCount; i++)
                    {
                        if (memoryProps2.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                        {
                            totalVram += memoryBudget.heapBudget[i];
                            usedVram += memoryBudget.heapUsage[i];
                        }
                    }

                    // Assign to the panel's VRAM tracking variables
                    m_VramTotalMB = static_cast<float>(totalVram) / (1024.0f * 1024.0f);
                    m_VramUsedMB = static_cast<float>(usedVram) / (1024.0f * 1024.0f);

                    if (m_VramTotalMB > 0.0f)
                    {
                        m_VramUsagePct = m_VramUsedMB / m_VramTotalMB;
                    }
                }
                else
                {
                    // Fallback state if the extension is completely unavailable
                    m_VramTotalMB = 0.0f;
                    m_VramUsedMB = 0.0f;
                    m_VramUsagePct = 0.0f;
                }
            }
        }
    }

}