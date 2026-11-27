#pragma once

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Common/interface/RefCntAutoPtr.hpp"

namespace Diligent {

    // A vertex buffer the CPU fills again every frame (debug lines, particle instances). It starts small and grows,
    // with headroom, when a frame needs more than it holds, up to a limit.
    //
    //   m_Lines = DynamicVertexBuffer("Debug Line Vertex Buffer", sizeof(DebugLineVertex), 4096, 1u << 20);
    //   if (m_Lines.Upload(pDevice, pContext, lines.data(), count)) m_Lines.Bind(pContext);
    class DynamicVertexBuffer {
    public:
        DynamicVertexBuffer() = default;
        DynamicVertexBuffer(const char* name, Uint32 elementSize, Uint32 initialCapacity, Uint32 maxCapacity)
            : m_Name(name), m_ElementSize(elementSize), m_InitialCapacity(initialCapacity), m_MaxCapacity(maxCapacity) {}

        // Drops the buffer, e.g. when the device is set up again
        void Release();

        // Replaces the contents with count elements, growing the buffer if needed. False (nothing uploaded) when count
        // is more than the limit or the buffer could not be created or mapped.
        bool Upload(IRenderDevice* pDevice, IDeviceContext* pContext, const void* pData, Uint32 count);

        // Binds it as the vertex buffer in slot 0
        void Bind(IDeviceContext* pContext) const;

    private:
        RefCntAutoPtr<IBuffer> m_pBuffer;
        const char* m_Name = "Dynamic Vertex Buffer";
        Uint32 m_ElementSize = 0;
        Uint32 m_InitialCapacity = 0;
        Uint32 m_MaxCapacity = 0;
        Uint32 m_Capacity = 0;      // In elements
    };

}
