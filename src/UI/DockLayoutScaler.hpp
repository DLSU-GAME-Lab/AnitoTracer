#pragma once

#include <imgui.h>
#include <unordered_map>

struct ImGuiDockNode;

class DockLayoutScaler
{
public:
    void Prepare(ImGuiID dockspaceId, ImVec2 size);
    void Finish(ImGuiID dockspaceId);
    void Reset();

private:
    struct Split
    {
        ImGuiID children[2] = {};
        int axis = -1;
        float ratio = 0.5f;
        float lastSizes[2] = {};
        int lastSeenFrame = -1;
        bool hasLastSizes = false;
    };

    void PrepareNode(ImGuiDockNode* node, ImVec2 oldSize, ImVec2 newSize);
    void FinishNode(ImGuiDockNode* node);

    std::unordered_map<ImGuiID, Split> m_Splits;
    bool m_Resizing = false;
};
