#pragma once

#include "CaretNavigator.hpp"
#include "EditorDocument.hpp"

#include <functional>
#include <string>
#include <unordered_map>

namespace Diligent::ScriptEditor
{
    /// Identifies an editor action. To add a new action: add an id here, register a handler in
    /// EditorCommandExecutor, and optionally bind a key in EditorKeyBindings.
    enum class CommandId
    {
        Save,
        Undo,
        Redo,
        Copy,
        Cut,
        Paste,
        DuplicateLine,
        SelectAll,
        MoveCaret,
        Backspace,
        DeleteForward,
        NewLine,
        Tab
    };

    /// An action plus its parameters (only some commands use them).
    struct EditorCommand
    {
        CommandId id = CommandId::MoveCaret;
        Movement movement = Movement::Left; ///< MoveCaret: where to go.
        bool extendSelection = false;       ///< MoveCaret: keep the anchor.
        bool byWord = false;                ///< Backspace / DeleteForward: act on a whole word.
    };

    /// Services commands need from the outside world; keeps the executor free of ImGui.
    struct EditorEnvironment
    {
        std::function<std::string()> getClipboard;
        std::function<void(const std::string&)> setClipboard;
        std::function<void(const std::string&)> reportStatus;
        int pageLines = 20; ///< How many lines PageUp/PageDown travel.
    };

    /// Maps command ids to handlers and runs them against a document.
    class EditorCommandExecutor
    {
    public:
        using Handler = std::function<void(EditorDocument&, const EditorCommand&, EditorEnvironment&)>;

        /// Registers all built-in commands.
        EditorCommandExecutor();

        /// Adds or replaces the handler for a command.
        void Register(CommandId id, Handler handler) { m_Handlers[id] = std::move(handler); }

        /// Runs a command. Returns false if no handler is registered for it.
        bool Execute(const EditorCommand& command, EditorDocument& document, EditorEnvironment& environment) const;

    private:
        std::unordered_map<CommandId, Handler> m_Handlers;
    };
}
