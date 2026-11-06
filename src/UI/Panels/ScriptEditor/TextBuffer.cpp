#include "TextBuffer.hpp"

#include <algorithm>

namespace Diligent::ScriptEditor
{
    namespace
    {
        std::vector<std::string> SplitLines(const std::string& text)
        {
            std::vector<std::string> parts(1);
            for (const char c : text)
            {
                if (c == '\r') continue;
                if (c == '\n') parts.emplace_back();
                else parts.back() += c;
            }
            return parts;
        }
    }

    void TextBuffer::SetText(const std::string& text)
    {
        m_UsesCrlf = text.find("\r\n") != std::string::npos;
        m_Lines = SplitLines(text);
    }

    std::string TextBuffer::GetText() const
    {
        const char* separator = m_UsesCrlf ? "\r\n" : "\n";
        std::string result;
        for (size_t i = 0; i < m_Lines.size(); ++i)
        {
            if (i > 0) result += separator;
            result += m_Lines[i];
        }
        return result;
    }

    TextPosition TextBuffer::Clamp(TextPosition position) const
    {
        position.line = std::clamp(position.line, 0, LineCount() - 1);
        position.column = std::clamp(position.column, 0, static_cast<int>(m_Lines[position.line].size()));
        return position;
    }

    TextPosition TextBuffer::EndPosition() const
    {
        return { LineCount() - 1, static_cast<int>(m_Lines.back().size()) };
    }

    std::string TextBuffer::GetText(TextRange range) const
    {
        range.begin = Clamp(range.begin);
        range.end = Clamp(range.end);
        if (range.begin.line == range.end.line)
            return m_Lines[range.begin.line].substr(range.begin.column, range.end.column - range.begin.column);

        std::string result = m_Lines[range.begin.line].substr(range.begin.column);
        for (int line = range.begin.line + 1; line < range.end.line; ++line)
            result += '\n' + m_Lines[line];
        result += '\n' + m_Lines[range.end.line].substr(0, range.end.column);
        return result;
    }

    TextPosition TextBuffer::Insert(TextPosition position, const std::string& text)
    {
        position = Clamp(position);
        std::vector<std::string> parts = SplitLines(text);

        std::string& first = m_Lines[position.line];
        const std::string tail = first.substr(position.column);
        first.resize(position.column);
        first += parts.front();

        TextPosition end;
        if (parts.size() == 1)
        {
            end = { position.line, static_cast<int>(first.size()) };
            first += tail;
        }
        else
        {
            m_Lines.insert(m_Lines.begin() + position.line + 1, parts.begin() + 1, parts.end());
            std::string& last = m_Lines[position.line + parts.size() - 1];
            end = { position.line + static_cast<int>(parts.size()) - 1, static_cast<int>(last.size()) };
            last += tail;
        }
        return end;
    }

    std::string TextBuffer::Erase(TextRange range)
    {
        range.begin = Clamp(range.begin);
        range.end = Clamp(range.end);
        if (range.begin >= range.end) return {};

        std::string removed = GetText(range);
        if (range.begin.line == range.end.line)
        {
            m_Lines[range.begin.line].erase(range.begin.column, range.end.column - range.begin.column);
        }
        else
        {
            std::string& first = m_Lines[range.begin.line];
            first.resize(range.begin.column);
            first += m_Lines[range.end.line].substr(range.end.column);
            m_Lines.erase(m_Lines.begin() + range.begin.line + 1, m_Lines.begin() + range.end.line + 1);
        }
        return removed;
    }

    TextPosition TextBuffer::EndOfText(TextPosition start, const std::string& text)
    {
        TextPosition end = start;
        for (const char c : text)
        {
            if (c == '\r') continue;
            if (c == '\n') { ++end.line; end.column = 0; }
            else ++end.column;
        }
        return end;
    }
}
