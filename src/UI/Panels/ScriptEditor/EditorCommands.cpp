#include "EditorCommands.hpp"

namespace Diligent::ScriptEditor
{
    namespace
    {
        void Report(EditorEnvironment& env, const std::string& message)
        {
            if (env.reportStatus) env.reportStatus(message);
        }

        /// Text to copy/cut: the selection, or the whole caret line when nothing is selected.
        TextRange ClipboardRange(const EditorDocument& document)
        {
            return document.GetSelection().HasSelection() ? document.GetSelection().Range() : document.GetCurrentLineRange();
        }
    }

    EditorCommandExecutor::EditorCommandExecutor()
    {
        Register(CommandId::Save, [](EditorDocument& doc, const EditorCommand&, EditorEnvironment& env) {
            Report(env, doc.Save() ? "Saved " + doc.GetDisplayName() : "Failed to save " + doc.GetDisplayName());
        });
        Register(CommandId::Undo, [](EditorDocument& doc, const EditorCommand&, EditorEnvironment&) { doc.Undo(); });
        Register(CommandId::Redo, [](EditorDocument& doc, const EditorCommand&, EditorEnvironment&) { doc.Redo(); });

        Register(CommandId::Copy, [](EditorDocument& doc, const EditorCommand&, EditorEnvironment& env) {
            const std::string text = doc.GetText(ClipboardRange(doc));
            if (env.setClipboard && !text.empty()) env.setClipboard(text);
        });
        Register(CommandId::Cut, [](EditorDocument& doc, const EditorCommand&, EditorEnvironment& env) {
            const TextRange range = ClipboardRange(doc);
            const std::string text = doc.GetText(range);
            if (text.empty()) return;
            if (!env.setClipboard)
            {
                Report(env, "Clipboard is unavailable.");
                return;
            }
            env.setClipboard(text);
            doc.DeleteRange(range);
        });
        Register(CommandId::Paste, [](EditorDocument& doc, const EditorCommand&, EditorEnvironment& env) {
            if (env.getClipboard) doc.InsertText(env.getClipboard());
        });

        Register(CommandId::DuplicateLine, [](EditorDocument& doc, const EditorCommand&, EditorEnvironment&) { doc.DuplicateLine(); });
        Register(CommandId::SelectAll, [](EditorDocument& doc, const EditorCommand&, EditorEnvironment&) { doc.SelectAll(); });
        Register(CommandId::MoveCaret, [](EditorDocument& doc, const EditorCommand& cmd, EditorEnvironment& env) {
            doc.MoveCaret(cmd.movement, cmd.extendSelection, env.pageLines);
        });
        Register(CommandId::Backspace, [](EditorDocument& doc, const EditorCommand& cmd, EditorEnvironment&) { doc.Backspace(cmd.byWord); });
        Register(CommandId::DeleteForward, [](EditorDocument& doc, const EditorCommand& cmd, EditorEnvironment&) { doc.DeleteForward(cmd.byWord); });
        Register(CommandId::NewLine, [](EditorDocument& doc, const EditorCommand&, EditorEnvironment&) { doc.InsertNewLine(); });
        Register(CommandId::Tab, [](EditorDocument& doc, const EditorCommand&, EditorEnvironment&) { doc.InsertTab(); });
    }

    bool EditorCommandExecutor::Execute(const EditorCommand& command, EditorDocument& document, EditorEnvironment& environment) const
    {
        const auto it = m_Handlers.find(command.id);
        if (it == m_Handlers.end()) return false;
        it->second(document, command, environment);
        return true;
    }
}
