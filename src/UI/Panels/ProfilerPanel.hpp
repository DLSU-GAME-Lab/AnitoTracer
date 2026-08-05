#pragma once

#include "BasePanel.hpp"
#include <string>

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Common/interface/RefCntAutoPtr.hpp"

namespace Diligent {

    class ProfilerPanel : public BasePanel
    {
    public:
        ProfilerPanel(IRenderDevice* pDevice, const std::string& name = "Profiler");
        ~ProfilerPanel() override = default;

        // Implementation of the abstract Draw method
        void Draw() override;

    private:
        // Helper sampling methods
        void UpdateSystemMetrics();

        RefCntAutoPtr<IRenderDevice> m_pDevice;

        // Cached metrics updated on a timer interval
        float m_CpuUsagePct = 0.0f;

        float m_RamUsagePct = 0.0f;
        float m_RamUsedMB = 0.0f;
        float m_RamTotalMB = 0.0f;

        float m_VramUsagePct = 0.0f;
        float m_VramUsedMB = 0.0f;
        float m_VramTotalMB = 0.0f;

        // Sampling timer variables
        double m_LastSampleTime = 0.0;
        const double m_SampleInterval = 0.5; // Update metrics twice per second
    };

}