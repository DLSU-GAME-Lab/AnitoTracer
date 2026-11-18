#pragma once

#include <filesystem>
#include <string>

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Common/interface/RefCntAutoPtr.hpp"

#include <glm/glm.hpp>

struct ProjectSkySettings;

namespace Diligent {

    // The sky as the renderer uses it, built from the project's sky settings (a cubemap file, a 360 degree panorama
    // file, or a solid color):
    //   - the sky cubemap, drawn by SkyboxPipeline. Floating point with a full mip chain, whatever the source was,
    //     so an .hdr sky keeps its real brightness.
    //   - an irradiance cube and a prefiltered specular cube filtered from it, which the ray-traced pipelines light
    //     the scene with (ScenePipeline::SetSkyLighting). Only built when asked for.
    // The sky's rotation and intensity are not baked into the cubes. Shaders apply them (BuildSkyboxConstants,
    // BuildShadowSettings), so changing them costs nothing.
    class SkyEnvironment {
    public:
        // Used when the settings name no texture, and when the named one can't be loaded
        static constexpr const char* kDefaultSkyboxPath = "Assets/Skyboxes/skybox_raw.dds";

        // Creates the passes that build the cubes. Safe to call again.
        void Initialize(IRenderDevice* pDevice);

        // Rebuilds the cubes if the sky's source changed since the last call. Cheap otherwise, so call it every frame.
        // projectDir resolves texture paths stored relative to the project folder.
        // buildLighting also builds the two lighting cubes, which takes most of the time of a rebuild.
        // Returns true when the cubes were replaced, meaning their views have to be bound again.
        bool Update(IDeviceContext* pContext, const ProjectSkySettings& settings, const std::filesystem::path& projectDir, bool buildLighting);

        // nullptr until the first Update
        ITextureView* GetSkySRV() const { return GetSRV(m_pSky); }
        // nullptr when the lighting cubes were not built
        ITextureView* GetIrradianceSRV() const { return GetSRV(m_pIrradiance); }
        ITextureView* GetSpecularSRV() const { return GetSRV(m_pSpecular); }

    private:
        // One full-screen pass that draws a cubemap face
        struct CubePass {
            RefCntAutoPtr<IPipelineState> pPSO;
            RefCntAutoPtr<IShaderResourceBinding> pSRB;
        };

        enum class Source { None, Texture, SolidColor };

        static ITextureView* GetSRV(ITexture* pTexture) {
            return pTexture ? pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE) : nullptr;
        }

        CubePass CreateCubePass(const char* name, const char* pixelShader, const char* textureName, const char* constantsName,
            const SamplerDesc& sampler, IBuffer* pConstantsCB);

        // Loads the file and builds the cubes from it. Leaves the current cubes untouched and returns false if it can't.
        bool BuildFromTexture(IDeviceContext* pContext, const std::string& path, bool buildLighting);
        // Draws the six faces of pSky from the source texture, then fills its mips
        void BakeSky(IDeviceContext* pContext, ITexture* pSource, ITexture* pSky);
        // Filters pSky into the irradiance cube and every mip of the specular cube
        void FilterLighting(IDeviceContext* pContext, ITexture* pSky, ITexture* pIrradiance, ITexture* pSpecular);
        void DrawFace(IDeviceContext* pContext, const CubePass& pass, ITexture* pTarget, Uint32 face, Uint32 mip);

        // One texel per face: a flat color is its own irradiance and its own reflection at every roughness
        bool BuildSolidColor(const glm::vec3& color);

        RefCntAutoPtr<IRenderDevice> m_pDevice;

        CubePass m_BakeFromCube;
        CubePass m_BakeFromPanorama;
        CubePass m_IrradiancePass;
        CubePass m_SpecularPass;
        RefCntAutoPtr<IBuffer> m_pBakeCB;
        RefCntAutoPtr<IBuffer> m_pFilterCB;

        RefCntAutoPtr<ITexture> m_pSky;
        RefCntAutoPtr<ITexture> m_pIrradiance;
        RefCntAutoPtr<ITexture> m_pSpecular;

        // What the settings asked for at the last Update, so an unchanged or unloadable sky is not rebuilt every frame
        Source m_Source = Source::None;
        std::string m_TexturePath;
        std::filesystem::path m_ProjectDir;
        bool m_LightingRequested = false;
        glm::vec3 m_Color{ 0.0f };

        // File the texture cubes were last built from (empty for a solid color sky)
        std::string m_BuiltFromFile;
        bool m_BuiltWithLighting = false;
    };

}
