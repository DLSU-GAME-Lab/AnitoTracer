#pragma once
#include "Panels/BasePanel.hpp"
#include "Graphics/GraphicsEngine/interface/TextureView.h"
#include <functional>
#include <string>

namespace Diligent {
    class ViewportPanel : public BasePanel {
    public:
        using SRVGetter = std::function<ITextureView* ()>;

        ViewportPanel(const std::string& name, SRVGetter srvGetter, bool drawGizmos = false);
        ~ViewportPanel() override = default;

        void Draw() override;
    private:
        SRVGetter m_GetSRV;
        bool m_DrawGizmos;
    };
}