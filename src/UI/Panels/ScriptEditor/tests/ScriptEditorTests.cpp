#include "../EditorCommands.hpp"
#include "../EditorKeyBindings.hpp"
#include "../LineLayout.hpp"
#include "../../FileOpeners/ScriptFileOpener.hpp"

#include "imgui.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

using namespace Diligent::ScriptEditor;

namespace
{
    void Check(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    void Write(const std::filesystem::path& path, const std::string& contents)
    {
        std::ofstream file(path, std::ios::binary);
        file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        if (!file) throw std::runtime_error("Unable to create test file");
    }

    std::string Read(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("Unable to read test file");
        return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    }

    void TestDocument(const std::filesystem::path& path)
    {
        const std::string original = "\xEF\xBB\xBF" "alpha beta\r\nsecond\r\n";
        Write(path, original);
        EditorDocument document;
        Check(document.Load(path), "load failed");
        Check(!document.IsDirty(), "new document is dirty");
        Check(document.GetBuffer().GetText() == original.substr(3), "load changed line endings");

        document.SetSelection({ 0, 0 }, { 0, 0 });
        document.MoveCaret(Movement::WordRight, true, 10);
        Check(document.GetSelectedText() == "alpha ", "word selection failed");

        EditorCommandExecutor commands;
        EditorEnvironment environment;
        std::string clipboard;
        environment.setClipboard = [&](const std::string& text) { clipboard = text; };
        environment.getClipboard = [&] { return clipboard; };
        commands.Execute({ CommandId::Copy }, document, environment);
        Check(clipboard == "alpha ", "copy failed");
        EditorEnvironment unavailableClipboard;
        commands.Execute({ CommandId::Cut }, document, unavailableClipboard);
        Check(document.GetSelectedText() == "alpha ", "cut lost text without clipboard");
        commands.Execute({ CommandId::Paste }, document, environment);
        Check(document.GetBuffer().GetText() == original.substr(3), "paste did not replace selection");
        Check(document.Undo(), "paste undo failed");
        Check(document.GetSelectedText() == "alpha ", "undo did not restore selection");
        Check(document.Redo(), "paste redo failed");

        document.SetSelection({ 1, 2 }, { 1, 2 });
        document.DuplicateLine();
        Check(document.GetBuffer().GetText() == "alpha beta\r\nsecond\r\nsecond\r\n", "duplicate line failed");
        Check(document.Undo(), "duplicate undo failed");
        Check(document.Save(), "save failed");
        Check(Read(path) == original, "save did not preserve BOM and CRLF");
        Check(!document.IsDirty(), "saved document is dirty");

        document.InsertText("x", true);
        document.InsertText("y", true);
        Check(document.Undo(), "typing undo failed");
        Check(!document.IsDirty(), "undo did not return to saved state");
        Check(document.Redo(), "typing redo failed");
        Check(document.IsDirty(), "redo did not restore dirty state");

        document.SetSelection({ 0, 0 }, { 0, 0 });
        document.InsertText("a", true);
        document.MoveCaret(Movement::Left, false, 10);
        document.MoveCaret(Movement::Right, false, 10);
        document.InsertText("b", true);
        Check(document.Undo(), "undo after navigation failed");
        Check(document.GetBuffer().Line(0).substr(0, 1) == "a", "typing merged across navigation");

        const std::string lineBeforeUtf8 = document.GetBuffer().Line(1);
        document.SetSelection({ 1, 0 }, { 1, 0 });
        document.InsertText("\xC3\xA9");
        document.MoveCaret(Movement::Left, true, 10);
        Check(document.GetSelectedText() == "\xC3\xA9", "UTF-8 character selection failed");
        document.Backspace(false);
        Check(document.GetBuffer().Line(1) == lineBeforeUtf8, "UTF-8 deletion failed");

        EditorDocument other;
        Check(other.Load(path), "second document load failed");
        Check(!other.IsDirty() && document.IsDirty(), "documents do not have independent state");
    }

    void TestOpener(const std::filesystem::path& path)
    {
        int calls = 0;
        auto opener = Diligent::FileOpeners::CreateScriptFileOpener(
            [&](const std::filesystem::path& opened) { if (opened == path) ++calls; });
        opener(path);
        opener(path.parent_path() / "not-a-script.txt");
        Check(calls == 1, "script opener did not filter extensions");
    }

    void TestLayout()
    {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = { 800, 600 };
        io.DeltaTime = 1.0f / 60.0f;
        unsigned char* pixels = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        ImGui::NewFrame();
        const std::string line = "long line with many characters and spaces";
        const LineLayout layout(line, 4, MeasureText(" ", nullptr));
        for (size_t i = 0; i <= line.size(); ++i)
        {
            const float expected = MeasureText(line.data(), line.data() + i);
            Check(std::abs(layout.ColumnToX(static_cast<int>(i)) - expected) < 0.01f, "caret width drift");
            Check(layout.XToColumn(expected) == static_cast<int>(i), "mouse hit test drift");
        }
        const std::string withTab = "a\tb";
        const LineLayout tabLayout(withTab, 4, MeasureText(" ", nullptr));
        Check(tabLayout.XToColumn(tabLayout.ColumnToX(2)) == 2, "tab hit test failed");
        ImGui::EndFrame();

        const EditorKeyBindings bindings = EditorKeyBindings::CreateDefault();
        io.AddKeyEvent(ImGuiMod_Ctrl, true);
        io.AddKeyEvent(ImGuiKey_S, true);
        ImGui::NewFrame();
        const auto commands = bindings.Poll();
        Check(commands.size() == 1 && commands.front().id == CommandId::Save, "Ctrl+S binding failed");
        ImGui::EndFrame();
        ImGui::DestroyContext();
    }
}

int main()
{
    const auto path = std::filesystem::temp_directory_path() /
        ("anito-script-editor-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".ascript");
    try
    {
        TestDocument(path);
        TestOpener(path);
        TestLayout();
        std::filesystem::remove(path);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
