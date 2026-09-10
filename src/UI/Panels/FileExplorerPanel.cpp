#include "FileExplorerPanel.hpp"

#include "ProjectLoader.hpp"
#include "HierarchyManager.hpp"
#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <system_error>

#include "HierarchyFeatures/PrefabFeature.hpp"

namespace Diligent
{
    namespace
    {
        std::filesystem::path Normalize(const std::filesystem::path& path)
        {
            std::error_code error;
            auto absolute = std::filesystem::absolute(path, error);
            return error ? path.lexically_normal() : absolute.lexically_normal();
        }

        bool IsSceneFile(const std::filesystem::path& path)
        {
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
            return extension == ".ascene";
        }

        bool IsPrefabFile(const std::filesystem::path& path)
        {
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
            return extension == ".aprefab";
        }
    }

    FileExplorerPanel::FileExplorerPanel(const std::string& name) : BasePanel(name)
    {
        SetRoot(ProjectLoader::GetCurrentProjectDir().empty() ? std::filesystem::current_path() : ProjectLoader::GetCurrentProjectDir());
        // TODO: Register file-type openers through a shared application registry instead of
        // coupling this panel directly to ProjectLoader.
        RegisterOpener([](const std::filesystem::path& path) {
            if (IsSceneFile(path))
                ProjectLoader::RequestSceneLoad(Normalize(path));
        });

        RegisterOpener([this](const std::filesystem::path& path) {
            if (!IsPrefabFile(path)) {
                return;
            }

            HierarchyObject::Ref created =
                PrefabFeature::GetInstance().InstantiatePrefab(Normalize(path));
            m_Status = created ? "Prefab instantiated." : "Failed to instantiate prefab.";
        });
    }

    void FileExplorerPanel::RegisterOpener(Opener opener) { if (opener) m_Openers.push_back(std::move(opener)); }

    void FileExplorerPanel::SetRoot(const std::filesystem::path& root)
    {
        m_RootDirectory = Normalize(root); m_CurrentDirectory = m_RootDirectory; m_Selection.clear();
    }

    bool FileExplorerPanel::IsInsideRoot(const std::filesystem::path& path) const
    {
        const auto root = Normalize(m_RootDirectory); const auto candidate = Normalize(path);
        auto rootIt = root.begin(); auto candidateIt = candidate.begin();
        for (; rootIt != root.end() && candidateIt != candidate.end(); ++rootIt, ++candidateIt)
            if (*rootIt != *candidateIt) return false;
        return rootIt == root.end();
    }

    void FileExplorerPanel::NavigateTo(const std::filesystem::path& directory)
    {
        std::error_code error;
        if (std::filesystem::is_directory(directory, error) && IsInsideRoot(directory))
        { m_CurrentDirectory = Normalize(directory); m_Selection.clear(); }
    }

    void FileExplorerPanel::Select(const std::filesystem::path& path, bool additive)
    {
        if (!additive) m_Selection.clear();
        auto it = std::find(m_Selection.begin(), m_Selection.end(), path);
        if (additive && it != m_Selection.end()) m_Selection.erase(it); else m_Selection.push_back(path);
    }

    void FileExplorerPanel::Draw()
    {
        if (!m_IsVisible) return;
        SyncProjectRoot();
        if (ImGui::Begin(m_Name.c_str(), &m_IsVisible))
        {
            DrawToolbar(); DrawDirectoryContents(); DrawContextMenu(); DrawDialogs(); HandleShortcuts();
            if (!m_Status.empty()) ImGui::TextDisabled("%s", m_Status.c_str());
        }
        ImGui::End();
    }

    void FileExplorerPanel::SyncProjectRoot()
    {
        const auto projectDirectory = ProjectLoader::GetCurrentProjectDir();
        const auto expectedRoot = Normalize(projectDirectory.empty() ? std::filesystem::current_path() : projectDirectory);
        if (expectedRoot == m_RootDirectory)
            return;

        m_RootDirectory = expectedRoot;
        m_CurrentDirectory = expectedRoot;
        m_Selection.clear();
        m_Clipboard.clear();
        m_ClipboardIsCut = false;
        m_UndoStack.clear();
        m_RedoStack.clear();
        m_Dialog = Dialog::None;
        m_Status.clear();
    }

    void FileExplorerPanel::DrawToolbar()
    {
        if (ImGui::Button("Back") && m_CurrentDirectory != m_RootDirectory) NavigateTo(m_CurrentDirectory.parent_path());
        ImGui::SameLine();
        if (ImGui::Button("Up") && m_CurrentDirectory != m_RootDirectory) NavigateTo(m_CurrentDirectory.parent_path());
        ImGui::SameLine();
        if (ImGui::Button("New Folder")) BeginDialog(Dialog::NewFolder, "New Folder");
        ImGui::SameLine();
        if (ImGui::Button("Undo")) Undo();
        ImGui::SameLine();
        if (ImGui::Button("Redo")) Redo();
        ImGui::Separator(); ImGui::Text("%s", m_CurrentDirectory.generic_string().c_str());
    }

    void FileExplorerPanel::DrawDirectoryContents()
    {
        if (!ImGui::BeginChild("##file_list", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true)) { ImGui::EndChild(); return; }
        std::error_code error; std::vector<std::filesystem::directory_entry> entries;
        for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory, error)) entries.push_back(entry);
        std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
            if (left.is_directory() != right.is_directory()) return left.is_directory();
            return left.path().filename().wstring() < right.path().filename().wstring();
        });

        for (const auto& entry : entries)
            DrawEntry(entry);
        ImGui::EndChild();
    }

    void FileExplorerPanel::DrawEntry(const std::filesystem::directory_entry& entry)
    {
        const auto path = entry.path();
        std::error_code error;
        const bool isDirectory = entry.is_directory(error);
        const bool selected = std::find(m_Selection.begin(), m_Selection.end(), path) != m_Selection.end();

        ImGui::PushID(path.generic_string().c_str());
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (selected) flags |= ImGuiTreeNodeFlags_Selected;
        if (!isDirectory) flags |= ImGuiTreeNodeFlags_Leaf;

        const bool isOpen = ImGui::TreeNodeEx(DisplayName(entry).c_str(), flags);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            Select(path, ImGui::GetIO().KeyCtrl);
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                if (isDirectory) NavigateTo(path);
                else OpenSelected();
            }
        }

        if (isOpen)
        {
            if (isDirectory)
            {
                std::vector<std::filesystem::directory_entry> children;
                for (const auto& child : std::filesystem::directory_iterator(path, error))
                    children.push_back(child);
                std::sort(children.begin(), children.end(), [](const auto& left, const auto& right) {
                    std::error_code leftError;
                    std::error_code rightError;
                    if (left.is_directory(leftError) != right.is_directory(rightError))
                        return left.is_directory(leftError);
                    return left.path().filename().wstring() < right.path().filename().wstring();
                });
                for (const auto& child : children)
                    DrawEntry(child);
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    void FileExplorerPanel::DrawContextMenu()
    {
        if (ImGui::BeginPopupContextWindow("##file_context", ImGuiPopupFlags_MouseButtonRight))
        {
            const bool hasSingleSelection = m_Selection.size() == 1;
            const bool canInstantiatePrefab = hasSingleSelection &&
                PrefabFeature::GetInstance().IsPrefabFile(m_Selection.front());

            if (ImGui::MenuItem("Open", nullptr, false, m_Selection.size() == 1)) OpenSelected();
            if (ImGui::MenuItem("Instantiate Prefab", nullptr, false, canInstantiatePrefab))
            {
                HierarchyObject::Ref created = PrefabFeature::GetInstance().InstantiatePrefab(m_Selection.front());
                m_Status = created ? "Prefab instantiated." : "Failed to instantiate prefab.";
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C", false, !m_Selection.empty())) CopySelection(false);
            if (ImGui::MenuItem("Cut", "Ctrl+X", false, !m_Selection.empty())) CopySelection(true);
            if (ImGui::MenuItem("Paste", "Ctrl+V", false, !m_Clipboard.empty())) PasteClipboard();
            if (ImGui::MenuItem("Rename", "F2", false, m_Selection.size() == 1)) BeginDialog(Dialog::Rename, m_Selection.front().filename().string());
            if (ImGui::MenuItem("Delete", "Del", false, !m_Selection.empty())) m_Dialog = Dialog::Delete;
            ImGui::Separator();
            if (ImGui::MenuItem("New Folder")) BeginDialog(Dialog::NewFolder, "New Folder");
            ImGui::EndPopup();
        }
    }

    void FileExplorerPanel::DrawDialogs()
    {
        if (m_Dialog == Dialog::None) return;
        ImGui::OpenPopup("File Explorer Action");
        if (ImGui::BeginPopupModal("File Explorer Action", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (m_Dialog == Dialog::Delete)
            {
                ImGui::Text("Delete %zu selected item(s)?", m_Selection.size());
                if (ImGui::Button("Delete")) { DeleteSelection(); m_Dialog = Dialog::None; ImGui::CloseCurrentPopup(); }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) { m_Dialog = Dialog::None; ImGui::CloseCurrentPopup(); }
            }
            else
            {
                ImGui::InputText("Name", m_DialogBuffer, sizeof(m_DialogBuffer));
                if (ImGui::Button("OK") && m_DialogBuffer[0] != '\0')
                { m_DialogText = m_DialogBuffer; if (m_Dialog == Dialog::NewFolder) CreateFolder(m_DialogText); else RenameSelected(m_DialogText); m_Dialog = Dialog::None; ImGui::CloseCurrentPopup(); }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) { m_Dialog = Dialog::None; ImGui::CloseCurrentPopup(); }
            }
            ImGui::EndPopup();
        }
    }

    void FileExplorerPanel::HandleShortcuts()
    {
        if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || ImGui::GetIO().WantTextInput) return;
        const bool ctrl = ImGui::GetIO().KeyCtrl;
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_C)) CopySelection(false);
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_X)) CopySelection(true);
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_V)) PasteClipboard();
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z)) Undo();
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y)) Redo();
        if (ImGui::IsKeyPressed(ImGuiKey_F2) && m_Selection.size() == 1) BeginDialog(Dialog::Rename, m_Selection.front().filename().string());
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !m_Selection.empty()) m_Dialog = Dialog::Delete;
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) && m_Selection.size() == 1) OpenSelected();
        if (ImGui::IsKeyPressed(ImGuiKey_Backspace) && m_CurrentDirectory != m_RootDirectory) NavigateTo(m_CurrentDirectory.parent_path());
    }

    void FileExplorerPanel::BeginDialog(Dialog dialog, const std::string& text)
    {
        m_Dialog = dialog;
        m_DialogText = text;
        std::strncpy(m_DialogBuffer, text.c_str(), sizeof(m_DialogBuffer) - 1);
        m_DialogBuffer[sizeof(m_DialogBuffer) - 1] = '\0';
    }

    void FileExplorerPanel::OpenSelected()
    {
        if (m_Selection.size() != 1) return;
        std::error_code error;
        if (std::filesystem::is_directory(m_Selection.front(), error)) NavigateTo(m_Selection.front());
        else for (const auto& opener : m_Openers) opener(m_Selection.front());
    }

    void FileExplorerPanel::CopySelection(bool cut) { m_Clipboard = m_Selection; m_ClipboardIsCut = cut; }

    bool FileExplorerPanel::CopyRecursively(const std::filesystem::path& source, const std::filesystem::path& destination)
    {
        std::error_code error;
        if (std::filesystem::is_directory(source, error))
        {
            std::filesystem::create_directories(destination, error);
            for (const auto& child : std::filesystem::directory_iterator(source, error))
                if (!CopyRecursively(child.path(), destination / child.path().filename())) return false;
            return !error;
        }
        std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing, error);
        return !error;
    }

    bool FileExplorerPanel::RemoveRecursively(const std::filesystem::path& path)
    { std::error_code error; std::filesystem::remove_all(path, error); return !error; }

    std::filesystem::path FileExplorerPanel::UniqueDestination(const std::filesystem::path& desired) const
    {
        if (!std::filesystem::exists(desired)) return desired;
        for (int index = 1; ; ++index)
        {
            auto candidate = desired.parent_path() / (desired.stem().string() + " (" + std::to_string(index) + ")" + desired.extension().string());
            if (!std::filesystem::exists(candidate)) return candidate;
        }
    }

    void FileExplorerPanel::PasteClipboard()
    {
        struct Change { std::filesystem::path source; std::filesystem::path destination; bool moved; };
        std::vector<Change> changes;
        for (const auto& source : m_Clipboard)
        {
            auto destination = UniqueDestination(m_CurrentDirectory / source.filename());
            if (m_ClipboardIsCut)
            {
                std::error_code error; std::filesystem::rename(source, destination, error);
                if (!error) changes.push_back({ source, destination, true });
            }
            else if (CopyRecursively(source, destination)) changes.push_back({ source, destination, false });
        }
        if (!changes.empty())
        {
            PushAction({ [changes]() { for (const auto& change : changes) { if (change.moved) { std::error_code ec; std::filesystem::rename(change.destination, change.source, ec); if (ec) return false; } else if (!RemoveRecursively(change.destination)) return false; } return true; },
                [changes]() { for (const auto& change : changes) { if (change.moved) { std::error_code ec; std::filesystem::rename(change.source, change.destination, ec); if (ec) return false; } else if (!CopyRecursively(change.source, change.destination)) return false; } return true; } });
            if (m_ClipboardIsCut) m_Clipboard.clear();
        }
    }

    void FileExplorerPanel::CreateFolder(const std::string& name)
    {
        const std::filesystem::path targetName(name);
        if (targetName.has_parent_path() || targetName.filename() != targetName) return;
        const auto target = UniqueDestination(m_CurrentDirectory / targetName); std::error_code error;
        if (std::filesystem::create_directory(target, error))
            PushAction({ [target]() { return RemoveRecursively(target); }, [target]() { std::error_code ec; return std::filesystem::create_directory(target, ec) && !ec; } });
    }

    void FileExplorerPanel::RenameSelected(const std::string& name)
    {
        if (m_Selection.size() != 1) return;
        const std::filesystem::path targetName(name);
        if (targetName.has_parent_path() || targetName.filename() != targetName) return;
        const auto oldPath = m_Selection.front(); const auto newPath = oldPath.parent_path() / targetName; std::error_code error;
        std::filesystem::rename(oldPath, newPath, error);
        if (!error)
        {
            m_Selection.front() = newPath;
            PushAction({ [oldPath, newPath]() { std::error_code ec; std::filesystem::rename(newPath, oldPath, ec); return !ec; }, [oldPath, newPath]() { std::error_code ec; std::filesystem::rename(oldPath, newPath, ec); return !ec; } });
        }
    }

    void FileExplorerPanel::DeleteSelection()
    {
        struct Deleted { std::filesystem::path path; std::filesystem::path backup; };
        std::vector<Deleted> deleted;
        for (const auto& path : m_Selection)
        {
            auto backup = std::filesystem::temp_directory_path() / ("anitotracer_undo_" + std::to_string(std::hash<std::string>{}(path.generic_string() + std::to_string(ImGui::GetTime())))); std::error_code error;
            std::filesystem::rename(path, backup, error); if (!error) deleted.push_back({ path, backup });
        }
        if (!deleted.empty())
        {
            m_Selection.clear();
            PushAction({ [deleted]() { for (const auto& item : deleted) { std::error_code ec; std::filesystem::rename(item.backup, item.path, ec); if (ec) return false; } return true; }, [deleted]() { for (const auto& item : deleted) { std::error_code ec; std::filesystem::rename(item.path, item.backup, ec); if (ec) return false; } return true; } });
        }
    }

    void FileExplorerPanel::PushAction(Action action) { m_UndoStack.push_back(std::move(action)); m_RedoStack.clear(); }
    void FileExplorerPanel::Undo() { if (m_UndoStack.empty()) return; Action action = std::move(m_UndoStack.back()); m_UndoStack.pop_back(); if (action.undo()) m_RedoStack.push_back(std::move(action)); }
    void FileExplorerPanel::Redo() { if (m_RedoStack.empty()) return; Action action = std::move(m_RedoStack.back()); m_RedoStack.pop_back(); if (action.redo()) m_UndoStack.push_back(std::move(action)); }
    std::string FileExplorerPanel::DisplayName(const std::filesystem::directory_entry& entry) { return entry.path().filename().string(); }
}