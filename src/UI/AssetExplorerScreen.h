#pragma once

#include <imgui.h>
#include "AUIScreen.h"
#include "AssetManagement/Material.hpp"

class AssetExplorerScreen :
    public AUIScreen
{
    // Options
    // bool            ShowTypeOverlay = true;
    // bool            AllowSorting = true;
    // bool            AllowDragUnselected = false;
    // bool            AllowBoxSelect = true;
    // float           IconSize = 32.0f;
    // int             IconSpacing = 10;
    // int             IconHitSpacing = 4;         // Increase hit-spacing if you want to make it possible to clear or box-select from gaps. Some spacing is required to able to amend with Shift+box-select. Value is small in Explorer.
    // bool            StretchSpacing = true;
    //
    // // State
    // ImVector<Assets::Material*> Items;               // Our items
    // ExampleSelectionWithDeletion Selection;     // Our selection (ImGuiSelectionBasicStorage + helper funcs to handle deletion)
    // ImGuiID         NextItemId = 0;             // Unique identifier when creating new items
    // bool            RequestDelete = false;      // Deferred deletion request
    // bool            RequestSort = false;        // Deferred sort request
    // float           ZoomWheelAccum = 0.0f;      // Mouse wheel accumulator to handle smooth wheels better
    //
    // // Calculated sizes for layout, output of UpdateLayoutSizes(). Could be locals but our code is simpler this way.
    // ImVec2          LayoutItemSize;
    // ImVec2          LayoutItemStep;             // == LayoutItemSize + LayoutItemSpacing
    // float           LayoutItemSpacing = 0.0f;
    // float           LayoutSelectableSpacing = 0.0f;
    // float           LayoutOuterPadding = 0.0f;
    // int             LayoutColumnCount = 0;
    // int             LayoutLineCount = 0;
public:
    AssetExplorerScreen();

private:
    void AddItems(int count);
    void ClearItems();
    void UpdateLayoutSizes(float avail_width);

protected:
    void drawUI() override;
};

