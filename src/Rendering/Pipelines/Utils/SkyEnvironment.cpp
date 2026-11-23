#include "SkyEnvironment.hpp"
#include "PSOBuilder.hpp"
#include "PipelineUtils.hpp"
#include "../../../Asset/ProjectSettings.hpp"

#include "TextureLoader/interface/TextureUtilities.h"

#include <glm/gtc/packing.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>

namespace Diligent {

    namespace {
        // Every cube is floating point, so sky brightness above white survives for lighting and tone mapping
        constexpr TEXTURE_FORMAT kCubeFormat = TEX_FORMAT_RGBA16_FLOAT;
        // Largest value a cube keeps, just under the limit of a 16-bit float (also in Shaders/Skybox/sky_bake_ps.hlsl)
        constexpr float kMaxSkyValue = 64000.0f;

        // Face size of the sky cubemap: the source's own detail, within these limits
        constexpr Uint32 kMinSkyFaceSize = 64;
        constexpr Uint32 kMaxSkyFaceSize = 2048;

        // Diffuse light changes slowly across the sky, so its cube can be small
        constexpr Uint32 kIrradianceFaceSize = 64;
        // Reflections: mip 0 is a mirror image of the sky, each further mip a rougher surface, down to kMinSpecularMipSize
        constexpr Uint32 kMaxSpecularFaceSize = 512;
        constexpr Uint32 kMinSpecularMipSize = 8;
        constexpr Uint32 kSpecularSamples = 256;

        // Mirrors cbuffer SkyBakeConstants in Shaders/Skybox/sky_bake_ps.hlsl
        struct SkyBakeConstants
        {
            Uint32 Face = 0;
            float SourceLod = 0.0f;
            float Padding[2] = { 0.0f, 0.0f };
        };
        static_assert(sizeof(SkyBakeConstants) % 16 == 0, "SkyBakeConstants must be 16-byte aligned for HLSL");

        // Mirrors cbuffer IBLPrecomputeConstants in Shaders/external/DiligentFX/ibl_precompute_common.hlsli
        struct IBLPrecomputeConstants
        {
            float Roughness = 0.0f;
            float EnvMapWidth = 0.0f;
            float EnvMapHeight = 0.0f;
            float EnvMipCount = 0.0f;

            Uint32 NumSamples = 1;
            Uint32 Face = 0;
            Uint32 Padding[2] = { 0, 0 };
        };
        static_assert(sizeof(IBLPrecomputeConstants) % 16 == 0, "IBLPrecomputeConstants must be 16-byte aligned for HLSL");

        // mipLevels 0 gives the full chain down to 1x1
        RefCntAutoPtr<ITexture> CreateCube(IRenderDevice* pDevice, const char* name, Uint32 faceSize, Uint32 mipLevels, bool generateMips)
        {
            TextureDesc Desc;
            Desc.Name = name;
            Desc.Type = RESOURCE_DIM_TEX_CUBE;
            Desc.Width = faceSize;
            Desc.Height = faceSize;
            Desc.ArraySize = 6;
            Desc.MipLevels = mipLevels;
            Desc.Format = kCubeFormat;
            Desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
            if (generateMips) Desc.MiscFlags = MISC_TEXTURE_FLAG_GENERATE_MIPS;

            RefCntAutoPtr<ITexture> pTexture;
            pDevice->CreateTexture(Desc, nullptr, &pTexture);
            return pTexture;
        }

        Uint32 MipSize(Uint32 size, Uint32 mip)
        {
            return (std::max)(size >> mip, 1u);
        }

        Uint32 NextPowerOfTwo(Uint32 value)
        {
            Uint32 result = 1;
            while (result < value) result <<= 1;
            return result;
        }

        // .hdr files store linear light. Everything else that isn't a float format stores display (sRGB) colors.
        bool IsHDRFile(const std::string& path)
        {
            std::string extension = std::filesystem::path(path).extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return extension == ".hdr";
        }

        // Paths inside the project are stored relative to it. Anything else is used as is, which for a relative
        // path means relative to the working directory (the built-in Assets folder).
        std::string ResolveTexturePath(const std::string& texturePath, const std::filesystem::path& projectDir)
        {
            if (texturePath.empty()) return SkyEnvironment::kDefaultSkyboxPath;

            const std::filesystem::path path(texturePath);
            if (path.is_relative() && !projectDir.empty()) {
                std::error_code ec;
                const std::filesystem::path inProject = projectDir / path;
                if (std::filesystem::exists(inProject, ec)) return inProject.string();
            }
            return texturePath;
        }
    }

    void SkyEnvironment::Initialize(IRenderDevice* pDevice)
    {
        if (m_pDevice) return;
        m_pDevice = pDevice;

        m_pBakeCB = PipelineUtils::CreateDynamicCB<SkyBakeConstants>(pDevice, "Sky Bake Constant Buffer");
        m_pFilterCB = PipelineUtils::CreateDynamicCB<IBLPrecomputeConstants>(pDevice, "Sky Filter Constant Buffer");

        // A panorama wraps around horizontally, so filtering has to continue across its left and right edges
        SamplerDesc panoramaSampler = PipelineUtils::LinearClampSampler();
        panoramaSampler.AddressU = TEXTURE_ADDRESS_WRAP;

        m_BakeFromCube = CreateCubePass("Sky Bake From Cubemap", "Skybox/sky_bake_ps.hlsl", "g_Source", "SkyBakeConstants",
            PipelineUtils::LinearClampSampler(), m_pBakeCB);
        m_BakeFromPanorama = CreateCubePass("Sky Bake From Panorama", "Skybox/sky_bake_panorama_ps.hlsl", "g_Source", "SkyBakeConstants",
            panoramaSampler, m_pBakeCB);
        m_IrradiancePass = CreateCubePass("Sky Irradiance Filter", "external/DiligentFX/compute_irradiance_map_ps.hlsl", "g_EnvironmentMap", "IBLPrecomputeConstants",
            PipelineUtils::LinearClampSampler(), m_pFilterCB);
        m_SpecularPass = CreateCubePass("Sky Specular Filter", "external/DiligentFX/prefilter_env_map_ps.hlsl", "g_EnvironmentMap", "IBLPrecomputeConstants",
            PipelineUtils::LinearClampSampler(), m_pFilterCB);
    }

    SkyEnvironment::CubePass SkyEnvironment::CreateCubePass(const char* name, const char* pixelShader, const char* textureName,
        const char* constantsName, const SamplerDesc& sampler, IBuffer* pConstantsCB)
    {
        const std::string samplerName = std::string(textureName) + "_sampler";

        CubePass pass;
        pass.pPSO = PSOBuilder(std::string(name) + " PSO")
            .Shaders("Deferred/fullscreen_vs.hlsl", pixelShader)
            .RenderTargets({ kCubeFormat }, TEX_FORMAT_UNKNOWN)
            .Cull(CULL_MODE_NONE)
            .Variable(SHADER_TYPE_PIXEL, textureName)
            .Variable(SHADER_TYPE_PIXEL, constantsName)
            .Sampler(SHADER_TYPE_PIXEL, samplerName.c_str(), sampler)
            .Build(m_pDevice);
        pass.pSRB = PipelineUtils::CreateSRB(pass.pPSO);
        PipelineUtils::BindVariable(pass.pSRB, SHADER_TYPE_PIXEL, constantsName, pConstantsCB);
        return pass;
    }

    bool SkyEnvironment::Update(IDeviceContext* pContext, const ProjectSkySettings& settings, const std::filesystem::path& projectDir, bool buildLighting)
    {
        if (!m_pDevice || !pContext) return false;

        if (settings.mode == SKY_MODE_SOLID_COLOR) {
            const glm::vec3 color = glm::clamp(glm::vec3(settings.color[0], settings.color[1], settings.color[2]), 0.0f, kMaxSkyValue);
            if (m_Source == Source::SolidColor && color == m_Color) return false;

            m_Source = Source::SolidColor;
            m_Color = color;
            return BuildSolidColor(color);
        }

        if (m_Source == Source::Texture && settings.texture_path == m_TexturePath && projectDir == m_ProjectDir && buildLighting == m_LightingRequested) {
            return false;
        }

        // Remembered before building, so a file that fails to load is reported once instead of retried every frame
        const bool wasTexture = (m_Source == Source::Texture);
        m_Source = Source::Texture;
        m_TexturePath = settings.texture_path;
        m_ProjectDir = projectDir;
        m_LightingRequested = buildLighting;

        // Opening a project changes projectDir, but often still resolves to the file the cubes were built from
        const std::string path = ResolveTexturePath(settings.texture_path, projectDir);
        if (wasTexture && m_pSky && path == m_BuiltFromFile && buildLighting == m_BuiltWithLighting) return false;

        if (BuildFromTexture(pContext, path, buildLighting)) return true;

        if (path != kDefaultSkyboxPath) {
            std::cout << "[Warn] Sky texture could not be loaded, using the default skybox instead: " << path << std::endl;
            if (BuildFromTexture(pContext, kDefaultSkyboxPath, buildLighting)) return true;
        }

        std::cout << "[Warn] The default skybox could not be loaded (" << kDefaultSkyboxPath << "). Using a plain sky." << std::endl;
        return BuildSolidColor(glm::vec3(0.1f, 0.15f, 0.25f));
    }

    bool SkyEnvironment::BuildFromTexture(IDeviceContext* pContext, const std::string& path, bool buildLighting)
    {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) {
            std::cout << "[Warn] Sky texture not found: " << path << std::endl;
            return false;
        }

        // Kept only until the sky cubemap is drawn from it
        RefCntAutoPtr<ITexture> pSource;
        TextureLoadInfo loadInfo;
        loadInfo.Name = "Sky Source";
        loadInfo.IsSRGB = !IsHDRFile(path);
        loadInfo.GenerateMips = true;
        CreateTextureFromFile(path.c_str(), loadInfo, m_pDevice, &pSource);
        if (!pSource) return false;

        const TextureDesc& sourceDesc = pSource->GetDesc();
        const bool isCube = (sourceDesc.Type == RESOURCE_DIM_TEX_CUBE);
        if (!isCube && sourceDesc.Type != RESOURCE_DIM_TEX_2D) {
            std::cout << "[Warn] Sky texture is neither a cubemap nor a 2D image: " << path << std::endl;
            return false;
        }

        const CubePass& bake = isCube ? m_BakeFromCube : m_BakeFromPanorama;
        if (!bake.pPSO || !bake.pSRB) return false;
        if (buildLighting && (!m_IrradiancePass.pPSO || !m_SpecularPass.pPSO)) buildLighting = false;

        // A cubemap keeps its own face size. A panorama spans four faces across its width.
        const Uint32 nativeFaceSize = isCube ? sourceDesc.Width : NextPowerOfTwo((std::max)(sourceDesc.Width / 4, 1u));
        const Uint32 skyFaceSize = (std::clamp)(nativeFaceSize, kMinSkyFaceSize, kMaxSkyFaceSize);

        RefCntAutoPtr<ITexture> pSky = CreateCube(m_pDevice, "Sky Cubemap", skyFaceSize, 0, true);
        if (!pSky) return false;

        RefCntAutoPtr<ITexture> pIrradiance;
        RefCntAutoPtr<ITexture> pSpecular;
        if (buildLighting) {
            const Uint32 specularFaceSize = (std::min)(skyFaceSize, kMaxSpecularFaceSize);
            Uint32 specularMips = 1;
            while ((specularFaceSize >> specularMips) >= kMinSpecularMipSize) ++specularMips;

            pIrradiance = CreateCube(m_pDevice, "Sky Irradiance Cubemap", kIrradianceFaceSize, 1, false);
            pSpecular = CreateCube(m_pDevice, "Sky Specular Cubemap", specularFaceSize, specularMips, false);
            if (!pIrradiance || !pSpecular) return false;
        }

        BakeSky(pContext, pSource, pSky);
        if (buildLighting) FilterLighting(pContext, pSky, pIrradiance, pSpecular);

        // Don't leave a cube face bound as a render target: the cubes are about to be sampled
        pContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE);

        m_pSky = pSky;
        m_pIrradiance = pIrradiance;
        m_pSpecular = pSpecular;
        m_BuiltFromFile = path;
        m_BuiltWithLighting = buildLighting;

        std::cout << "[Info] Sky loaded from " << path << " (" << (isCube ? "cubemap" : "panorama") << ", "
            << skyFaceSize << " px faces" << (buildLighting ? ", with lighting cubes" : "") << ")." << std::endl;
        return true;
    }

    void SkyEnvironment::BakeSky(IDeviceContext* pContext, ITexture* pSource, ITexture* pSky)
    {
        const TextureDesc& sourceDesc = pSource->GetDesc();
        const bool isCube = (sourceDesc.Type == RESOURCE_DIM_TEX_CUBE);
        const CubePass& bake = isCube ? m_BakeFromCube : m_BakeFromPanorama;
        const Uint32 skyFaceSize = pSky->GetDesc().Width;

        // When the source has more detail than the sky cubemap keeps, read the source mip that matches,
        // so the extra detail is averaged instead of skipped over
        const float sourceTexelsAcrossFace = isCube ? static_cast<float>(sourceDesc.Width) : static_cast<float>(sourceDesc.Width) / 4.0f;

        SkyBakeConstants constants;
        constants.SourceLod = (std::max)(std::log2(sourceTexelsAcrossFace / static_cast<float>(skyFaceSize)), 0.0f);

        PipelineUtils::BindVariable(bake.pSRB, SHADER_TYPE_PIXEL, "g_Source", pSource->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        for (Uint32 face = 0; face < 6; ++face) {
            constants.Face = face;
            PipelineUtils::WriteCB(pContext, m_pBakeCB, constants);
            DrawFace(pContext, bake, pSky, face, 0);
        }
        // The pass must not keep the source file's texture alive
        PipelineUtils::BindVariable(bake.pSRB, SHADER_TYPE_PIXEL, "g_Source", nullptr);

        // Mips are what the skybox reads at a distance and what the filters below read for their widest samples
        pContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_NONE);
        pContext->GenerateMips(pSky->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    }

    void SkyEnvironment::FilterLighting(IDeviceContext* pContext, ITexture* pSky, ITexture* pIrradiance, ITexture* pSpecular)
    {
        const TextureDesc& skyDesc = pSky->GetDesc();
        ITextureView* pSkySRV = pSky->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

        IBLPrecomputeConstants constants;
        constants.EnvMapWidth = static_cast<float>(skyDesc.Width);
        constants.EnvMapHeight = static_cast<float>(skyDesc.Height);
        constants.EnvMipCount = static_cast<float>(skyDesc.MipLevels);

        // Diffuse: every texel averages the whole hemisphere around it, so it takes many samples to be free of noise.
        // Same counts DiligentFX uses by default.
        constants.NumSamples = (m_pDevice->GetAdapterInfo().Type == ADAPTER_TYPE_DISCRETE) ? 8192 : 2048;
        PipelineUtils::BindVariable(m_IrradiancePass.pSRB, SHADER_TYPE_PIXEL, "g_EnvironmentMap", pSkySRV);
        for (Uint32 face = 0; face < 6; ++face) {
            constants.Face = face;
            PipelineUtils::WriteCB(pContext, m_pFilterCB, constants);
            DrawFace(pContext, m_IrradiancePass, pIrradiance, face, 0);
        }
        PipelineUtils::BindVariable(m_IrradiancePass.pSRB, SHADER_TYPE_PIXEL, "g_EnvironmentMap", nullptr);

        // Reflections: one roughness per mip, 0 (mirror) in the first to 1 in the last. Shaders/hybrid_shading.hlsli
        // reads it back with the same mapping.
        const Uint32 specularMips = pSpecular->GetDesc().MipLevels;
        PipelineUtils::BindVariable(m_SpecularPass.pSRB, SHADER_TYPE_PIXEL, "g_EnvironmentMap", pSkySRV);
        for (Uint32 mip = 0; mip < specularMips; ++mip) {
            constants.Roughness = specularMips > 1 ? static_cast<float>(mip) / static_cast<float>(specularMips - 1) : 0.0f;
            // A mirror reflects one direction, so one sample is exact
            constants.NumSamples = (mip == 0) ? 1 : kSpecularSamples;

            for (Uint32 face = 0; face < 6; ++face) {
                constants.Face = face;
                PipelineUtils::WriteCB(pContext, m_pFilterCB, constants);
                DrawFace(pContext, m_SpecularPass, pSpecular, face, mip);
            }
        }
        PipelineUtils::BindVariable(m_SpecularPass.pSRB, SHADER_TYPE_PIXEL, "g_EnvironmentMap", nullptr);
    }

    void SkyEnvironment::DrawFace(IDeviceContext* pContext, const CubePass& pass, ITexture* pTarget, Uint32 face, Uint32 mip)
    {
        // A cubemap is an array of six faces: this view is one face at one mip
        TextureViewDesc ViewDesc;
        ViewDesc.Name = "Sky Cubemap Face";
        ViewDesc.ViewType = TEXTURE_VIEW_RENDER_TARGET;
        ViewDesc.TextureDim = RESOURCE_DIM_TEX_2D_ARRAY;
        ViewDesc.MostDetailedMip = mip;
        ViewDesc.NumMipLevels = 1;
        ViewDesc.FirstArraySlice = face;
        ViewDesc.NumArraySlices = 1;

        RefCntAutoPtr<ITextureView> pFaceRTV;
        pTarget->CreateView(ViewDesc, &pFaceRTV);
        if (!pFaceRTV) return;

        ITextureView* pRTVs[] = { pFaceRTV };
        pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const Uint32 size = MipSize(pTarget->GetDesc().Width, mip);
        PipelineUtils::SetViewport(pContext, size, size, size, size);

        pContext->SetPipelineState(pass.pPSO);
        pContext->CommitShaderResources(pass.pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        PipelineUtils::DrawFullscreenTriangle(pContext);
    }

    bool SkyEnvironment::BuildSolidColor(const glm::vec3& color)
    {
        // RGBA16_FLOAT texel
        const Uint16 texel[4] = {
            glm::packHalf1x16(color.r),
            glm::packHalf1x16(color.g),
            glm::packHalf1x16(color.b),
            glm::packHalf1x16(1.0f),
        };

        TextureDesc Desc;
        Desc.Name = "Sky Solid Color Cubemap";
        Desc.Type = RESOURCE_DIM_TEX_CUBE;
        Desc.Width = 1;
        Desc.Height = 1;
        Desc.ArraySize = 6;
        Desc.MipLevels = 1;
        Desc.Format = kCubeFormat;
        Desc.Usage = USAGE_IMMUTABLE;
        Desc.BindFlags = BIND_SHADER_RESOURCE;

        TextureSubResData Faces[6];
        for (auto& face : Faces) {
            face.pData = texel;
            face.Stride = sizeof(texel);
        }

        TextureData InitData{ Faces, Desc.ArraySize };
        RefCntAutoPtr<ITexture> pTexture;
        m_pDevice->CreateTexture(Desc, &InitData, &pTexture);
        if (!pTexture) return false;

        m_pSky = pTexture;
        m_pIrradiance = pTexture;
        m_pSpecular = pTexture;
        m_BuiltFromFile.clear();
        m_BuiltWithLighting = true;
        return true;
    }

}
