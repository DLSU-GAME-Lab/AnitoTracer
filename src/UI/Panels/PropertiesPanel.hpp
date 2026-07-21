#pragma once

#include "BasePanel.hpp"
#include "../../Objects/CameraObj.hpp"
#include "imgui.h"

namespace Diligent {

    class PropertiesPanel : public BasePanel
    {
    public:
        PropertiesPanel()
            : BasePanel("Properties"), m_pCamera(nullptr) {}

        void SetCamera(CameraObj* pCamera) { m_pCamera = pCamera; }

        void Draw() override;

    private:
        CameraObj* m_pCamera;
    };

}