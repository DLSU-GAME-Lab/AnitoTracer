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
                // Position
                LayoutElement{0, 0, 3, VT_FLOAT32, False},
                //Norm
                LayoutElement{1, 0, 3, VT_FLOAT32, False},
                // UV
                LayoutElement{2, 0, 2, VT_FLOAT32, False}
            };
        }

        // You can add more layouts here later, for example:
        // static std::vector<LayoutElement> GetPositionOnlyLayout() { ... }
    };

}