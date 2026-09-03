#include <imgui.h>

inline void ApplyTheme()
{
    ImGui::GetIO().Fonts->AddFontFromFileTTF("Assets/Font/Inter/Inter-VariableFont_opsz,wght.ttf", 15.0f);

    ImGuiStyle &style = ImGui::GetStyle();

    // --- Style Settings ---
    style.Alpha = 1.0f;
    style.DisabledAlpha = 0.6f;
    style.WindowPadding = ImVec2(15.0f, 10.1f);
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowMinSize = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.ChildRounding = 5.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 5.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(4.0f, 3.0f);
    style.FrameRounding = 4.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.IndentSpacing = 21.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize = 12.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabMinSize = 10.0f;
    style.GrabRounding = 10.0f;
    style.TabRounding = 4.0f;
    style.TabBorderSize = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Left;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
    style.WindowMenuButtonPosition = ImGuiDir_None;

    // --- Color Mapping ---
    style.Colors[ImGuiCol_Text] = ImVec4(0.7686f, 0.7686f, 0.7686f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.7686f, 0.7686f, 0.7686f, 1.0f);

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.235f, 0.235f, 0.235f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.196f, 0.196f, 0.196f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.235f, 0.235f, 0.235f, 1.0f);

    style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.165f, 0.165f, 0.165f, 1.0f);

    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.165f, 0.165f, 0.165f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.404f, 0.404f, 0.404f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.275f, 0.376f, 0.486f, 0.698f);

    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.063f, 0.063f, 0.063f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.063f, 0.063f, 0.063f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.063f, 0.063f, 0.063f, 1.0f);

    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.063f, 0.063f, 0.063f, 1.0f);

    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.165f, 0.165f, 0.165f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.345f, 0.345f, 0.345f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.404f, 0.404f, 0.404f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.275f, 0.376f, 0.486f, 0.698f);

    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.8431f, 0.8431f, 0.8431f, 1.0f);

    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.6000f, 0.6000f, 0.6000f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.9176f, 0.9176f, 0.9176f, 1.0f);

    style.Colors[ImGuiCol_Button] = ImVec4(0.345f, 0.345f, 0.345f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.504f, 0.504f, 0.504f, 0.698f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.275f, 0.376f, 0.486f, 0.698f);

    style.Colors[ImGuiCol_Header] = ImVec4(0.196f, 0.196f, 0.196f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.275f, 0.376f, 0.486f, 0.698f);

    style.Colors[ImGuiCol_Separator] = ImVec4(0.122f, 0.122f, 0.122f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.122f, 0.122f, 0.122f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.122f, 0.122f, 0.122f, 1.0f);

    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.063f, 0.063f, 0.063f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.063f, 0.063f, 0.063f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.063f, 0.063f, 0.063f, 1.0f);

    style.Colors[ImGuiCol_Tab] = ImVec4(0.063f, 0.063f, 0.063f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.235f, 0.235f, 0.235f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.063f, 0.063f, 0.063f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.235f, 0.235f, 0.235f, 1.0f);

    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.486f, 0.095f, 0.095f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.486f, 0.095f, 0.095f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.486f, 0.095f, 0.095f, 0.75f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.486f, 0.095f, 0.095f, 0.75f);

    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.176f, 0.176f, 0.176f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.196f, 0.196f, 0.196f, 0.67f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.176f, 0.176f, 0.176f, 0.67f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.176f, 0.176f, 0.176f, 1.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.196f, 0.196f, 0.196f, 1.0f);

    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.275f, 0.376f, 0.486f, 0.549f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.267f, 0.267f, 0.267f, 1.0f);

    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.1647f, 0.5176f, 0.8235f, 0.45f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.1647f, 0.5176f, 0.8235f, 0.85f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.25f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
}