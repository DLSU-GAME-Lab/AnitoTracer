#pragma once

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/TopLevelAS.h"
#include "Common/interface/RefCntAutoPtr.hpp"

namespace Diligent {

    class RenderData;

    // Top-level acceleration structure over the scene's opaque models, rebuilt every frame for ray-traced pipelines
    class SceneTLAS {
    public:
        void Initialize(IRenderDevice* pDevice, Uint32 maxInstances = 1000);
        bool IsInitialized() const { return m_pTLAS != nullptr; }

        // Builds missing BLASes, then rebuilds the TLAS from renderData.Models
        void Build(IDeviceContext* pContext, const RenderData& renderData);

        ITopLevelAS* Get() const { return m_pTLAS; }

    private:
        void EnsureInstanceBufferSize(Uint64 requiredSize);

        RefCntAutoPtr<IRenderDevice> m_pDevice;
        RefCntAutoPtr<ITopLevelAS> m_pTLAS;
        RefCntAutoPtr<IBuffer> m_pScratchBuffer;
        RefCntAutoPtr<IBuffer> m_pInstanceBuffer;
    };

}
