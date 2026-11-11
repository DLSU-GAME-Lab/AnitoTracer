#include "EditorKeyBindings.hpp"

#include <algorithm>

namespace Diligent::ScriptEditor
{
    namespace
    {
        EditorCommand Simple(CommandId id)
        {
            EditorCommand command;
            command.id = id;
            return command;
        }

        EditorCommand Move(Movement movement, bool extend)
        {
            EditorCommand command;
            command.id = CommandId::MoveCaret;
            command.movement = movement;
            command.extendSelection = extend;
            return command;
        }

        EditorCommand Delete(CommandId id, bool byWord)
        {
            EditorCommand command;
            command.id = id;
            command.byWord = byWord;
            return command;
        }

        /// Binds a movement key plain (+Shift to extend) and with Ctrl (+Shift to extend).
        void BindMovementKey(EditorKeyBindings& bindings, ImGuiKey key, Movement plain, Movement withCtrl)
        {
            bindings.Bind(key, { false, false, false }, Move(plain, false));
            bindings.Bind(key, { false, true, false }, Move(plain, true));
            bindings.Bind(key, { true, false, false }, Move(withCtrl, false));
            bindings.Bind(key, { true, true, false }, Move(withCtrl, true));
        }
    }

    EditorKeyBindings EditorKeyBindings::CreateDefault()
    {
        EditorKeyBindings bindings;

        bindings.Bind(ImGuiKey_S, { true, false, false }, Simple(CommandId::Save), false);
        bindings.Bind(ImGuiKey_Z, { true, false, false }, Simple(CommandId::Undo));
        bindings.Bind(ImGuiKey_Y, { true, false, false }, Simple(CommandId::Redo));
        bindings.Bind(ImGuiKey_Z, { true, true, false }, Simple(CommandId::Redo));
        bindings.Bind(ImGuiKey_C, { true, false, false }, Simple(CommandId::Copy), false);
        bindings.Bind(ImGuiKey_X, { true, false, false }, Simple(CommandId::Cut));
        bindings.Bind(ImGuiKey_V, { true, false, false }, Simple(CommandId::Paste));
        bindings.Bind(ImGuiKey_D, { true, false, false }, Simple(CommandId::DuplicateLine));
        bindings.Bind(ImGuiKey_A, { true, false, false }, Simple(CommandId::SelectAll), false);

        BindMovementKey(bindings, ImGuiKey_LeftArrow, Movement::Left, Movement::WordLeft);
        BindMovementKey(bindings, ImGuiKey_RightArrow, Movement::Right, Movement::WordRight);
        BindMovementKey(bindings, ImGuiKey_UpArrow, Movement::Up, Movement::Up);
        BindMovementKey(bindings, ImGuiKey_DownArrow, Movement::Down, Movement::Down);
        BindMovementKey(bindings, ImGuiKey_Home, Movement::LineStart, Movement::DocumentStart);
        BindMovementKey(bindings, ImGuiKey_End, Movement::LineEnd, Movement::DocumentEnd);
        BindMovementKey(bindings, ImGuiKey_PageUp, Movement::PageUp, Movement::PageUp);
        BindMovementKey(bindings, ImGuiKey_PageDown, Movement::PageDown, Movement::PageDown);

        bindings.Bind(ImGuiKey_Backspace, { false, false, false }, Delete(CommandId::Backspace, false));
        bindings.Bind(ImGuiKey_Backspace, { false, true, false }, Delete(CommandId::Backspace, false));
        bindings.Bind(ImGuiKey_Backspace, { true, false, false }, Delete(CommandId::Backspace, true));
        bindings.Bind(ImGuiKey_Delete, { false, false, false }, Delete(CommandId::DeleteForward, false));
        bindings.Bind(ImGuiKey_Delete, { true, false, false }, Delete(CommandId::DeleteForward, true));

        bindings.Bind(ImGuiKey_Enter, { false, false, false }, Simple(CommandId::NewLine));
        bindings.Bind(ImGuiKey_KeypadEnter, { false, false, false }, Simple(CommandId::NewLine));
        bindings.Bind(ImGuiKey_Tab, { false, false, false }, Simple(CommandId::Tab));
        return bindings;
    }

    void EditorKeyBindings::Bind(ImGuiKey key, KeyModifiers modifiers, const EditorCommand& command, bool repeat)
    {
        m_Bindings.push_back({ key, modifiers, command, repeat });
    }

    std::vector<EditorCommand> EditorKeyBindings::Poll() const
    {
        const ImGuiIO& io = ImGui::GetIO();
        std::vector<EditorCommand> fired;
        for (const KeyBinding& binding : m_Bindings)
        {
            if (binding.modifiers.ctrl != io.KeyCtrl ||
                binding.modifiers.shift != io.KeyShift ||
                binding.modifiers.alt != io.KeyAlt)
                continue;
            if (ImGui::IsKeyPressed(binding.key, binding.repeat))
                fired.push_back(binding.command);
        }
        return fired;
    }
}
