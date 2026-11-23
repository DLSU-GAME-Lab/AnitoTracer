#include "UndoHistory.hpp"

namespace Diligent::ScriptEditor
{
    namespace
    {
        void ApplyForward(TextBuffer& buffer, const TextEdit& edit)
        {
            buffer.Erase({ edit.position, TextBuffer::EndOfText(edit.position, edit.removed) });
            buffer.Insert(edit.position, edit.inserted);
        }

        void ApplyBackward(TextBuffer& buffer, const TextEdit& edit)
        {
            buffer.Erase({ edit.position, TextBuffer::EndOfText(edit.position, edit.inserted) });
            buffer.Insert(edit.position, edit.removed);
        }
    }

    bool UndoHistory::CanMerge(const Transaction& incoming) const
    {
        if (m_Sealed || m_Undo.empty()) return false;
        const Transaction& last = m_Undo.back();
        if (last.group != EditGroup::Typing || incoming.group != EditGroup::Typing) return false;
        if (last.edits.size() != 1 || incoming.edits.size() != 1) return false;

        const TextEdit& previous = last.edits.front();
        const TextEdit& next = incoming.edits.front();
        if (!previous.removed.empty() || !next.removed.empty()) return false;
        if (next.inserted.find('\n') != std::string::npos) return false;
        return TextBuffer::EndOfText(previous.position, previous.inserted) == next.position;
    }

    void UndoHistory::Record(Transaction transaction)
    {
        m_Redo.clear();
        transaction.id = m_NextId++;

        if (CanMerge(transaction))
        {
            Transaction& last = m_Undo.back();
            last.edits.front().inserted += transaction.edits.front().inserted;
            last.selectionAfter = transaction.selectionAfter;
            last.id = transaction.id;
        }
        else
        {
            m_Undo.push_back(std::move(transaction));
        }
        m_Sealed = false;
    }

    bool UndoHistory::Undo(TextBuffer& buffer, TextSelection& selection)
    {
        if (m_Undo.empty()) return false;
        Transaction transaction = std::move(m_Undo.back());
        m_Undo.pop_back();

        for (auto it = transaction.edits.rbegin(); it != transaction.edits.rend(); ++it)
            ApplyBackward(buffer, *it);
        selection = transaction.selectionBefore;

        m_Redo.push_back(std::move(transaction));
        m_Sealed = true;
        return true;
    }

    bool UndoHistory::Redo(TextBuffer& buffer, TextSelection& selection)
    {
        if (m_Redo.empty()) return false;
        Transaction transaction = std::move(m_Redo.back());
        m_Redo.pop_back();

        for (const TextEdit& edit : transaction.edits)
            ApplyForward(buffer, edit);
        selection = transaction.selectionAfter;

        m_Undo.push_back(std::move(transaction));
        m_Sealed = true;
        return true;
    }

    void UndoHistory::Clear()
    {
        m_Undo.clear();
        m_Redo.clear();
        m_Sealed = false;
    }
}
