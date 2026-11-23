#pragma once

#include "BasePanel.hpp"
#include "ScriptEditor/EditorCommands.hpp"
#include "ScriptEditor/EditorDocument.hpp"
#include "ScriptEditor/ScriptEditorView.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Diligent
{
    /// Integrated script editor: a tab bar of open files, each shown in a ScriptEditorView.
    /// The panel only manages tabs, focus and file lifecycle; text editing lives in the ScriptEditor module.
    class ScriptEditorPanel final : public BasePanel
    {
    public:
        explicit ScriptEditorPanel(const std::string& name = "Script Editor");
        void Draw() override;

        /// Opens `path` in a tab (or activates the existing tab), then shows and focuses the panel.
        /// Returns false if the file could not be read.
        bool OpenFile(const std::filesystem::path& path);

    private:
        struct Tab
        {
            std::unique_ptr<ScriptEditor::EditorDocument> document;
            ScriptEditor::EditorViewState viewState;
        };

        void DrawTabs();
        void DrawTabContent(Tab& tab);
        void DrawStatusBar(const Tab& tab) const;
        void DrawCloseConfirmation();
        void RequestClose(Tab& tab);
        void CloseTab(const Tab* tab);
        Tab* FindTab(const std::filesystem::path& path);

        std::vector<std::unique_ptr<Tab>> m_Tabs;
        ScriptEditor::ScriptEditorView m_View;
        ScriptEditor::EditorEnvironment m_Environment;

        const Tab* m_TabToSelect = nullptr;   ///< Tab to activate next frame (a newly opened/re-opened file).
        const Tab* m_TabPendingClose = nullptr; ///< Dirty tab awaiting the user's save/discard decision.
        bool m_OpenCloseConfirmation = false;
        bool m_FocusPanelRequested = false;
        bool m_FocusEditorRequested = false;
        std::string m_Status;
    };
}
