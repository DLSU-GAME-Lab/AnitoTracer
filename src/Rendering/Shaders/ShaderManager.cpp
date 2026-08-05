#include "ShaderManager.hpp"
#include "EngineFactory.h"

namespace Diligent {

    void ShaderManager::Initialize(IRenderDevice* pDevice, const char* shaderDirectory)
    {
        // Prevent double initialization
        if (m_pDevice) return;

        m_pDevice = pDevice;

        // Initialize the stream factory mapped to your target folder
        auto* pEngineFactory = m_pDevice->GetEngineFactory();
        pEngineFactory->CreateDefaultShaderSourceStreamFactory(shaderDirectory, &m_pShaderSourceFactory);
    }

    IShader* ShaderManager::GetShader(const std::string& filename, SHADER_TYPE type, const char* entryPoint)
    {
        if (!m_pDevice || !m_pShaderSourceFactory) return nullptr;

        // 1. Check if we already compiled this exact shader
        std::string cacheKey = filename + "|" + entryPoint + "|" + std::to_string(static_cast<int>(type));
        auto it = m_ShaderCache.find(cacheKey);
        if (it != m_ShaderCache.end()) {
            return it->second;
        }

        // 2. Setup creation info for runtime compilation
        ShaderCreateInfo ShaderCI;
        ShaderCI.pShaderSourceStreamFactory = m_pShaderSourceFactory;

        // Assumes HLSL. Change to SHADER_SOURCE_LANGUAGE_GLSL if you write GLSL.
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.Desc.ShaderType = type;
        ShaderCI.Desc.Name = filename.c_str(); // Debug name
        ShaderCI.EntryPoint = entryPoint;
        ShaderCI.FilePath = filename.c_str(); // The actual file in the Shaders folder
        ShaderCI.ShaderCompiler = SHADER_COMPILER_DXC;
        ShaderCI.HLSLVersion = { 6, 5 };

        // 3. Compile the shader
        RefCntAutoPtr<IShader> pShader;
        m_pDevice->CreateShader(ShaderCI, &pShader);

        if (!pShader) {
            // Diligent will output the compilation error to the console automatically
            return nullptr;
        }

        // 4. Cache and return
        m_ShaderCache[cacheKey] = pShader;
        return pShader;
    }

    void ShaderManager::ClearCache()
    {
        m_ShaderCache.clear();
    }

    void ShaderManager::Shutdown()
    {
        ClearCache();
        // Explicitly release Diligent engine smart pointers
        m_pShaderSourceFactory.Release();
        m_pDevice.Release();
    }
}