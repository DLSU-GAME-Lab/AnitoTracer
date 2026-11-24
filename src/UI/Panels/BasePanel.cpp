#include "BasePanel.hpp"

#include <imgui.h>

namespace Diligent
{
    void BasePanel::Open()
    {
        m_IsVisible = true;
        m_FocusRequested = true;
    }

    void BasePanel::PrepareDraw()
    {
        if (!m_FocusRequested)
            return;

        ImGui::SetNextWindowFocus();
        m_FocusRequested = false;
    }
}