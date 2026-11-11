#include "ScriptEditorPanel.hpp"

#include "FileOpeners/FileOpener.hpp"
#include "ScriptEditor/Utf8.hpp"
#include "imgui.h"

#include <algorithm>

namespace Diligent
{
    ScriptEditorPanel::ScriptEditorPanel(const std::string& name) : BasePanel(name)
    {
        // Hidden until a script is opened (or the user enables it from the Windows menu).
        m_IsVisible = false;

        m_Environment.getClipboard = [] {
            const char* text = ImGui::GetClipboardText();
            return text ? std::string(text) : std::string();
        };
        m_Environment.setClipboard = [](const std::string& text) { ImGui::SetClipboardText(text.c_str()); };
        m_Environment.reportStatus = [this](const std::string& status) { m_Status = status; };
    }

    ScriptEditorPanel::Tab* ScriptEditorPanel::FindTab(const std::filesystem::path& path)
    {
        for (auto& tab : m_Tabs)
            if (tab->document->GetPath() == path)
                return tab.get();
        return nullptr;
    }

    bool ScriptEditorPanel::OpenFile(const std::filesystem::path& path)
    {
        const std::filesystem::path normalized = FileOpeners::NormalizePath(path);

        Tab* tab = FindTab(normalized);
        if (!tab)
        {
            auto document = std::make_unique<ScriptEditor::EditorDocument>();
            if (!document->Load(normalized))
            {
                m_Status = "Failed to open " + normalized.filename().string();
                m_IsVisible = true;
                m_FocusPanelRequested = true;
                return false;
            }
            auto created = std::make_unique<Tab>();
            created->document = std::move(document);
            tab = created.get();
            m_Tabs.push_back(std::move(created));
        }

        m_Status.clear();
        m_TabToSelect = tab;
        m_IsVisible = true;
        m_FocusPanelRequested = true;
        m_FocusEditorRequested = true;
        return true;
    }

    void ScriptEditorPanel::Draw()
    {
        if (!m_IsVisible) return;

        const bool open = ImGui::Begin(m_Name.c_str(), &m_IsVisible, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // Must happen even when the window is a background dock tab so it comes to the front.
        if (m_FocusPanelRequested)
        {
            ImGui::SetWindowFocus();
            m_FocusPanelRequested = false;
        }

        if (open)
        {
            if (m_Tabs.empty())
            {
                ImGui::TextDisabled("Double-click an .ascript file in the File Explorer to edit it here.");
                if (!m_Status.empty()) ImGui::TextUnformatted(m_Status.c_str());
            }
            else
                DrawTabs();
            DrawCloseConfirmation();
        }
        ImGui::End();
    }

    void ScriptEditorPanel::DrawTabs()
    {
        if (!ImGui::BeginTabBar("##ScriptTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll))
            return;

        Tab* closeRequested = nullptr;
        for (auto& tab : m_Tabs)
        {
            ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
            if (tab->document->IsDirty()) flags |= ImGuiTabItemFlags_UnsavedDocument;
            if (m_TabToSelect == tab.get()) flags |= ImGuiTabItemFlags_SetSelected;

            // "###" keeps the tab identity stable when the title changes.
            const std::string label = tab->document->GetDisplayName() + "###" + tab->document->GetPath().string();
            bool keepOpen = true;
            if (ImGui::BeginTabItem(label.c_str(), &keepOpen, flags))
            {
                DrawTabContent(*tab);
                ImGui::EndTabItem();
                if (m_TabToSelect == tab.get()) m_TabToSelect = nullptr;
            }
            if (!keepOpen) closeRequested = tab.get();
        }
        ImGui::EndTabBar();

        if (closeRequested) RequestClose(*closeRequested);
    }

    void ScriptEditorPanel::DrawTabContent(Tab& tab)
    {
        const float statusHeight = ImGui::GetFrameHeightWithSpacing();
        ImVec2 size = ImGui::GetContentRegionAvail();
        size.y = std::max(1.0f, size.y - statusHeight);

        const bool takeFocus = m_FocusEditorRequested && (!m_TabToSelect || m_TabToSelect == &tab);
        m_View.Draw(*tab.document, tab.viewState, m_Environment, size, takeFocus);
        if (takeFocus) m_FocusEditorRequested = false;
        DrawStatusBar(tab);
    }

    void ScriptEditorPanel::DrawStatusBar(const Tab& tab) const
    {
        const auto& buffer = tab.document->GetBuffer();
        const auto caret = tab.document->GetSelection().caret;
        const int column = ScriptEditor::Utf8::CountCodepoints(buffer.Line(caret.line), caret.column) + 1;
        ImGui::TextDisabled("Ln %d, Col %d", caret.line + 1, column);
        if (!m_Status.empty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("|  %s", m_Status.c_str());
        }
    }

    void ScriptEditorPanel::RequestClose(Tab& tab)
    {
        if (tab.document->IsDirty())
        {
            m_Status.clear();
            m_TabPendingClose = &tab;
            m_OpenCloseConfirmation = true;
        }
        else
        {
            CloseTab(&tab);
        }
    }

    void ScriptEditorPanel::CloseTab(const Tab* tab)
    {
        m_Tabs.erase(std::remove_if(m_Tabs.begin(), m_Tabs.end(), [tab](const auto& t) { return t.get() == tab; }), m_Tabs.end());
        if (m_TabToSelect == tab) m_TabToSelect = nullptr;
        if (m_TabPendingClose == tab) m_TabPendingClose = nullptr;
    }

    void ScriptEditorPanel::DrawCloseConfirmation()
    {
        const char* popupId = "Unsaved Changes##ScriptEditor";
        if (m_OpenCloseConfirmation)
        {
            ImGui::OpenPopup(popupId);
            m_OpenCloseConfirmation = false;
        }

        if (!ImGui::BeginPopupModal(popupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;

        Tab* pending = nullptr;
        for (auto& tab : m_Tabs)
            if (tab.get() == m_TabPendingClose) pending = tab.get();

        if (!pending)
        {
            ImGui::CloseCurrentPopup();
        }
        else
        {
            ImGui::Text("Save changes to %s before closing?", pending->document->GetDisplayName().c_str());
            if (ImGui::Button("Save"))
            {
                if (pending->document->Save())
                {
                    CloseTab(pending);
                    ImGui::CloseCurrentPopup();
                }
                else m_Status = "Failed to save " + pending->document->GetDisplayName();
            }
            if (!m_Status.empty()) ImGui::TextUnformatted(m_Status.c_str());
            ImGui::SameLine();
            if (ImGui::Button("Discard"))
            {
                CloseTab(pending);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                m_TabPendingClose = nullptr;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}
