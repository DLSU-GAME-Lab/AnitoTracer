#pragma once

#include <string>

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "../Utils/PipelineUtils.hpp"

struct ProjectSettings;

namespace Diligent {

    class RenderData;

    // Fullscreen effect in the PostProcessStack. The previous stage renders into this effect's intermediate texture;
    // Apply samples it as g_SceneColor (with the cbuffer named in Desc::ConstantsName bound) and writes into the next stage.
    //
    // New effects usually derive from PostProcessEffect<TConstants> below and only implement Prepare.
    class PostProcessPipeline {
    public:
        struct Desc {
            const char* Name;               // Used for the PSO and intermediate texture names
            const char* PixelShader;        // Uses Shaders/Deferred/fullscreen_vs.hlsl as its vertex shader
            const char* ConstantsName;      // cbuffer name in the pixel shader
            Uint32 ConstantsSize;
            SamplerDesc SceneSampler;       // Immutable sampler for g_SceneColor
        };

        explicit PostProcessPipeline(const Desc& desc);
        virtual ~PostProcessPipeline() = default;

        void Initialize(IRenderDevice* pDevice, TEXTURE_FORMAT RTVFormat);

        // Reads the effect's settings and builds this frame's constants. Returning false skips the effect for this view.
        virtual bool Prepare(const ProjectSettings& settings, const RenderData& renderData, Uint32 width, Uint32 height) = 0;

        // Intermediate render target matching the final target, (re)created on resize
        ITextureView* GetIntermediateRTV(Uint32 width, Uint32 height);

        // Samples the intermediate texture, applies the effect and writes the result to pDstRTV
        void Apply(IDeviceContext* pContext, ITextureView* pDstRTV);

        const char* GetName() const { return m_Desc.Name; }

    protected:
        // Uploads the constants built by Prepare
        virtual void WriteConstants(IDeviceContext* pContext, IBuffer* pConstantsCB) = 0;

    private:
        Desc m_Desc;
        RefCntAutoPtr<IRenderDevice> m_pDevice;
        RefCntAutoPtr<IPipelineState> m_pPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
        RefCntAutoPtr<IBuffer> m_pConstantsCB;

        RefCntAutoPtr<ITexture> m_pIntermediate;
        TEXTURE_FORMAT m_Format = TEX_FORMAT_UNKNOWN;
    };

    // PostProcessPipeline whose constants are a single struct mirroring the shader's cbuffer; Prepare fills m_Constants
    template <typename TConstants>
    class PostProcessEffect : public PostProcessPipeline {
        static_assert(sizeof(TConstants) % 16 == 0, "Post process constants must be 16-byte aligned for HLSL");

    protected:
        PostProcessEffect(const char* name, const char* pixelShader, const char* constantsName, const SamplerDesc& sceneSampler)
            : PostProcessPipeline({ name, pixelShader, constantsName, static_cast<Uint32>(sizeof(TConstants)), sceneSampler }) {}

        void WriteConstants(IDeviceContext* pContext, IBuffer* pConstantsCB) override {
            PipelineUtils::WriteCB(pContext, pConstantsCB, m_Constants);
        }

        TConstants m_Constants{};
    };

}
