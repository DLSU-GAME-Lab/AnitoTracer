#include "DynamicVertexBuffer.hpp"
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"
#include <algorithm>
#include <cstring>

namespace Diligent {

    void DynamicVertexBuffer::Release()
    {
        m_pBuffer.Release();
        m_Capacity = 0;
    }

    bool DynamicVertexBuffer::Upload(IRenderDevice* pDevice, IDeviceContext* pContext, const void* pData, Uint32 count)
    {
        if (count == 0 || count > m_MaxCapacity || !pDevice) return false;

        if (!m_pBuffer || count > m_Capacity) {
            // Grow with headroom
            Uint32 newCapacity = (std::max)(count, m_Capacity == 0 ? m_InitialCapacity : m_Capacity * 2);
            newCapacity = (std::min)(newCapacity, m_MaxCapacity);

            BufferDesc Desc;
            Desc.Name = m_Name;
            Desc.Usage = USAGE_DYNAMIC;
            Desc.BindFlags = BIND_VERTEX_BUFFER;
            Desc.CPUAccessFlags = CPU_ACCESS_WRITE;
            Desc.Size = static_cast<Uint64>(newCapacity) * m_ElementSize;

            m_pBuffer.Release();
            pDevice->CreateBuffer(Desc, nullptr, &m_pBuffer);
            m_Capacity = newCapacity;
        }
        if (!m_pBuffer) return false;

        MapHelper<Uint8> Mapped(pContext, m_pBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
        Uint8* pDst = Mapped;
        if (!pDst) return false;
        std::memcpy(pDst, pData, static_cast<size_t>(count) * m_ElementSize);
        return true;
    }

    void DynamicVertexBuffer::Bind(IDeviceContext* pContext) const
    {
        IBuffer* pBuffs[] = { m_pBuffer };
        pContext->SetVertexBuffers(0, 1, pBuffs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    }

}
