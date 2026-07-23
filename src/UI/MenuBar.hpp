#pragma once

#include <vector>
#include <memory>
#include "Panels/BasePanel.hpp"
#include "../Objects/ObjectFactory.hpp"
#include "imgui.h"

#include "FileDialog.hpp"

namespace Diligent {

    class MenuBar
    {
    public:
        // Draws the main menu bar. Requires application state and registered panels.
        void Draw(bool& appRunning, const std::vector<std::unique_ptr<BasePanel>>& panels);
    };

}