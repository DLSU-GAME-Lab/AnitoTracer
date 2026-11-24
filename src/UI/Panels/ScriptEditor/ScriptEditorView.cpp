#include "ScriptEditorView.hpp"

#include "CaretNavigator.hpp"
#include "LineLayout.hpp"
#include "Utf8.hpp"

#include "imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace Diligent::ScriptEditor
{
    namespace
    {
        constexpr float kCaretBlinkPeriod = 1.06f;
        constexpr float kGutterPadding = 8.0f;
        constexpr float kScrollMargin = 24.0f;
        constexpr int kMaxCompletionRows = 6;

        /// Frame-constant measurements shared by the drawing and input helpers.
        struct Metrics
        {
            float lineHeight = 0.0f;
            float spaceWidth = 0.0f;
            float gutterWidth = 0.0f;
            ImVec2 origin;   ///< Screen position of the top-left of line 0 (including the gutter).
            ImVec2 viewSize; ///< Visible size of the code region.
        };

        LineLayout Layout(const EditorDocument& document, int line, const Metrics& metrics)
        {
            return LineLayout(document.GetBuffer().Line(line), EditorDocument::TabSize, metrics.spaceWidth);
        }

        /// Marks the frame as wanting text input so global shortcuts (Ctrl+S, Ctrl+Z...) stand down.
        void ClaimTextInput()
        {
            if (ImGuiContext* context = ImGui::GetCurrentContext())
                context->PlatformImeData.WantTextInput = true;
        }

        void RefreshWidthCache(const EditorDocument& document, EditorViewState& state, const Metrics& metrics)
        {
            if (state.measuredRevision == document.GetRevision() &&
                state.measuredFont == ImGui::GetFont() && state.measuredFontSize == ImGui::GetFontSize()) return;
            state.measuredRevision = document.GetRevision();
            state.measuredFont = ImGui::GetFont();
            state.measuredFontSize = ImGui::GetFontSize();

            const TextBuffer& buffer = document.GetBuffer();
            state.maxLineWidth = 0.0f;
            for (int i = 0; i < buffer.LineCount(); ++i)
                state.maxLineWidth = std::max(state.maxLineWidth, Layout(document, i, metrics).GetWidth());
        }

        TextPosition HitTest(const EditorDocument& document, const Metrics& metrics, const ImVec2& mouse)
        {
            const int lineCount = document.GetBuffer().LineCount();
            const int line = std::clamp(static_cast<int>(std::floor((mouse.y - metrics.origin.y) / metrics.lineHeight)), 0, lineCount - 1);
            const float x = mouse.x - metrics.origin.x - metrics.gutterWidth;
            return { line, Layout(document, line, metrics).XToColumn(x) };
        }

        void ScrollCaretIntoView(const EditorDocument& document, const Metrics& metrics)
        {
            const TextPosition caret = document.GetSelection().caret;
            const float scrollbar = ImGui::GetStyle().ScrollbarSize;
            const float viewHeight = metrics.viewSize.y - scrollbar;
            const float viewWidth = metrics.viewSize.x - scrollbar;

            const float top = caret.line * metrics.lineHeight;
            const float scrollY = ImGui::GetScrollY();
            if (top < scrollY) ImGui::SetScrollY(top);
            else if (top + metrics.lineHeight > scrollY + viewHeight) ImGui::SetScrollY(top + metrics.lineHeight - viewHeight);

            const float x = metrics.gutterWidth + Layout(document, caret.line, metrics).ColumnToX(caret.column);
            const float scrollX = ImGui::GetScrollX();
            if (x < scrollX + metrics.gutterWidth + kScrollMargin)
                ImGui::SetScrollX(std::max(0.0f, x - metrics.gutterWidth - kScrollMargin));
            else if (x > scrollX + viewWidth - kScrollMargin)
                ImGui::SetScrollX(x - viewWidth + kScrollMargin);
        }

        ImU32 TokenColor(ScriptLanguage::TokenKind kind)
        {
            switch (kind)
            {
            case ScriptLanguage::TokenKind::Keyword:    return IM_COL32(197, 134, 192, 255);
            case ScriptLanguage::TokenKind::Type:       return IM_COL32(78, 201, 176, 255);
            case ScriptLanguage::TokenKind::Number:     return IM_COL32(181, 206, 168, 255);
            case ScriptLanguage::TokenKind::String:     return IM_COL32(206, 145, 120, 255);
            case ScriptLanguage::TokenKind::Comment:    return IM_COL32(106, 153, 85, 255);
            case ScriptLanguage::TokenKind::Annotation: return IM_COL32(220, 220, 170, 255);
            case ScriptLanguage::TokenKind::Operator:   return IM_COL32(212, 212, 212, 255);
            default:                                    return ImGui::GetColorU32(ImGuiCol_Text);
            }
        }

        std::vector<std::size_t> LineStarts(const std::string& source)
        {
            std::vector<std::size_t> starts(1, 0);
            for (std::size_t i = 0; i < source.size(); ++i)
                if (source[i] == '\n') starts.push_back(i + 1);
            return starts;
        }

        void DrawCodeRange(ImDrawList& drawList, const std::string& line, const LineLayout& layout,
                           int begin, int end, ImU32 color, float left, float y)
        {
            const std::vector<int>& offsets = layout.GetOffsets();
            const std::vector<float>& xs = layout.GetXs();
            int runStart = begin;
            for (std::size_t i = 0; i + 1 < offsets.size(); ++i)
            {
                const int offset = offsets[i];
                const int next = offsets[i + 1];
                if (offset < begin || next > end) continue;
                if (line[offset] == '\t')
                {
                    if (runStart < offset)
                        drawList.AddText({ left + layout.ColumnToX(runStart), y }, color,
                                         line.c_str() + runStart, line.c_str() + offset);
                    runStart = next;
                }
            }
            if (runStart < end)
                drawList.AddText({ left + layout.ColumnToX(runStart), y }, color,
                                 line.c_str() + runStart, line.c_str() + end);
        }

        void DrawHighlightedLine(ImDrawList& drawList, const std::string& line, const LineLayout& layout,
                                 const std::vector<ScriptLanguage::SyntaxToken>& tokens,
                                 std::size_t lineStart, float left, float y)
        {
            std::vector<int> boundaries{ 0, static_cast<int>(line.size()) };
            for (const auto& token : tokens)
            {
                if (token.end <= lineStart || token.begin >= lineStart + line.size()) continue;
                boundaries.push_back(std::clamp(
                    static_cast<int>(token.begin > lineStart ? token.begin - lineStart : 0),
                    0, static_cast<int>(line.size())));
                boundaries.push_back(std::clamp(
                    static_cast<int>(std::min(token.end, lineStart + line.size()) - lineStart),
                    0, static_cast<int>(line.size())));
            }
            std::sort(boundaries.begin(), boundaries.end());
            boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());

            for (std::size_t i = 0; i + 1 < boundaries.size(); ++i)
            {
                const int begin = boundaries[i];
                const int end = boundaries[i + 1];
                if (begin == end) continue;
                ScriptLanguage::TokenKind kind = ScriptLanguage::TokenKind::Plain;
                for (const auto& token : tokens)
                {
                    if (lineStart + static_cast<std::size_t>(begin) >= token.begin &&
                        lineStart + static_cast<std::size_t>(end) <= token.end)
                    {
                        kind = token.kind;
                        break;
                    }
                }
                DrawCodeRange(drawList, line, layout, begin, end, TokenColor(kind), left, y);
            }
        }

        void DrawLines(ImDrawList& drawList, const EditorDocument& document, const EditorViewState& state,
                      const Metrics& metrics, bool focused, float contentWidth,
                      const std::vector<std::size_t>& lineStarts)
        {
            const TextBuffer& buffer = document.GetBuffer();
            const TextSelection& selection = document.GetSelection();
            const TextRange range = selection.Range();

            const int firstLine = std::max(0, static_cast<int>(ImGui::GetScrollY() / metrics.lineHeight));
            const int lastLine = std::min(buffer.LineCount() - 1, firstLine + static_cast<int>(metrics.viewSize.y / metrics.lineHeight) + 2);

            const ImU32 selectionColor = ImGui::GetColorU32(ImGuiCol_TextSelectedBg);
            const ImU32 currentLineColor = IM_COL32(255, 255, 255, 14);
            const float textLeft = metrics.origin.x + metrics.gutterWidth;
            const bool caretVisible = focused &&
                std::fmod(static_cast<float>(ImGui::GetTime() - state.lastInteractionTime), kCaretBlinkPeriod) < kCaretBlinkPeriod * 0.5f;

            for (int line = firstLine; line <= lastLine; ++line)
            {
                const std::string& text = buffer.Line(line);
                const LineLayout layout(text, EditorDocument::TabSize, metrics.spaceWidth);
                const float y = metrics.origin.y + line * metrics.lineHeight;

                if (!selection.HasSelection() && line == selection.caret.line)
                    drawList.AddRectFilled({ textLeft, y }, { metrics.origin.x + contentWidth, y + metrics.lineHeight }, currentLineColor);

                if (selection.HasSelection() && line >= range.begin.line && line <= range.end.line)
                {
                    const int from = line == range.begin.line ? range.begin.column : 0;
                    const int to = line == range.end.line ? range.end.column : static_cast<int>(text.size());
                    float x0 = layout.ColumnToX(from);
                    float x1 = layout.ColumnToX(to);
                    if (line < range.end.line) x1 += metrics.spaceWidth; // Shows the selected line break.
                    drawList.AddRectFilled({ textLeft + x0, y }, { textLeft + x1, y + metrics.lineHeight }, selectionColor);
                }

                DrawHighlightedLine(drawList, text, layout, state.syntaxTokens,
                                    lineStarts[static_cast<std::size_t>(line)], textLeft, y);

                if (caretVisible && line == selection.caret.line)
                {
                    const float x = textLeft + layout.ColumnToX(selection.caret.column);
                    drawList.AddLine({ x, y }, { x, y + metrics.lineHeight }, TokenColor(ScriptLanguage::TokenKind::Plain), 1.5f);
                }
            }
        }

        void DrawCompletion(ImDrawList& drawList, const EditorDocument& document,
                            const EditorViewState& state, const Metrics& metrics, bool focused)
        {
            if (!focused || document.GetSelection().HasSelection() || !state.completion.Selected()) return;

            const std::string& selected = *state.completion.Selected();
            if (selected.size() <= state.completion.prefix.size()) return;
            const std::string suffix = selected.substr(state.completion.prefix.size());
            const TextPosition caret = document.GetSelection().caret;
            const float x = metrics.origin.x + metrics.gutterWidth +
                Layout(document, caret.line, metrics).ColumnToX(caret.column);
            const float y = metrics.origin.y + caret.line * metrics.lineHeight;
            drawList.AddText({ x, y }, IM_COL32(140, 140, 140, 190), suffix.c_str());

            const std::size_t rows = std::min(kMaxCompletionRows, static_cast<int>(state.completion.words.size()));
            float width = 0.0f;
            for (std::size_t i = 0; i < rows; ++i)
                width = std::max(width, ImGui::CalcTextSize(state.completion.words[i].c_str()).x);
            width += 52.0f;
            const float popupX = std::min(x, metrics.origin.x + metrics.viewSize.x - width - 4.0f);
            float popupY = y + metrics.lineHeight;
            const float popupHeight = rows * metrics.lineHeight + 4.0f;
            if (popupY + popupHeight > metrics.origin.y + metrics.viewSize.y)
                popupY = y - popupHeight;
            drawList.AddRectFilled({ popupX, popupY }, { popupX + width, popupY + popupHeight },
                                   IM_COL32(40, 40, 40, 245), 3.0f);
            drawList.AddRect({ popupX, popupY }, { popupX + width, popupY + popupHeight },
                             IM_COL32(90, 90, 90, 255), 3.0f);
            for (std::size_t i = 0; i < rows; ++i)
            {
                const float rowY = popupY + 2.0f + i * metrics.lineHeight;
                if (i == 0)
                    drawList.AddRectFilled({ popupX + 2.0f, rowY }, { popupX + width - 2.0f, rowY + metrics.lineHeight },
                                           IM_COL32(62, 62, 66, 255));
                drawList.AddText({ popupX + 7.0f, rowY }, TokenColor(ScriptLanguage::TokenKind::Plain),
                                 state.completion.words[i].c_str());
                if (i == 0)
                    drawList.AddText({ popupX + width - 35.0f, rowY }, ImGui::GetColorU32(ImGuiCol_TextDisabled), "Tab");
            }
        }

        void DrawGutter(ImDrawList& drawList, const EditorDocument& document, const Metrics& metrics)
        {
            const TextBuffer& buffer = document.GetBuffer();
            const ImVec2 windowPos = ImGui::GetWindowPos();
            const int firstLine = std::max(0, static_cast<int>(ImGui::GetScrollY() / metrics.lineHeight));
            const int lastLine = std::min(buffer.LineCount() - 1, firstLine + static_cast<int>(metrics.viewSize.y / metrics.lineHeight) + 2);

            drawList.AddRectFilled(windowPos, { windowPos.x + metrics.gutterWidth, windowPos.y + ImGui::GetWindowHeight() },
                                   ImGui::GetColorU32(ImGuiCol_ChildBg, 1.0f));

            for (int line = firstLine; line <= lastLine; ++line)
            {
                const std::string number = std::to_string(line + 1);
                const bool current = line == document.GetSelection().caret.line;
                const float width = ImGui::CalcTextSize(number.c_str()).x;
                const float y = metrics.origin.y + line * metrics.lineHeight;
                drawList.AddText({ windowPos.x + metrics.gutterWidth - kGutterPadding - width, y },
                                 ImGui::GetColorU32(current ? ImGuiCol_Text : ImGuiCol_TextDisabled), number.c_str());
            }
        }

        void HandleMouse(EditorDocument& document, EditorViewState& state, const Metrics& metrics)
        {
            const ImGuiIO& io = ImGui::GetIO();
            const ImVec2 windowPos = ImGui::GetWindowPos();
            const ImVec2 windowSize = ImGui::GetWindowSize();
            const float scrollbar = ImGui::GetStyle().ScrollbarSize;

            const bool overText = ImGui::IsWindowHovered() &&
                io.MousePos.x < windowPos.x + windowSize.x - scrollbar &&
                io.MousePos.y < windowPos.y + windowSize.y - scrollbar;
            if (overText) ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);

            if (overText && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                const TextPosition position = HitTest(document, metrics, io.MousePos);
                const int clicks = ImGui::GetMouseClickedCount(ImGuiMouseButton_Left);
                const TextBuffer& buffer = document.GetBuffer();

                if (clicks >= 3)
                    document.SelectRange({ { position.line, 0 }, { position.line, static_cast<int>(buffer.Line(position.line).size()) } });
                else if (clicks == 2)
                    document.SelectRange(CaretNavigator::WordRangeAt(buffer, position));
                else if (io.KeyShift)
                    document.SetSelection(document.GetSelection().anchor, position);
                else
                    document.SetSelection(position, position);

                state.dragging = clicks == 1;
            }

            if (!state.dragging) return;
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                state.dragging = false;
                return;
            }

            const float speed = 800.0f * io.DeltaTime;
            if (io.MousePos.y < windowPos.y) ImGui::SetScrollY(ImGui::GetScrollY() - speed);
            else if (io.MousePos.y > windowPos.y + windowSize.y) ImGui::SetScrollY(ImGui::GetScrollY() + speed);
            if (io.MousePos.x < windowPos.x + metrics.gutterWidth) ImGui::SetScrollX(ImGui::GetScrollX() - speed);
            else if (io.MousePos.x > windowPos.x + windowSize.x) ImGui::SetScrollX(ImGui::GetScrollX() + speed);

            document.SetSelection(document.GetSelection().anchor, HitTest(document, metrics, io.MousePos));
        }

        void HandleTextInput(EditorDocument& document)
        {
            const ImGuiIO& io = ImGui::GetIO();
            // Ctrl alone means a shortcut; Ctrl+Alt is AltGr on some layouts and does produce text.
            if (io.KeyCtrl && !io.KeyAlt) return;

            std::string text;
            for (const ImWchar character : io.InputQueueCharacters)
            {
                if (character < 32 || character == 127) continue;
                Utf8::Append(text, character);
            }
            document.InsertText(text, true);
        }
    }

    ScriptEditorView::ScriptEditorView()
        : m_KeyBindings(EditorKeyBindings::CreateDefault())
    {
    }

    void ScriptEditorView::Draw(EditorDocument& document, EditorViewState& state, EditorEnvironment& environment,
                                const ImVec2& size, bool takeFocus)
    {
        ImGui::PushID(&document);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(30, 30, 30, 255));
        ImGui::BeginChild("##code", size, ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNavInputs);

        if (takeFocus) ImGui::SetWindowFocus();
        const bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        Metrics metrics;
        metrics.lineHeight = ImGui::GetTextLineHeightWithSpacing();
        metrics.spaceWidth = MeasureText(" ", nullptr);
        const int digits = static_cast<int>(std::to_string(document.GetBuffer().LineCount()).size());
        metrics.gutterWidth = ImGui::CalcTextSize("0").x * static_cast<float>(std::max(digits, 3)) + kGutterPadding * 2.0f;
        metrics.viewSize = ImGui::GetContentRegionAvail();
        metrics.origin = ImGui::GetCursorScreenPos();
        environment.pageLines = std::max(1, static_cast<int>(metrics.viewSize.y / metrics.lineHeight) - 1);

        const std::string sourceBeforeInput = document.GetBuffer().GetText();
        if (state.syntaxRevision != document.GetRevision())
        {
            state.syntaxTokens = ScriptLanguage::Highlight(sourceBeforeInput);
            state.syntaxRevision = document.GetRevision();
        }

        // Input first so this frame already renders its result.
        bool acceptedCompletion = false;
        if (focused)
        {
            ClaimTextInput();
            if (!document.GetSelection().HasSelection() && state.completion.Selected() &&
                ImGui::IsKeyPressed(ImGuiKey_Tab, false))
            {
                const TextPosition caret = document.GetSelection().caret;
                document.SelectRange({
                    { caret.line, std::max(0, caret.column - static_cast<int>(state.completion.prefix.size())) },
                    caret
                });
                document.InsertText(*state.completion.Selected(), false);
                acceptedCompletion = true;
            }
            for (const EditorCommand& command : m_KeyBindings.Poll())
            {
                if (acceptedCompletion && command.id == CommandId::Tab) continue;
                m_Executor.Execute(command, document, environment);
            }
            HandleTextInput(document);
        }
        HandleMouse(document, state, metrics);

        RefreshWidthCache(document, state, metrics);
        const std::string source = document.GetBuffer().GetText();
        if (state.syntaxRevision != document.GetRevision())
        {
            state.syntaxTokens = ScriptLanguage::Highlight(source);
            state.syntaxRevision = document.GetRevision();
        }
        const auto lineStarts = LineStarts(source);
        if (!document.GetSelection().HasSelection())
        {
            const TextPosition caret = document.GetSelection().caret;
            const std::size_t cursor = lineStarts[static_cast<std::size_t>(caret.line)] + caret.column;
            const std::vector<std::string> projectWords = environment.completionWords ?
                environment.completionWords() : std::vector<std::string>{};
            state.completion = ScriptLanguage::Complete(source, cursor, projectWords);
        }
        else
        {
            state.completion = {};
        }
        const float contentWidth = metrics.gutterWidth + state.maxLineWidth + metrics.spaceWidth * 4.0f;
        ImGui::Dummy({ contentWidth, static_cast<float>(document.GetBuffer().LineCount()) * metrics.lineHeight });

        const TextPosition caret = document.GetSelection().caret;
        if (caret != state.lastCaret || document.GetRevision() != state.lastRevision)
        {
            state.lastCaret = caret;
            state.lastRevision = document.GetRevision();
            state.lastInteractionTime = ImGui::GetTime();
            if (!state.dragging) ScrollCaretIntoView(document, metrics);
        }

        ImDrawList& drawList = *ImGui::GetWindowDrawList();
        DrawLines(drawList, document, state, metrics, focused, std::max(contentWidth, metrics.viewSize.x),
                  lineStarts);
        DrawGutter(drawList, document, metrics);
        DrawCompletion(drawList, document, state, metrics, focused);

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopID();
    }
}
