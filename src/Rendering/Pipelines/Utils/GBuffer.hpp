#pragma once

#include <string>
#include <vector>

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Common/interface/RefCntAutoPtr.hpp"

namespace Diligent {

    // Same-sized color targets plus depth, written by a geometry pass and sampled by a lighting pass.
    // Describe the layout once with Configure, then call Resize whenever the view size may have changed.
    class GBuffer {
    public:
        struct Attachment {
            const char* Name;               // Texture debug name
            TEXTURE_FORMAT Format;
            const char* ShaderVariable;     // Lighting shader texture it is bound to; nullptr if it isn't sampled
        };

        // growOnly keeps the textures when the view shrinks, so one G-buffer can serve views of different sizes
        // (passes then draw into its top-left corner, see PipelineUtils::SetViewport)
        void Configure(std::vector<Attachment> colors, Attachment depth, bool growOnly = false);
        bool IsConfigured() const { return m_Depth.Format != TEX_FORMAT_UNKNOWN; }

        // Returns true when the textures were (re)created, meaning SRBs that sample them must be rebound
        bool Resize(IRenderDevice* pDevice, Uint32 width, Uint32 height);
        bool IsCreated() const { return m_pDepthTexture != nullptr; }
        void Release();

        void SetRenderTargets(IDeviceContext* pContext, bool depthOnly = false) const;
        // Depth is always cleared to 1; colors (to 0) only when requested
        void Clear(IDeviceContext* pContext, bool clearColor) const;

        // Binds every sampled attachment to its ShaderVariable in the pixel shader
        void BindToSRB(IShaderResourceBinding* pSRB) const;

        std::vector<TEXTURE_FORMAT> GetColorFormats() const;
        TEXTURE_FORMAT GetDepthFormat() const { return m_Depth.Format; }
        ITextureView* GetDSV() const;
        Uint32 GetWidth() const { return m_Width; }
        Uint32 GetHeight() const { return m_Height; }

    private:
        std::vector<Attachment> m_Colors;
        Attachment m_Depth{ nullptr, TEX_FORMAT_UNKNOWN, nullptr };
        bool m_GrowOnly = false;

        std::vector<RefCntAutoPtr<ITexture>> m_ColorTextures;
        std::vector<ITextureView*> m_RTVs;
        RefCntAutoPtr<ITexture> m_pDepthTexture;
        Uint32 m_Width = 0;
        Uint32 m_Height = 0;
    };

}
