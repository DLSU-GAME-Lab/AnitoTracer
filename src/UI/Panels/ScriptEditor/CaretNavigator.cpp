#include "CaretNavigator.hpp"

#include "Utf8.hpp"

#include <algorithm>
#include <cctype>

namespace Diligent::ScriptEditor::CaretNavigator
{
    namespace
    {
        enum class CharClass { Space, Word, Punctuation };

        CharClass Classify(char c)
        {
            const unsigned char u = static_cast<unsigned char>(c);
            if (u == ' ' || u == '\t') return CharClass::Space;
            if (u >= 0x80 || u == '_' || std::isalnum(u)) return CharClass::Word;
            return CharClass::Punctuation;
        }

        int FirstNonBlank(const std::string& line)
        {
            int i = 0;
            while (i < static_cast<int>(line.size()) && Classify(line[i]) == CharClass::Space) ++i;
            return i;
        }

        TextPosition Left(const TextBuffer& buffer, TextPosition p)
        {
            if (p.column > 0) return { p.line, Utf8::PreviousBoundary(buffer.Line(p.line), p.column) };
            if (p.line > 0) return { p.line - 1, static_cast<int>(buffer.Line(p.line - 1).size()) };
            return p;
        }

        TextPosition Right(const TextBuffer& buffer, TextPosition p)
        {
            const std::string& line = buffer.Line(p.line);
            if (p.column < static_cast<int>(line.size())) return { p.line, Utf8::NextBoundary(line, p.column) };
            if (p.line + 1 < buffer.LineCount()) return { p.line + 1, 0 };
            return p;
        }

        TextPosition WordLeft(const TextBuffer& buffer, TextPosition p)
        {
            if (p.column == 0) return Left(buffer, p);
            const std::string& line = buffer.Line(p.line);
            int i = p.column;
            while (i > 0 && Classify(line[i - 1]) == CharClass::Space) --i;
            if (i > 0)
            {
                const CharClass cls = Classify(line[i - 1]);
                while (i > 0 && Classify(line[i - 1]) == cls) --i;
            }
            return { p.line, i };
        }

        TextPosition WordRight(const TextBuffer& buffer, TextPosition p)
        {
            const std::string& line = buffer.Line(p.line);
            const int size = static_cast<int>(line.size());
            if (p.column >= size) return Right(buffer, p);
            int i = p.column;
            const CharClass cls = Classify(line[i]);
            if (cls != CharClass::Space)
                while (i < size && Classify(line[i]) == cls) ++i;
            while (i < size && Classify(line[i]) == CharClass::Space) ++i;
            return { p.line, i };
        }

        TextPosition Vertical(const TextBuffer& buffer, TextPosition p, int lineDelta, int& preferredColumn)
        {
            if (preferredColumn < 0) preferredColumn = p.column;
            const int target = p.line + lineDelta;
            if (target < 0) return { 0, 0 };
            if (target >= buffer.LineCount()) return buffer.EndPosition();

            const std::string& line = buffer.Line(target);
            int column = std::min(preferredColumn, static_cast<int>(line.size()));
            while (column > 0 && column < static_cast<int>(line.size()) && Utf8::IsContinuationByte(line[column])) --column;
            return { target, column };
        }
    }

    TextPosition Move(const TextBuffer& buffer, TextPosition from, Movement movement, int pageLines, int& preferredColumn)
    {
        from = buffer.Clamp(from);
        const bool vertical = movement == Movement::Up || movement == Movement::Down ||
                              movement == Movement::PageUp || movement == Movement::PageDown;
        if (!vertical) preferredColumn = -1;

        switch (movement)
        {
        case Movement::Left:          return Left(buffer, from);
        case Movement::Right:         return Right(buffer, from);
        case Movement::Up:            return Vertical(buffer, from, -1, preferredColumn);
        case Movement::Down:          return Vertical(buffer, from, 1, preferredColumn);
        case Movement::PageUp:        return Vertical(buffer, from, -std::max(1, pageLines), preferredColumn);
        case Movement::PageDown:      return Vertical(buffer, from, std::max(1, pageLines), preferredColumn);
        case Movement::WordLeft:      return WordLeft(buffer, from);
        case Movement::WordRight:     return WordRight(buffer, from);
        case Movement::LineEnd:       return { from.line, static_cast<int>(buffer.Line(from.line).size()) };
        case Movement::DocumentStart: return { 0, 0 };
        case Movement::DocumentEnd:   return buffer.EndPosition();
        case Movement::LineStart:
        {
            const int indent = FirstNonBlank(buffer.Line(from.line));
            return { from.line, from.column == indent ? 0 : indent };
        }
        }
        return from;
    }

    TextRange WordRangeAt(const TextBuffer& buffer, TextPosition position)
    {
        position = buffer.Clamp(position);
        const std::string& line = buffer.Line(position.line);
        if (line.empty()) return { position, position };

        // Prefer the character under the caret; fall back to the one before it at end of line.
        int index = std::min(position.column, static_cast<int>(line.size()) - 1);
        if (position.column >= static_cast<int>(line.size())) index = static_cast<int>(line.size()) - 1;

        const CharClass cls = Classify(line[index]);
        int begin = index;
        int end = index + 1;
        while (begin > 0 && Classify(line[begin - 1]) == cls) --begin;
        while (end < static_cast<int>(line.size()) && Classify(line[end]) == cls) ++end;
        return { { position.line, begin }, { position.line, end } };
    }
}
