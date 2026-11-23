#pragma once

#include "TextBuffer.hpp"
#include "TextTypes.hpp"

namespace Diligent::ScriptEditor
{
    /// Ways a caret can travel through a buffer.
    enum class Movement
    {
        Left,
        Right,
        Up,
        Down,
        WordLeft,
        WordRight,
        LineStart,  ///< Smart home: toggles between first non-blank character and column 0.
        LineEnd,
        DocumentStart,
        DocumentEnd,
        PageUp,
        PageDown
    };

    /// Pure caret movement logic; stateless apart from the sticky column passed in by the caller.
    namespace CaretNavigator
    {
        /// Computes where `from` ends up after `movement`.
        /// `preferredColumn` is the sticky column used by vertical moves (-1 = unset); it is
        /// updated here: kept by vertical moves, reset by everything else.
        TextPosition Move(const TextBuffer& buffer, TextPosition from, Movement movement, int pageLines, int& preferredColumn);

        /// Range of the word (or whitespace/punctuation run) around `position`.
        TextRange WordRangeAt(const TextBuffer& buffer, TextPosition position);
    }
}
