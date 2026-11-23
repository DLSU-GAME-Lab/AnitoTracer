#include "DockLayoutScaler.hpp"

#include <imgui_internal.h>
#include <algorithm>

namespace
{
    bool ContainsCentralNode(const ImGuiDockNode* node)
    {
        return node->IsCentralNode() ||
            (node->IsSplitNode() &&
                (ContainsCentralNode(node->ChildNodes[0]) || ContainsCentralNode(node->ChildNodes[1])));
    }

    float InitialRatio(const ImGuiDockNode* node, float available)
    {
        const auto* first = node->ChildNodes[0];
        const auto* second = node->ChildNodes[1];
        const int axis = node->SplitAxis;
        if (first->IsVisible && second->IsVisible && first->Size[axis] > 0.0f && second->Size[axis] > 0.0f)
            return first->Size[axis] / (first->Size[axis] + second->Size[axis]);

        const float firstRef = first->SizeRef[axis];
        const float secondRef = second->SizeRef[axis];
        // ImGui keeps the non-central child at SizeRef and gives the central child the remainder.
        if (available > 0.0f)
        {
            if (ContainsCentralNode(second) && firstRef > 0.0f)
                return std::clamp(firstRef / available, 0.0f, 1.0f);
            if (ContainsCentralNode(first) && secondRef > 0.0f)
                return std::clamp(1.0f - secondRef / available, 0.0f, 1.0f);
        }
        return firstRef > 0.0f && secondRef > 0.0f ? firstRef / (firstRef + secondRef) : 0.5f;
    }
}

void DockLayoutScaler::Prepare(ImGuiID dockspaceId, ImVec2 size)
{
    ImGuiDockNode* root = ImGui::DockBuilderGetNode(dockspaceId);
    if (!root || root->Size.x <= 0.0f || root->Size.y <= 0.0f)
    {
        m_Splits.clear();
        m_Resizing = false;
        return;
    }

    m_Resizing = root->Size.x != size.x || root->Size.y != size.y;
    PrepareNode(root, root->Size, size);
    for (auto it = m_Splits.begin(); it != m_Splits.end();)
    {
        if (it->second.lastSeenFrame != ImGui::GetFrameCount())
            it = m_Splits.erase(it);
        else
            ++it;
    }
}

void DockLayoutScaler::PrepareNode(ImGuiDockNode* node, ImVec2 oldSize, ImVec2 newSize)
{
    if (!node->IsSplitNode())
        return;

    auto* first = node->ChildNodes[0];
    auto* second = node->ChildNodes[1];
    const int axis = node->SplitAxis;
    const float separator = ImGui::GetStyle().DockingSeparatorSize;
    const float oldAvailable = std::max(oldSize[axis] - separator, 0.0f);
    const float newAvailable = std::max(newSize[axis] - separator, 0.0f);
    Split& split = m_Splits[node->ID];
    if (split.children[0] != first->ID || split.children[1] != second->ID || split.axis != axis)
    {
        split = {};
        split.children[0] = first->ID;
        split.children[1] = second->ID;
        split.axis = axis;
        split.ratio = InitialRatio(node, oldAvailable);
    }
    split.lastSeenFrame = ImGui::GetFrameCount();

    ImVec2 oldFirst = oldSize, oldSecond = oldSize;
    ImVec2 newFirst = newSize, newSecond = newSize;
    oldFirst[axis] = oldAvailable * split.ratio;
    oldSecond[axis] = oldAvailable - oldFirst[axis];
    newFirst[axis] = newAvailable * split.ratio;
    newSecond[axis] = newAvailable - newFirst[axis];

    if (m_Resizing && newAvailable > 0.0f && newFirst[axis] > 0.0f && newSecond[axis] > 0.0f)
    {
        first->SizeRef[axis] = newFirst[axis];
        second->SizeRef[axis] = newSecond[axis];
    }

    PrepareNode(first, oldFirst, newFirst);
    PrepareNode(second, oldSecond, newSecond);
}

void DockLayoutScaler::Finish(ImGuiID dockspaceId)
{
    if (ImGuiDockNode* root = ImGui::DockBuilderGetNode(dockspaceId))
        FinishNode(root);
}

void DockLayoutScaler::FinishNode(ImGuiDockNode* node)
{
    if (!node->IsSplitNode())
        return;

    auto* first = node->ChildNodes[0];
    auto* second = node->ChildNodes[1];
    auto it = m_Splits.find(node->ID);
    if (it != m_Splits.end() && it->second.children[0] == first->ID && it->second.children[1] == second->ID &&
        first->IsVisible && second->IsVisible)
    {
        Split& split = it->second;
        const float firstSize = first->Size[split.axis];
        const float secondSize = second->Size[split.axis];
        if (firstSize > 0.0f && secondSize > 0.0f)
        {
            const bool changed = split.hasLastSizes &&
                (firstSize != split.lastSizes[0] || secondSize != split.lastSizes[1]);
            // Keep the intended ratio through rounding and minimum-size clamping during resize.
            if (changed && !m_Resizing)
                split.ratio = firstSize / (firstSize + secondSize);
            split.lastSizes[0] = firstSize;
            split.lastSizes[1] = secondSize;
            split.hasLastSizes = true;
        }
    }
    FinishNode(first);
    FinishNode(second);
}

void DockLayoutScaler::Reset()
{
    m_Splits.clear();
    m_Resizing = false;
}
