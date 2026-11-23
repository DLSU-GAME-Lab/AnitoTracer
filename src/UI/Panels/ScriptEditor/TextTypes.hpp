#pragma once

#include <algorithm>
#include <compare>

namespace Diligent::ScriptEditor
{
    /// A location inside a text buffer. `column` is a byte offset into the line
    /// and always sits on a UTF-8 code point boundary.
    struct TextPosition
    {
        int line = 0;
        int column = 0;

        auto operator<=>(const TextPosition&) const = default;
    };

    /// A half-open span [begin, end) of text. Callers are expected to pass normalized ranges (begin <= end).
    struct TextRange
    {
        TextPosition begin;
        TextPosition end;

        bool Empty() const { return begin == end; }
    };

    /// A caret plus the anchor it was extended from. When anchor == caret nothing is selected.
    struct TextSelection
    {
        TextPosition anchor;
        TextPosition caret;

        bool HasSelection() const { return anchor != caret; }
        TextRange Range() const { return { std::min(anchor, caret), std::max(anchor, caret) }; }
        void Collapse(TextPosition position) { anchor = caret = position; }
    };
}
