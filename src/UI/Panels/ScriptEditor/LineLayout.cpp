#include "LineLayout.hpp"

#include "Utf8.hpp"

#include "imgui.h"

#include <algorithm>

namespace Diligent::ScriptEditor
{
    float MeasureText(const char* begin, const char* end)
    {
        // ImGui::CalcTextSize rounds up to whole pixels, which accumulates when measuring glyph by glyph.
        return ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0.0f, begin, end).x;
    }

    LineLayout::LineLayout(const std::string& line, int tabSize, float spaceWidth)
    {
        float x = 0.0f;
        int displayColumn = 0;
        int index = 0;
        const int size = static_cast<int>(line.size());
        while (index < size)
        {
            m_Offsets.push_back(index);
            m_Xs.push_back(x);

            const int next = Utf8::NextBoundary(line, index);
            if (line[index] == '\t')
                x += static_cast<float>(tabSize - displayColumn % tabSize) * spaceWidth;
            else
                x += MeasureText(line.c_str() + index, line.c_str() + next);

            // A tab advances the display column to the next stop; anything else by one.
            displayColumn = line[index] == '\t' ? displayColumn + (tabSize - displayColumn % tabSize) : displayColumn + 1;
            index = next;
        }
        m_Offsets.push_back(size);
        m_Xs.push_back(x);
    }

    float LineLayout::ColumnToX(int column) const
    {
        const auto it = std::lower_bound(m_Offsets.begin(), m_Offsets.end(), column);
        return m_Xs[static_cast<size_t>(it - m_Offsets.begin())];
    }

    int LineLayout::XToColumn(float x) const
    {
        for (size_t i = 0; i + 1 < m_Xs.size(); ++i)
            if (x < (m_Xs[i] + m_Xs[i + 1]) * 0.5f)
                return m_Offsets[i];
        return m_Offsets.back();
    }
}
