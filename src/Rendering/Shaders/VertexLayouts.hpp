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
                LayoutElement{2, 0, 2, VT_FLOAT32, False},
                // Tangent  
                LayoutElement{3, 0, 3, VT_FLOAT32, False},
                // Bitangent
                LayoutElement{4, 0, 3, VT_FLOAT32, False}
            };
        }

        /// <summary>
        /// The standard layout plus a second stream for the Anime renderer (AnimeVertexInput): the smooth normal and
        /// curvature of each vertex as a float4 (Model::pAnimeVertexBuffer)
        /// </summary>
        static std::vector<LayoutElement> GetAnimeLayout() {
            std::vector<LayoutElement> layout = GetStandardLayout();
            layout.push_back(LayoutElement{5, 1, 4, VT_FLOAT32, False});
            return layout;
        }

        static std::vector<LayoutElement> GetDebugLineLayout() {
            return {
                // Position
                LayoutElement{0, 0, 3, VT_FLOAT32, False},
                // Color (RGBA)
                LayoutElement{1, 0, 4, VT_FLOAT32, False}
            };
        }

        // You can add more layouts here later, for example:
        // static std::vector<LayoutElement> GetPositionOnlyLayout() { ... }
    };

}