#pragma once

#include "EditorCommands.hpp"
#include "EditorDocument.hpp"
#include "EditorKeyBindings.hpp"

#include "imgui.h"

namespace Diligent::ScriptEditor
{
    /// Per-tab presentation state that must survive between frames (kept outside the document
    /// so the document stays UI-agnostic).
    struct EditorViewState
    {
        unsigned measuredRevision = ~0u; ///< Document revision `maxLineWidth` was computed for.
        ImFont* measuredFont = nullptr;
        float measuredFontSize = 0.0f;
        float maxLineWidth = 0.0f;       ///< Width of the widest line, used for the horizontal scroll range.
        TextPosition lastCaret{ -1, -1 };///< Used to detect caret movement and scroll it into view.
        unsigned lastRevision = ~0u;
        double lastInteractionTime = 0.0;///< Restarts caret blinking.
        bool dragging = false;           ///< A mouse drag selection is in progress.
    };

    /// Draws one EditorDocument as an editable code view: line numbers, selection, caret,
    /// mouse selection and keyboard handling. Editing itself is delegated to the command executor.
    class ScriptEditorView
    {
    public:
        ScriptEditorView();

        /// Draws the view into a child region of `size`. When `takeFocus` is set the view grabs keyboard focus.
        void Draw(EditorDocument& document, EditorViewState& state, EditorEnvironment& environment,
                  const ImVec2& size, bool takeFocus);

        EditorKeyBindings& GetKeyBindings() { return m_KeyBindings; }
        EditorCommandExecutor& GetExecutor() { return m_Executor; }

    private:
        EditorKeyBindings m_KeyBindings;
        EditorCommandExecutor m_Executor;
    };
}
