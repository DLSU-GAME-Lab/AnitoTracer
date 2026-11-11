#pragma once

#include "TextTypes.hpp"

#include <string>
#include <vector>

namespace Diligent::ScriptEditor
{
    /// Line-based text storage. Knows nothing about carets, undo or rendering:
    /// it only supports inserting and erasing text at positions.
    /// Internally lines never contain '\r' or '\n'; the original line ending style is remembered for saving.
    class TextBuffer
    {
    public:
        TextBuffer() { m_Lines.emplace_back(); }

        /// Replaces the whole content, detecting CRLF vs LF line endings.
        void SetText(const std::string& text);
        /// Serializes the content using the detected line ending.
        std::string GetText() const;

        int LineCount() const { return static_cast<int>(m_Lines.size()); }
        const std::string& Line(int index) const { return m_Lines[index]; }

        /// Clamps a position into the buffer (valid line, column within the line).
        TextPosition Clamp(TextPosition position) const;
        TextPosition EndPosition() const;

        /// Text inside a range, lines joined with '\n'.
        std::string GetText(TextRange range) const;

        /// Inserts text (any '\r' is dropped) and returns the position right after it.
        TextPosition Insert(TextPosition position, const std::string& text);
        /// Removes a range and returns the removed text.
        std::string Erase(TextRange range);

        /// Where text would end if it were inserted at `start`. Does not modify any buffer.
        static TextPosition EndOfText(TextPosition start, const std::string& text);

    private:
        std::vector<std::string> m_Lines;
        bool m_UsesCrlf = false;
    };
}
