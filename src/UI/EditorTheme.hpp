#pragma once
#include "imgui.h"
#include <string>

struct EditorTheme
{
    std::string EDITOR_FONT;
    std::string ICON_FONT;

    ImVec4 TEXT;
    //TextDisabled

    ImVec4 WINDOW_BG;
    ImVec4 CHILD_BG;
    ImVec4 POPUP_BG;

    ImVec4 BORDER;
    //BorderShadow

    ImVec4 FRAME_BG;
    ImVec4 FRAME_BG_HOVERED;
    ImVec4 FRAME_BG_ACTIVE;

    ImVec4 TITLE_BG;
    ImVec4 TITLE_BG_ACTIVE;
    ImVec4 TITLE_BG_COLLAPSED;

    ImVec4 MENU_BAR_BG;

    //Scrollbar BG
    //Scrollbar Grab
    //Scrollbar Grab Hovered
    //Scrollbar Grab Active

    //Checkmark

    //Slider Grab
    //Slider Grab Active
    ImVec4 BUTTON;
    ImVec4 BUTTON_HOVERED;
    ImVec4 BUTTON_ACTIVE;

    ImVec4 HEADER;
    ImVec4 HEADER_HOVERED;
    ImVec4 HEADER_ACTIVE;

    //Separator
    //Separator Hovered
    //Separator Active

    //Resize Grip
    //Resize Grip Hovered
    //Resize Grip Active

    ImVec4 TAB;
    ImVec4 TAB_HOVERED;
    ImVec4 TAB_ACTIVE;
    ImVec4 TAB_UNFOCUSED;
    ImVec4 TAB_UNFOCUSED_ACTIVE;

    //Plot Lines
    //Plot Lines Hovered
    //Plot Histogram
    //Plot Histogram Hovered

    //Table Header BG
    //Table Border Strong
    //Table Border Light
    //Table Row BG
    //Table Row BG ALT

    ImVec4 TEXT_SELECTED_BG;
    //Drag Drop Target

    //Nav Highlight
    //Nav Windowing Highlight
    //Nav Windowing Dim BG
    //Modal Window Dim BG
};

extern EditorTheme DarkTheme;

