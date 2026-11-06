#pragma once

#include "CaretNavigator.hpp"
#include "TextBuffer.hpp"
#include "TextTypes.hpp"
#include "UndoHistory.hpp"

#include <filesystem>
#include <string>

namespace Diligent::ScriptEditor
{
    /// A single open file: its text, selection, undo history and on-disk state.
    /// All mutations go through this class so undo history and the dirty flag stay consistent.
    /// It has no dependency on ImGui; presentation lives in ScriptEditorView.
    class EditorDocument
    {
    public:
        static constexpr int TabSize = 4;

        /// Loads `path` from disk. On failure returns false and leaves the document untouched.
        bool Load(const std::filesystem::path& path);
        /// Writes the document back to its path and marks it clean.
        bool Save();

        const std::filesystem::path& GetPath() const { return m_Path; }
        std::string GetDisplayName() const { return m_Path.filename().string(); }
        bool IsDirty() const { return m_History.CurrentStateId() != m_SavedStateId; }

        const TextBuffer& GetBuffer() const { return m_Buffer; }
        const TextSelection& GetSelection() const { return m_Selection; }
        /// Increases whenever the text changes (including undo/redo). Useful for cache invalidation.
        unsigned GetRevision() const { return m_Revision; }

        // --- Selection and caret ---
        void SetSelection(TextPosition anchor, TextPosition caret);
        void SelectAll();
        void SelectRange(const TextRange& range);
        /// Moves the caret; with `extend` the anchor stays so a selection grows.
        void MoveCaret(Movement movement, bool extend, int pageLines);

        // --- Queries ---
        std::string GetSelectedText() const;
        std::string GetText(const TextRange& range) const { return m_Buffer.GetText(range); }
        /// The caret line including its trailing line break (when there is one).
        TextRange GetCurrentLineRange() const;

        // --- Editing ---
        /// Replaces the selection (if any) with `text`. `typed` enables merging into one undo step.
        void InsertText(const std::string& text, bool typed = false);
        void InsertNewLine();
        void InsertTab();
        void Backspace(bool byWord);
        void DeleteForward(bool byWord);
        void DeleteRange(const TextRange& range);
        /// Duplicates the selection in place, or the caret line below itself when nothing is selected.
        void DuplicateLine();

        bool Undo();
        bool Redo();
        bool CanUndo() const { return m_History.CanUndo(); }
        bool CanRedo() const { return m_History.CanRedo(); }

    private:
        /// Core mutation: replaces `range` with `text`, records history and sets the resulting selection.
        void Replace(TextRange range, const std::string& text, const TextSelection& selectionAfter, EditGroup group);
        void ReplaceSelection(const std::string& text, EditGroup group);

        std::filesystem::path m_Path;
        TextBuffer m_Buffer;
        TextSelection m_Selection;
        UndoHistory m_History;
        unsigned m_SavedStateId = 0;
        unsigned m_Revision = 0;
        int m_PreferredColumn = -1;
        bool m_HasUtf8Bom = false;
    };
}
