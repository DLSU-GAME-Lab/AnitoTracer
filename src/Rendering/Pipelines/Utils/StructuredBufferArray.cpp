#include "StructuredBufferArray.hpp"
#include <algorithm>

namespace Diligent {

    void StructuredBufferArray::Initialize(IRenderDevice* pDevice, const char* name, Uint32 elementSize, Uint32 initialCapacity)
    {
        m_pDevice = pDevice;
        m_Name = name;
        m_ElementSize = elementSize;
        m_Count = 0;
        m_Capacity = 0;
        m_pBuffer.Release();

        Reserve(nullptr, (std::max)(initialCapacity, 1u), false);
    }

    void StructuredBufferArray::Reserve(IDeviceContext* pContext, Uint32 capacity, bool keepContents)
    {
        if (!m_pDevice || (m_pBuffer && capacity <= m_Capacity)) return;

        // Double, so a scene that keeps growing doesn't reallocate on every addition
        const Uint32 newCapacity = (std::max)(capacity, m_Capacity * 2);

        BufferDesc Desc;
        Desc.Name = m_Name;
        Desc.Usage = USAGE_DEFAULT;
        Desc.BindFlags = BIND_SHADER_RESOURCE;
        Desc.Mode = BUFFER_MODE_STRUCTURED;
        Desc.ElementByteStride = m_ElementSize;
        Desc.Size = static_cast<Uint64>(newCapacity) * m_ElementSize;

        RefCntAutoPtr<IBuffer> pNewBuffer;
        m_pDevice->CreateBuffer(Desc, nullptr, &pNewBuffer);
        if (!pNewBuffer) return;

        if (keepContents && m_pBuffer && m_Count > 0 && pContext) {
            pContext->CopyBuffer(m_pBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                pNewBuffer, 0, static_cast<Uint64>(m_Count) * m_ElementSize, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        }

        m_pBuffer = pNewBuffer;
        m_Capacity = newCapacity;
    }

    void StructuredBufferArray::Assign(IDeviceContext* pContext, const void* pData, Uint32 count)
    {
        m_Count = 0;
        Append(pContext, pData, count);
    }

    Uint32 StructuredBufferArray::Append(IDeviceContext* pContext, const void* pData, Uint32 count)
    {
        const Uint32 first = m_Count;
        if (count == 0 || !pData || !pContext) return first;

        Reserve(pContext, m_Count + count, true);
        if (!m_pBuffer || m_Count + count > m_Capacity) return first;

        pContext->UpdateBuffer(m_pBuffer, static_cast<Uint64>(first) * m_ElementSize, static_cast<Uint64>(count) * m_ElementSize,
            pData, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_Count += count;
        return first;
    }

}
