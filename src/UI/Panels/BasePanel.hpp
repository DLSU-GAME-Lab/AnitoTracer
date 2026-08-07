#pragma once

//very important
#include "PropertyDrawers.hpp"

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

    protected:
        std::string m_Name;
        bool m_IsVisible;
    };

}