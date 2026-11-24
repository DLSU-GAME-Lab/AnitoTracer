#include "DockLayoutScaler.hpp"

#include <imgui_internal.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{
    constexpr ImGuiID Dockspace = 0x12345678;
    constexpr ImGuiID Left = 0x00000001;
    constexpr ImGuiID Right = 0x00000002;
    constexpr ImGuiID Top = 0x00000003;
    constexpr ImGuiID Bottom = 0x00000004;
    constexpr ImGuiID Center = 0x00000005;
    constexpr ImGuiID TopRight = 0x00000006;

    const char* Layout = R"ini(
[Window][Left]
DockId=0x00000001,0
[Window][Center]
DockId=0x00000005,0
[Window][TopRight]
DockId=0x00000006,0
[Window][Bottom]
DockId=0x00000004,0
[Docking][Data]
DockSpace     ID=0x12345678 Pos=0,0 Size=1000,800 Split=X
  DockNode    ID=0x00000001 Parent=0x12345678 SizeRef=300,800 Selected=0x00000000
  DockNode    ID=0x00000002 Parent=0x12345678 SizeRef=698,800 Split=Y
    DockNode  ID=0x00000003 Parent=0x00000002 SizeRef=698,400 Split=X
      DockNode ID=0x00000005 Parent=0x00000003 SizeRef=698,400 CentralNode=1
      DockNode ID=0x00000006 Parent=0x00000003 SizeRef=200,400
    DockNode  ID=0x00000004 Parent=0x00000002 SizeRef=698,398
)ini";

    void Check(bool condition, const char* message)
    {
        if (!condition)
        {
            std::fprintf(stderr, "%s\n", message);
            std::exit(1);
        }
    }

    float Ratio(ImGuiID parent)
    {
        const auto* node = ImGui::DockBuilderGetNode(parent);
        Check(node && node->IsSplitNode(), "Missing split node");
        const float first = node->ChildNodes[0]->Size[node->SplitAxis];
        const float second = node->ChildNodes[1]->Size[node->SplitAxis];
        return first / (first + second);
    }

    void CheckNear(float actual, float expected, const char* message)
    {
        if (std::abs(actual - expected) > 0.012f)
        {
            std::fprintf(stderr, "%s: got %.4f, expected %.4f\n", message, actual, expected);
            std::exit(1);
        }
    }

    void Frame(DockLayoutScaler& scaler, float width, float height, bool showLeft = true)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(width, height);
        io.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        scaler.Prepare(Dockspace, viewport->WorkSize);
        ImGui::DockSpaceOverViewport(Dockspace, viewport, ImGuiDockNodeFlags_PassthruCentralNode);
        scaler.Finish(Dockspace);
        if (showLeft) { ImGui::Begin("Left"); ImGui::End(); }
        ImGui::Begin("Center"); ImGui::End();
        ImGui::Begin("TopRight"); ImGui::End();
        ImGui::Begin("Bottom"); ImGui::End();
        ImGui::Render();
    }
}

int main()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    unsigned char* pixels = nullptr;
    int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    DockLayoutScaler scaler;
    ImGui::LoadIniSettingsFromMemory(Layout);
    for (int i = 0; i < 4; ++i)
        Frame(scaler, 1600, 1200);
    CheckNear(Ratio(Dockspace), 300.0f / 998.0f, "Loaded horizontal split");
    CheckNear(Ratio(Right), 400.0f / 798.0f, "Loaded vertical split");
    CheckNear(Ratio(Top), 1.0f - 200.0f / 696.0f, "Loaded nested central split");

    const float horizontal = Ratio(Dockspace);
    const float vertical = Ratio(Right);
    const float nested = Ratio(Top);
    for (int i = 0; i < 100; ++i)
        Frame(scaler, 1600.0f + i, 1200.0f - i);
    CheckNear(Ratio(Dockspace), horizontal, "Continuous horizontal resize");
    CheckNear(Ratio(Right), vertical, "Continuous vertical resize");
    CheckNear(Ratio(Top), nested, "Continuous nested resize");
    Frame(scaler, 800, 600);
    CheckNear(Ratio(Dockspace), horizontal, "Shrink horizontal split");
    CheckNear(Ratio(Right), vertical, "Shrink vertical split");
    CheckNear(Ratio(Top), nested, "Shrink nested split");

    Frame(scaler, 120, 100);
    Frame(scaler, 120, 100);
    Frame(scaler, 1600, 1200);
    CheckNear(Ratio(Dockspace), horizontal, "Restore after minimum size");
    CheckNear(Ratio(Right), vertical, "Restore vertical after minimum size");
    CheckNear(Ratio(Top), nested, "Restore nested after minimum size");

    Frame(scaler, 1600, 1200, false);
    Frame(scaler, 1600, 1200, false);
    Frame(scaler, 1200, 900, false);
    Frame(scaler, 1200, 900);
    Frame(scaler, 1200, 900);
    CheckNear(Ratio(Dockspace), horizontal, "Reopened panel after resize");

    ImGuiDockNode* left = ImGui::DockBuilderGetNode(Left);
    io.AddMousePosEvent(left->Pos.x + left->Size.x, left->Pos.y + 100.0f);
    Frame(scaler, 1200, 900);
    io.AddMouseButtonEvent(0, true);
    Frame(scaler, 1200, 900);
    io.AddMousePosEvent(left->Pos.x + left->Size.x + 100.0f, left->Pos.y + 100.0f);
    Frame(scaler, 1200, 900);
    io.AddMouseButtonEvent(0, false);
    Frame(scaler, 1200, 900);
    const float adjusted = Ratio(Dockspace);
    Check(adjusted > horizontal + 0.05f, "Dragging the splitter did not adjust its ratio");
    const float adjustedVertical = Ratio(Right);
    const float adjustedNested = Ratio(Top);
    Frame(scaler, 1600, 1200);
    CheckNear(Ratio(Dockspace), adjusted, "Preserve manually adjusted split");
    CheckNear(Ratio(Right), adjustedVertical, "Preserve vertical split after dragging");
    CheckNear(Ratio(Top), adjustedNested, "Preserve nested split after dragging");

    std::string saved = ImGui::SaveIniSettingsToMemory();
    scaler.Reset();
    ImGui::LoadIniSettingsFromMemory(saved.c_str());
    for (int i = 0; i < 4; ++i)
        Frame(scaler, 1400, 1000);
    CheckNear(Ratio(Dockspace), adjusted, "Reloaded horizontal split");
    CheckNear(Ratio(Right), adjustedVertical, "Reloaded vertical split");
    CheckNear(Ratio(Top), adjustedNested, "Reloaded nested split");

    ImGui::DestroyContext();

    ImGui::CreateContext();
    ImGuiIO& defaultIo = ImGui::GetIO();
    defaultIo.IniFilename = nullptr;
    defaultIo.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    defaultIo.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    ImGui::LoadIniSettingsFromDisk(ANITO_DEFAULT_LAYOUT);
    scaler.Reset();
    const auto defaultFrame = [&scaler](float w, float h)
    {
        ImGuiIO& frameIo = ImGui::GetIO();
        frameIo.DisplaySize = ImVec2(w, h);
        frameIo.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        char hostName[32];
        ImFormatString(hostName, sizeof(hostName), "WindowOverViewport_%08X", viewport->ID);
        const ImGuiID id = ImHashStr("DockSpace", 0, ImHashStr(hostName));
        Check(id == 0x08BD597D, "Default layout dockspace ID changed");
        scaler.Prepare(id, viewport->WorkSize);
        ImGui::DockSpaceOverViewport(id, viewport, ImGuiDockNodeFlags_PassthruCentralNode);
        scaler.Finish(id);
        if (ImGui::BeginMainMenuBar())
            ImGui::EndMainMenuBar();
        for (const char* name : {"Inspector", "Hierarchy", "Game Camera", "Editor Camera", "File Explorer", "User Settings", "Profiler"})
        {
            ImGui::Begin(name);
            ImGui::End();
        }
        ImGui::Render();
    };
    for (int i = 0; i < 4; ++i)
        defaultFrame(1920, 1080);
    CheckNear(Ratio(0x08BD597D), 373.0f / 1262.0f, "Default layout horizontal split");
    CheckNear(Ratio(0x0000000E), 330.0f / 658.0f, "Default layout vertical split");
    CheckNear(Ratio(0x0000000D), 1.0f - 442.0f / 887.0f, "Default layout nested split");
    for (int i = 0; i < 4; ++i)
        defaultFrame(1264, 681);
    CheckNear(Ratio(0x08BD597D), 373.0f / 1262.0f, "Default layout resized horizontal split");
    CheckNear(Ratio(0x0000000E), 330.0f / 658.0f, "Default layout resized vertical split");
    CheckNear(Ratio(0x0000000D), 1.0f - 442.0f / 887.0f, "Default layout resized nested split");
    ImGui::DestroyContext();
    std::puts("Dock layout scaler tests passed");
    return 0;
}
