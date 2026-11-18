#pragma once

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/TopLevelAS.h"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "StructuredBufferArray.hpp"

namespace Diligent {

    class RenderData;

    // Top-level acceleration structure over the scene's opaque models, rebuilt every frame for ray-traced pipelines.
    //
    // With geometry records on, it also writes the table that lets a ray shade what it hits (Shaders/rt_scene.hlsli):
    // one SceneGeometryRecord per (instance, BLAS geometry), holding where the triangles are in SceneGeometryPool and
    // the material, with the instance's overrides applied. Each instance's custom ID is the index of its first record,
    // so the shader finds a hit's record at InstanceID + GeometryIndex.
    class SceneTLAS {
    public:
        // geometryRecords needs SceneGeometryPool to be enabled; without it only visibility can be traced
        void Initialize(IRenderDevice* pDevice, Uint32 maxInstances = 1000, bool geometryRecords = false);
        bool IsInitialized() const { return m_pTLAS != nullptr; }

        // Builds missing BLASes, then rebuilds the TLAS (and the geometry records) from renderData.Models
        void Build(IDeviceContext* pContext, const RenderData& renderData);

        ITopLevelAS* Get() const { return m_pTLAS; }

        // This frame's records, or nullptr when they are off. The buffer can be replaced by Build, so bind it every frame.
        IBufferView* GetGeometryRecordsSRV() const { return m_GeometryRecords.GetSRV(); }

    private:
        void EnsureInstanceBufferSize(Uint64 requiredSize);

        RefCntAutoPtr<IRenderDevice> m_pDevice;
        RefCntAutoPtr<ITopLevelAS> m_pTLAS;
        RefCntAutoPtr<IBuffer> m_pScratchBuffer;
        RefCntAutoPtr<IBuffer> m_pInstanceBuffer;

        bool m_WriteGeometryRecords = false;
        StructuredBufferArray m_GeometryRecords;
    };

}
