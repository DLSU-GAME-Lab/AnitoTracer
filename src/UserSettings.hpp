#pragma once
#include <cstdint>
#include "Rendering/Pipelines/Data/PipelineDefs.hpp"

namespace Diligent {
    // Render resolutions the game and editor views can be drawn at
    struct RenderResolution {
        const char* Name;
        uint32_t Width;
        uint32_t Height;
    };

    // Menu order, smallest to largest
    inline constexpr RenderResolution kRenderResolutions[] = {
        { "720p",             1280, 720  },
        { "768p",             1366, 768  },
        { "900p",             1600, 900  },
        { "1080p",            1920, 1080 },
        { "1200p",            1920, 1200 },
        { "1440p",            2560, 1440 },
        { "1600p",            2560, 1600 },
        { "1440p Ultrawide",  3440, 1440 },
        { "4K",               3840, 2160 },
    };
    inline constexpr int kRenderResolutionCount = static_cast<int>(sizeof(kRenderResolutions) / sizeof(kRenderResolutions[0]));
    inline constexpr int kDefaultRenderResolution = 3; // 1080p

    // How the game and editor views fit their rendered image into the panel
    enum class ViewportScaleMode {
        Letterbox, // Keep the resolution's aspect ratio, with bars filling the rest of the panel
        Stretch,   // Fill the whole panel, distorting the image when the shapes differ
    };

    class UserSettings {
    public:
        // ------------------------------------------------------------------
        // Singleton Access
        // ------------------------------------------------------------------
        static UserSettings& GetInstance()
        {
            static UserSettings instance;
            return instance;
        }

        // Delete copy and assignment operators to enforce the singleton pattern
        UserSettings(const UserSettings&) = delete;
        UserSettings& operator=(const UserSettings&) = delete;
        UserSettings(UserSettings&&) = delete;
        UserSettings& operator=(UserSettings&&) = delete;

        // ------------------------------------------------------------------
        // Settings Accessors
        // ------------------------------------------------------------------
        bool& GetEnableMSAA() { return m_EnableMSAA; }
        const bool& GetEnableMSAA() const { return m_EnableMSAA; }

        PipelineType& GetRendererType() { return m_RendererType; }
        const PipelineType& GetRendererType() const { return m_RendererType; }

        // Index into kRenderResolutions
        int& GetResolutionIndex() { return m_ResolutionIndex; }
        const int& GetResolutionIndex() const { return m_ResolutionIndex; }
        const RenderResolution& GetResolution() const {
            const int index = (m_ResolutionIndex >= 0 && m_ResolutionIndex < kRenderResolutionCount) ? m_ResolutionIndex : kDefaultRenderResolution;
            return kRenderResolutions[index];
        }

        ViewportScaleMode& GetViewportScaleMode() { return m_ViewportScaleMode; }
        const ViewportScaleMode& GetViewportScaleMode() const { return m_ViewportScaleMode; }

    private:
        // Private constructor ensures it can only be created via GetInstance()
        UserSettings() = default;
        ~UserSettings() = default;

        // ------------------------------------------------------------------
        // Data Storage
        // ------------------------------------------------------------------
        //False for now for deferred- TODO- Fix for deferred
        bool m_EnableMSAA = false; // Added MSAA toggle state

        //Default to deferred for easy rendering
        PipelineType m_RendererType = PipelineType::DEFERRED;

        int m_ResolutionIndex = kDefaultRenderResolution;

        ViewportScaleMode m_ViewportScaleMode = ViewportScaleMode::Letterbox;
    };
}