#pragma once
#include "Rendering/Pipelines/PipelineDefs.hpp"

// Include your math library or other dependencies if future settings need them
// #include <glm/glm.hpp> 

namespace Diligent {
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
        // Returning by reference allows for easy, direct modification of the struct
        ShadowSettings& GetShadowSettings() { return m_ShadowSettings; }
        const ShadowSettings& GetShadowSettings() const { return m_ShadowSettings; }

        // Example of how you can easily add more settings groups later:
        // GraphicsSettings& GetGraphicsSettings() { return m_GraphicsSettings; }
        // AudioSettings& GetAudioSettings() { return m_AudioSettings; }

        bool& GetEnableMSAA() { return m_EnableMSAA; }
        const bool& GetEnableMSAA() const { return m_EnableMSAA; }

    private:
        // Private constructor ensures it can only be created via GetInstance()
        UserSettings() = default;
        ~UserSettings() = default;

        // ------------------------------------------------------------------
        // Data Storage
        // ------------------------------------------------------------------
        ShadowSettings m_ShadowSettings;
        bool m_EnableMSAA = true; // Added MSAA toggle state

        // GraphicsSettings m_GraphicsSettings;
        // AudioSettings m_AudioSettings;
    };
}