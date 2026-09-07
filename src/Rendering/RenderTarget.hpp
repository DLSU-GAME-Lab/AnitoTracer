#pragma once
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/Texture.h"
#include "Graphics/GraphicsEngine/interface/TextureView.h"
#include "Common/interface/RefCntAutoPtr.hpp"
#include <string>

namespace Diligent {

    class RenderTarget {
    public:
        RenderTarget(const std::string& name) : m_Name(name) {}

        void Create(IRenderDevice* pDevice, uint32_t width, uint32_t height, TEXTURE_FORMAT colorFmt, TEXTURE_FORMAT depthFmt, uint8_t sampleCount = 1);
        void Release();

        ITextureView* GetRTV() const { return m_pRTV; }
        ITextureView* GetDSV() const { return m_pDSV; }
        ITextureView* GetSRV() const { return m_pSRV; }

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }
        const std::string& GetName() const { return m_Name; }

    private:
        std::string m_Name;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;

        RefCntAutoPtr<ITexture> m_pColorTexture;
        RefCntAutoPtr<ITexture> m_pDepthTexture;
        RefCntAutoPtr<ITextureView> m_pRTV;
        RefCntAutoPtr<ITextureView> m_pSRV;
        RefCntAutoPtr<ITextureView> m_pDSV;
    };

}