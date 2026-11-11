#pragma once

#include "TextBuffer.hpp"
#include "TextTypes.hpp"

#include <string>
#include <vector>

namespace Diligent::ScriptEditor
{
    /// One atomic buffer change: `removed` was replaced by `inserted` at `position`.
    struct TextEdit
    {
        TextPosition position;
        std::string removed;
        std::string inserted;
    };

    /// Hint used to coalesce consecutive edits into a single undo step.
    enum class EditGroup
    {
        None,   ///< Always its own undo step.
        Typing  ///< Consecutive single-line typing merges into one step.
    };

    /// A set of edits that are undone/redone together, with the selection to restore.
    struct Transaction
    {
        std::vector<TextEdit> edits;
        TextSelection selectionBefore;
        TextSelection selectionAfter;
        EditGroup group = EditGroup::None;
        unsigned id = 0;
    };

    /// Undo/redo stacks. Applies transactions back onto a TextBuffer.
    class UndoHistory
    {
    public:
        /// Pushes a transaction (possibly merging it into the previous one) and clears the redo stack.
        void Record(Transaction transaction);

        bool CanUndo() const { return !m_Undo.empty(); }
        bool CanRedo() const { return !m_Redo.empty(); }

        /// Reverts the latest transaction. Returns false when there is nothing to undo.
        bool Undo(TextBuffer& buffer, TextSelection& selection);
        /// Re-applies the latest undone transaction. Returns false when there is nothing to redo.
        bool Redo(TextBuffer& buffer, TextSelection& selection);

        /// Identifies the current content state; equal ids mean equal content. 0 is the initial state.
        unsigned CurrentStateId() const { return m_Undo.empty() ? 0u : m_Undo.back().id; }

        /// Stops the next edit from merging into the previous undo step (e.g. after a save).
        void Seal() { m_Sealed = true; }

        void Clear();

    private:
        bool CanMerge(const Transaction& incoming) const;

        std::vector<Transaction> m_Undo;
        std::vector<Transaction> m_Redo;
        unsigned m_NextId = 1;
        bool m_Sealed = false;
    };
}
