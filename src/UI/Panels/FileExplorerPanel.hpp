#pragma once

#include "BasePanel.hpp"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace Diligent
{
    class FileExplorerPanel final : public BasePanel
    {
    public:
        using Opener = std::function<void(const std::filesystem::path&)>;

        explicit FileExplorerPanel(const std::string& name = "File Explorer");
        void Draw() override;

        void RegisterOpener(Opener opener);
        void SetRoot(const std::filesystem::path& root);
        const std::filesystem::path& GetCurrentDirectory() const { return m_CurrentDirectory; }

    private:
        struct Action { std::function<bool()> undo; std::function<bool()> redo; };
        enum class Dialog { None, NewFolder, Rename, Delete };

        void DrawToolbar();
        void DrawDirectoryContents();
        void DrawContextMenu();
        void DrawDialogs();
        void HandleShortcuts();
        void BeginDialog(Dialog dialog, const std::string& text);
        void SyncProjectRoot();
        void NavigateTo(const std::filesystem::path& directory);
        void Select(const std::filesystem::path& path, bool additive);
        void OpenSelected();
        void CopySelection(bool cut);
        void PasteClipboard();
        void CreateFolder(const std::string& name);
        void RenameSelected(const std::string& name);
        void DeleteSelection();
        void Undo();
        void Redo();
        void PushAction(Action action);
        bool IsInsideRoot(const std::filesystem::path& path) const;
        std::filesystem::path UniqueDestination(const std::filesystem::path& desired) const;
        static std::string DisplayName(const std::filesystem::directory_entry& entry);
        static bool CopyRecursively(const std::filesystem::path& source, const std::filesystem::path& destination);
        static bool RemoveRecursively(const std::filesystem::path& path);

        std::filesystem::path m_RootDirectory;
        std::filesystem::path m_CurrentDirectory;
        std::vector<std::filesystem::path> m_Selection;
        std::vector<std::filesystem::path> m_Clipboard;
        bool m_ClipboardIsCut = false;
        std::vector<Opener> m_Openers;
        std::vector<Action> m_UndoStack;
        std::vector<Action> m_RedoStack;
        std::string m_DialogText;
        char m_DialogBuffer[512] = {};
        Dialog m_Dialog = Dialog::None;
        std::string m_Status;
    };
}