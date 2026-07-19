#pragma once

#include <string>
#include <unordered_map>

#include "RenderDevice.h"
#include "RefCntAutoPtr.hpp"

namespace Diligent {

    class ShaderManager {
    public:
        // Meyers Singleton access
        static ShaderManager& GetInstance()
        {
            static ShaderManager instance;
            return instance;
        }

        // Initialize the manager and stream factory
        void Initialize(IRenderDevice* pDevice, const char* shaderDirectory = "Shaders");

        // Loads, compiles, and caches the shader
        IShader* GetShader(const std::string& filename,
            SHADER_TYPE type,
            const char* entryPoint = "main");

        // Clears the cache
        void ClearCache();

        // Cleanup resources
        void Shutdown();

        // Check if the manager is ready
        bool IsInitialized() const { return m_pDevice != nullptr; }

    private:
        ShaderManager() = default;
        ~ShaderManager() = default;

        // Disable copy/move
        ShaderManager(const ShaderManager&) = delete;
        ShaderManager& operator=(const ShaderManager&) = delete;

        RefCntAutoPtr<IRenderDevice> m_pDevice;
        RefCntAutoPtr<IShaderSourceInputStreamFactory> m_pShaderSourceFactory;

        // Cache key combines filename, type, and entry point
        std::unordered_map<std::string, RefCntAutoPtr<IShader>> m_ShaderCache;
    };

}