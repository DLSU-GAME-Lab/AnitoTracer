#pragma once

#include "Graphics/GraphicsEngine/interface/InputLayout.h"
#include <vector>

namespace Diligent {

    class VertexLayouts {
    public:
        /// <summary>
        /// Standard layout matching the common VertexInput (Pos, Norm, UV)
        /// </summary>
        static std::vector<LayoutElement> GetStandardLayout() {
            return {
                // Position (float4)
                LayoutElement{0, 0, 4, VT_FLOAT32, False},
                // Norm (float4)
                LayoutElement{1, 0, 4, VT_FLOAT32, False},
                // UV (float4 padded)
                LayoutElement{2, 0, 4, VT_FLOAT32, False},
                // Tangent (float4)  
                LayoutElement{3, 0, 4, VT_FLOAT32, False},
                // Bitangent (float4)
                LayoutElement{4, 0, 4, VT_FLOAT32, False}
            };
        }

        // You can add more layouts here later, for example:
        // static std::vector<LayoutElement> GetPositionOnlyLayout() { ... }
    };

}