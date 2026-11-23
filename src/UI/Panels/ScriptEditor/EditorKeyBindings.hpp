#pragma once

#include "EditorCommands.hpp"

#include "imgui.h"

#include <vector>

namespace Diligent::ScriptEditor
{
    /// Modifier keys that must be held (exactly) for a binding to fire.
    struct KeyModifiers
    {
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
    };

    /// Associates a key chord with an editor command.
    struct KeyBinding
    {
        ImGuiKey key = ImGuiKey_None;
        KeyModifiers modifiers;
        EditorCommand command;
        bool repeat = true; ///< Fire again while the key is held down.
    };

    /// Table of key chords to commands. Rebind or extend by calling Bind().
    class EditorKeyBindings
    {
    public:
        /// Standard IDE bindings: Ctrl+S/Z/Y/C/X/V/D/A, arrows with Shift/Ctrl selection, Home/End, etc.
        static EditorKeyBindings CreateDefault();

        /// Adds a binding. Bind a chord with the same key and modifiers only once.
        void Bind(ImGuiKey key, KeyModifiers modifiers, const EditorCommand& command, bool repeat = true);

        /// Commands whose chord was pressed this frame, in binding order.
        std::vector<EditorCommand> Poll() const;

    private:
        std::vector<KeyBinding> m_Bindings;
    };
}
