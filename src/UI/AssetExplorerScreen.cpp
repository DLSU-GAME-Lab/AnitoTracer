#include "AssetExplorerScreen.h"

// Functions
AssetExplorerScreen::AssetExplorerScreen() : AUIScreen("AssetExplorerScreen")
{
    //AddItems(10000);
}

void AssetExplorerScreen::AddItems(int count)
{
    // if (Items.Size == 0)
    //     NextItemId = 0;
    // Items.reserve(Items.Size + count);
    // for (int n = 0; n < count; n++, NextItemId++)
    //     Items.push_back(ExampleAsset(NextItemId, (NextItemId % 20) < 15 ? 0 : (NextItemId % 20) < 18 ? 1 : 2));
    // RequestSort = true;
}

void AssetExplorerScreen::ClearItems()
{
    // Items.clear();
    // Selection.Clear();
}

// Logic would be written in the main code BeginChild() and outputing to local variables.
// We extracted it into a function so we can call it easily from multiple places.
void AssetExplorerScreen::UpdateLayoutSizes(float avail_width)
{
    // // Layout: when not stretching: allow extending into right-most spacing.
    // LayoutItemSpacing = (float)IconSpacing;
    // if (StretchSpacing == false)
    //     avail_width += floorf(LayoutItemSpacing * 0.5f);
    //
    // // Layout: calculate number of icon per line and number of lines
    // LayoutItemSize = ImVec2(floorf(IconSize), floorf(IconSize));
    // LayoutColumnCount = ImMin((int)(avail_width / (LayoutItemSize.x + LayoutItemSpacing)), 1);
    // LayoutLineCount = (Items.Size + LayoutColumnCount - 1) / LayoutColumnCount;
    //
    // // Layout: when stretching: allocate remaining space to more spacing. Round before division, so item_spacing may be non-integer.
    // if (StretchSpacing && LayoutColumnCount > 1)
    //     LayoutItemSpacing = floorf(avail_width - LayoutItemSize.x * LayoutColumnCount) / LayoutColumnCount;
    //
    // LayoutItemStep = ImVec2(LayoutItemSize.x + LayoutItemSpacing, LayoutItemSize.y + LayoutItemSpacing);
    // LayoutSelectableSpacing = ImMax(floorf(LayoutItemSpacing) - IconHitSpacing, 0.0f);
    // LayoutOuterPadding = floorf(LayoutItemSpacing * 0.5f);
}

void AssetExplorerScreen::drawUI()
{
    ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
    if (int flags = UISettings::GlobalWindowFlags; ImGui::Begin("Example: Simple layout", &enabled, flags |= ImGuiWindowFlags_MenuBar))
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Close", "Ctrl+W")) { enabled = false; }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Left
        static int selected = 0;
        {
            ImGui::BeginChild("left pane", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);
            for (int i = 0; i < 100; i++)
            {
                // FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
                char label[128];
                sprintf(label, "MyObject %d", i);
                if (ImGui::Selectable(label, selected == i))
                    selected = i;
            }
            ImGui::EndChild();
        }
        ImGui::SameLine();

        // Right
        {
            ImGui::BeginGroup();
            ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
            ImGui::Text("MyObject: %d", selected);
            ImGui::Separator();
            if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("Description"))
                {
                    ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. ");
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Details"))
                {
                    ImGui::Text("ID: 0123456789");
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndChild();
            if (ImGui::Button("Revert")) {}
            ImGui::SameLine();
            if (ImGui::Button("Save")) {}
            ImGui::EndGroup();
        }
    }
    ImGui::End();
}
