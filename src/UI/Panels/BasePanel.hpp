#pragma once

//very important
#include "PropertyDrawers/PropertyDrawers.hpp"

#include <string>

namespace Diligent {

    class BasePanel
    {
    public:
        BasePanel(const std::string& name)
            : m_Name(name), m_IsVisible(true) {}

        virtual ~BasePanel() = default;

        // Abstract method to be implemented by derived panels
        virtual void Draw() = 0;

        const std::string& GetName() const { return m_Name; }

        bool& GetVisible() { return m_IsVisible; }
        void SetVisible(bool visible) { m_IsVisible = visible; }

        // Shows the panel and requests input focus for its next draw.
        void Open();

        // Called by GUIManager immediately before Draw.
        void PrepareDraw();

    protected:
        std::string m_Name;
        bool m_IsVisible;

    private:
        bool m_FocusRequested = false;
    };

}