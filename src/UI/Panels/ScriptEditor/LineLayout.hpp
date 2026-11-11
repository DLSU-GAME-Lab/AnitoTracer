#pragma once

#include <string>
#include <vector>

namespace Diligent::ScriptEditor
{
    /// Exact (unrounded) width of [begin, end) in the current font.
    float MeasureText(const char* begin, const char* end);

    /// Horizontal metrics of one line as rendered (tabs expanded to tab stops).
    /// `offsets[i]` is the byte offset of the i-th code point boundary and `xs[i]` its x position;
    /// the last entry is the end of the line.
    class LineLayout
    {
    public:
        LineLayout(const std::string& line, int tabSize, float spaceWidth);

        float GetWidth() const { return m_Xs.back(); }
        /// X position (relative to the line start) of a byte column.
        float ColumnToX(int column) const;
        /// Byte column whose boundary is nearest to x.
        int XToColumn(float x) const;

        const std::vector<int>& GetOffsets() const { return m_Offsets; }
        const std::vector<float>& GetXs() const { return m_Xs; }

    private:
        std::vector<int> m_Offsets;
        std::vector<float> m_Xs;
    };
}
