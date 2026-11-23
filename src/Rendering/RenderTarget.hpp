#pragma once
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/Texture.h"
#include "Graphics/GraphicsEngine/interface/TextureView.h"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "FrameTarget.hpp"
#include <string>

namespace Diligent {

    // Off-screen color + depth pair. Single sampled targets can be sampled (GetSRV), e.g. to show a view in ImGui,
    // and so can their depth (e.g. for TAA reprojection).
    class RenderTarget {
    public:
        explicit RenderTarget(const std::string& name) : m_Name(name) {}

        // (Re)creates the textures only when the size, formats or sample count changed, so it is cheap to call every frame
        void Create(IRenderDevice* pDevice, uint32_t width, uint32_t height, TEXTURE_FORMAT colorFmt, TEXTURE_FORMAT depthFmt, uint8_t sampleCount = 1);
        void Release();

        bool IsValid() const { return m_pRTV && m_pDSV; }

        ITexture* GetColorTexture() const { return m_pColorTexture; }
        ITextureView* GetRTV() const { return m_pRTV; }
        ITextureView* GetDSV() const { return m_pDSV; }
        ITextureView* GetSRV() const { return m_pSRV; } // nullptr when multisampled

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }
        uint8_t GetSampleCount() const { return m_SampleCount; }
        const std::string& GetName() const { return m_Name; }

        FrameTarget AsFrameTarget() const { return { m_pRTV, m_pDSV, m_Width, m_Height }; }

    private:
        std::string m_Name;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        TEXTURE_FORMAT m_ColorFormat = TEX_FORMAT_UNKNOWN;
        TEXTURE_FORMAT m_DepthFormat = TEX_FORMAT_UNKNOWN;
        uint8_t m_SampleCount = 1;

        RefCntAutoPtr<ITexture> m_pColorTexture;
        RefCntAutoPtr<ITexture> m_pDepthTexture;
        RefCntAutoPtr<ITextureView> m_pRTV;
        RefCntAutoPtr<ITextureView> m_pSRV;
        RefCntAutoPtr<ITextureView> m_pDSV;
    };

}
