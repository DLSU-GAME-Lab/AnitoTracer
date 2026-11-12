#pragma once

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Common/interface/RefCntAutoPtr.hpp"

namespace Diligent {

    // An array of structs on the GPU that shaders read as StructuredBuffer<T>, and that grows when it runs out of room.
    // Growing creates a new buffer, so whoever binds it must fetch GetSRV() again afterwards (bind it every frame).
    //
    //   m_Records.Initialize(pDevice, "Scene Geometry Records", sizeof(Record));
    //   m_Records.Assign(pContext, records.data(), records.size());      // replace everything, every frame
    //   m_Vertices.Append(pContext, vertices.data(), vertices.size());   // or add to what is already there
    //
    // Assign and Append copy on the GPU, so call them outside a render pass (before the frame's targets are bound).
    class StructuredBufferArray {
    public:
        // Creates the buffer right away, so there is always something valid to bind, even while it is empty
        void Initialize(IRenderDevice* pDevice, const char* name, Uint32 elementSize, Uint32 initialCapacity = 64);
        bool IsInitialized() const { return m_pBuffer != nullptr; }

        // Replaces the contents with count elements. Previous contents are dropped.
        void Assign(IDeviceContext* pContext, const void* pData, Uint32 count);
        // Adds count elements after the existing ones and returns the index of the first one added
        Uint32 Append(IDeviceContext* pContext, const void* pData, Uint32 count);
        // Forgets the contents; the buffer and its capacity stay
        void Clear() { m_Count = 0; }

        Uint32 GetCount() const { return m_Count; }
        IBufferView* GetSRV() const { return m_pBuffer ? m_pBuffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE) : nullptr; }

    private:
        // Makes room for at least `capacity` elements. keepContents copies the current elements into the new buffer.
        void Reserve(IDeviceContext* pContext, Uint32 capacity, bool keepContents);

        RefCntAutoPtr<IRenderDevice> m_pDevice;
        RefCntAutoPtr<IBuffer> m_pBuffer;
        const char* m_Name = "Structured Buffer Array";
        Uint32 m_ElementSize = 0;
        Uint32 m_Capacity = 0;
        Uint32 m_Count = 0;
    };

}
