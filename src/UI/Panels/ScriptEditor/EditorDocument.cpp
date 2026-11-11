#include "EditorDocument.hpp"

#include "Utf8.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace Diligent::ScriptEditor
{
    bool EditorDocument::Load(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (file.bad()) return false;
        m_HasUtf8Bom = content.size() >= 3 && content.compare(0, 3, "\xEF\xBB\xBF") == 0;
        if (m_HasUtf8Bom) content.erase(0, 3);

        m_Path = path;
        m_Buffer.SetText(content);
        m_Selection = {};
        m_History.Clear();
        m_SavedStateId = m_History.CurrentStateId();
        m_PreferredColumn = -1;
        ++m_Revision;
        return true;
    }

    bool EditorDocument::Save()
    {
        if (m_Path.empty()) return false;

        std::ofstream file(m_Path, std::ios::binary | std::ios::trunc);
        if (!file) return false;
        if (m_HasUtf8Bom) file << "\xEF\xBB\xBF";
        const std::string content = m_Buffer.GetText();
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        file.flush();
        if (!file) return false;
        file.close();
        if (!file) return false;

        m_SavedStateId = m_History.CurrentStateId();
        m_History.Seal();
        return true;
    }

    void EditorDocument::SetSelection(TextPosition anchor, TextPosition caret)
    {
        if (m_Selection.anchor != anchor || m_Selection.caret != caret)
            m_History.Seal();
        m_Selection = { m_Buffer.Clamp(anchor), m_Buffer.Clamp(caret) };
        m_PreferredColumn = -1;
    }

    void EditorDocument::SelectAll()
    {
        SetSelection({ 0, 0 }, m_Buffer.EndPosition());
    }

    void EditorDocument::SelectRange(const TextRange& range)
    {
        SetSelection(range.begin, range.end);
    }

    void EditorDocument::MoveCaret(Movement movement, bool extend, int pageLines)
    {
        m_History.Seal();
        // Without extend, a horizontal move with a selection collapses to that edge like most editors do.
        if (!extend && m_Selection.HasSelection() && (movement == Movement::Left || movement == Movement::Right))
        {
            const TextRange range = m_Selection.Range();
            m_Selection.Collapse(movement == Movement::Left ? range.begin : range.end);
            m_PreferredColumn = -1;
            return;
        }

        const TextPosition target = CaretNavigator::Move(m_Buffer, m_Selection.caret, movement, pageLines, m_PreferredColumn);
        m_Selection.caret = target;
        if (!extend) m_Selection.anchor = target;
    }

    std::string EditorDocument::GetSelectedText() const
    {
        return m_Selection.HasSelection() ? m_Buffer.GetText(m_Selection.Range()) : std::string{};
    }

    TextRange EditorDocument::GetCurrentLineRange() const
    {
        const int line = m_Selection.caret.line;
        const TextPosition begin{ line, 0 };
        if (line + 1 < m_Buffer.LineCount()) return { begin, { line + 1, 0 } };
        return { begin, m_Buffer.EndPosition() };
    }

    void EditorDocument::Replace(TextRange range, const std::string& text, const TextSelection& selectionAfter, EditGroup group)
    {
        range.begin = m_Buffer.Clamp(range.begin);
        range.end = m_Buffer.Clamp(range.end);

        Transaction transaction;
        transaction.selectionBefore = m_Selection;
        transaction.group = group;

        TextEdit edit;
        edit.position = range.begin;
        edit.removed = m_Buffer.Erase(range);
        edit.inserted = text;
        m_Buffer.Insert(range.begin, text);
        transaction.edits.push_back(std::move(edit));

        m_Selection = selectionAfter;
        transaction.selectionAfter = selectionAfter;
        m_History.Record(std::move(transaction));
        m_PreferredColumn = -1;
        ++m_Revision;
    }

    void EditorDocument::ReplaceSelection(const std::string& text, EditGroup group)
    {
        const TextRange range = m_Selection.Range();
        TextSelection after;
        after.Collapse(TextBuffer::EndOfText(range.begin, text));
        Replace(range, text, after, group);
    }

    void EditorDocument::InsertText(const std::string& text, bool typed)
    {
        if (text.empty()) return;
        const bool merge = typed && !m_Selection.HasSelection();
        ReplaceSelection(text, merge ? EditGroup::Typing : EditGroup::None);
    }

    void EditorDocument::InsertNewLine()
    {
        const TextPosition caret = m_Selection.Range().begin;
        const std::string& line = m_Buffer.Line(caret.line);

        // Keep the indentation of the current line (only the part before the caret).
        int indentEnd = 0;
        while (indentEnd < caret.column && (line[indentEnd] == ' ' || line[indentEnd] == '\t')) ++indentEnd;
        ReplaceSelection("\n" + line.substr(0, indentEnd), EditGroup::None);
    }

    void EditorDocument::InsertTab()
    {
        const TextPosition caret = m_Selection.Range().begin;
        const int displayColumn = Utf8::CountCodepoints(m_Buffer.Line(caret.line), caret.column);
        ReplaceSelection(std::string(TabSize - displayColumn % TabSize, ' '), EditGroup::None);
    }

    void EditorDocument::DeleteRange(const TextRange& range)
    {
        if (range.Empty()) return;
        TextSelection after;
        after.Collapse(range.begin);
        Replace(range, "", after, EditGroup::None);
    }

    void EditorDocument::Backspace(bool byWord)
    {
        if (m_Selection.HasSelection())
        {
            DeleteRange(m_Selection.Range());
            return;
        }
        int unused = -1;
        const TextPosition start = CaretNavigator::Move(
            m_Buffer, m_Selection.caret, byWord ? Movement::WordLeft : Movement::Left, 1, unused);
        DeleteRange({ start, m_Selection.caret });
    }

    void EditorDocument::DeleteForward(bool byWord)
    {
        if (m_Selection.HasSelection())
        {
            DeleteRange(m_Selection.Range());
            return;
        }
        int unused = -1;
        const TextPosition end = CaretNavigator::Move(
            m_Buffer, m_Selection.caret, byWord ? Movement::WordRight : Movement::Right, 1, unused);
        DeleteRange({ m_Selection.caret, end });
    }

    void EditorDocument::DuplicateLine()
    {
        if (m_Selection.HasSelection())
        {
            const TextRange range = m_Selection.Range();
            const std::string text = m_Buffer.GetText(range);
            TextSelection after;
            after.anchor = range.end;
            after.caret = TextBuffer::EndOfText(range.end, text);
            Replace({ range.end, range.end }, text, after, EditGroup::None);
            return;
        }

        const TextPosition caret = m_Selection.caret;
        const std::string& line = m_Buffer.Line(caret.line);
        const TextPosition lineEnd{ caret.line, static_cast<int>(line.size()) };
        TextSelection after;
        after.Collapse({ caret.line + 1, caret.column });
        Replace({ lineEnd, lineEnd }, "\n" + line, after, EditGroup::None);
    }

    bool EditorDocument::Undo()
    {
        if (!m_History.Undo(m_Buffer, m_Selection)) return false;
        m_Selection = { m_Buffer.Clamp(m_Selection.anchor), m_Buffer.Clamp(m_Selection.caret) };
        m_PreferredColumn = -1;
        ++m_Revision;
        return true;
    }

    bool EditorDocument::Redo()
    {
        if (!m_History.Redo(m_Buffer, m_Selection)) return false;
        m_Selection = { m_Buffer.Clamp(m_Selection.anchor), m_Buffer.Clamp(m_Selection.caret) };
        m_PreferredColumn = -1;
        ++m_Revision;
        return true;
    }
}
