#pragma once

#include "Panels/BasePanel.hpp"
#include <string>

namespace Diligent {

    class UserSettingsPanel : public BasePanel
    {
    public:
        UserSettingsPanel(const std::string& name = "User Settings");
        ~UserSettingsPanel() override = default;

        // Implementation of the abstract Draw method
        void Draw() override;
    };

}