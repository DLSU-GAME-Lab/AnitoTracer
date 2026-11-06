#pragma once

#include <string>

namespace Diligent::ScriptEditor::Utf8
{
    inline bool IsContinuationByte(char byte) { return (static_cast<unsigned char>(byte) & 0xC0) == 0x80; }

    /// Returns the byte offset of the code point following the one at `index`.
    inline int NextBoundary(const std::string& text, int index)
    {
        const int size = static_cast<int>(text.size());
        if (index >= size) return size;
        ++index;
        while (index < size && IsContinuationByte(text[index])) ++index;
        return index;
    }

    /// Returns the byte offset of the code point preceding `index`.
    inline int PreviousBoundary(const std::string& text, int index)
    {
        if (index <= 0) return 0;
        --index;
        while (index > 0 && IsContinuationByte(text[index])) --index;
        return index;
    }

    /// Appends a Unicode code point to `out` encoded as UTF-8.
    inline void Append(std::string& out, unsigned int codepoint)
    {
        if (codepoint < 0x80)
            out += static_cast<char>(codepoint);
        else if (codepoint < 0x800)
        {
            out += static_cast<char>(0xC0 | (codepoint >> 6));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
        else if (codepoint < 0x10000)
        {
            out += static_cast<char>(0xE0 | (codepoint >> 12));
            out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
        else
        {
            out += static_cast<char>(0xF0 | (codepoint >> 18));
            out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
    }

    /// Number of code points in the first `byteCount` bytes of `text`.
    inline int CountCodepoints(const std::string& text, int byteCount)
    {
        int count = 0;
        for (int i = 0; i < byteCount && i < static_cast<int>(text.size()); ++i)
            if (!IsContinuationByte(text[i])) ++count;
        return count;
    }
}
