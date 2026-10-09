#include "Health.hpp"

#include "PropertyDrawers/event_drawer.hpp"
#include "HierarchyManager.hpp"
#include "HierarchyObject.hpp"
#include "Components/Transform.hpp"
#include "GUIManager.hpp"
#include "imgui.h"
#include "../../../AppState.hpp"

#include <algorithm>

#include <glm/glm.hpp>

Health::Health(gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("Health", owner) {}

float Health::GetCurrentHealth() {
    if (m_currentHealth < 0.0f) {
        m_currentHealth = m_maxHealth;
    }
    return m_currentHealth;
}

bool Health::TakeDamage(float amount) {
    if (m_depleted || amount <= 0.0f) {
        return false;
    }

    m_currentHealth = std::max(0.0f, GetCurrentHealth() - amount);
    if (m_currentHealth > 0.0f) {
        return false;
    }

    m_depleted = true;
    m_onDepleted.Invoke();
    return true;
}

void Health::OnGUI_ReleaseEvent(float /*deltaTime*/) {
    if (!m_showHealthBar || m_depleted) {
        return;
    }

    HierarchyObject* owner = GetOwner().GetPtr();
    Transform* transform = owner ? owner->GetTransform() : nullptr;
    HierarchyManager& hierarchy = HierarchyManager::GetInstance();
    if (!transform || !hierarchy.GetMainCamera()) {
        return;
    }

    glm::mat4 view(1.0f);
    glm::mat4 proj(1.0f);
    if (!hierarchy.GetMainCameraMatrices(view, proj)) {
        return;
    }

    ImVec2 viewportPos(0.0f, 0.0f);
    ImVec2 viewportSize = ImGui::GetIO().DisplaySize;
    if (!AppState::isReleaseBuild) {
        const auto& gui = Diligent::GUIManager::GetInstance();
        viewportPos = gui.GetGameViewportPos();
        viewportSize = gui.GetGameViewportSize();
    }
    if (viewportSize.x <= 1.0f || viewportSize.y <= 1.0f) {
        return;
    }

    const glm::vec4 clip = proj * view * glm::vec4(transform->GetPosition() + glm::vec3(0.0f, m_barHeightOffset, 0.0f), 1.0f);
    if (clip.w <= 0.0001f) {
        return;
    }
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f || ndc.z < 0.0f || ndc.z > 1.0f) {
        return;
    }

    const float centerX = viewportPos.x + (ndc.x * 0.5f + 0.5f) * viewportSize.x;
    const float centerY = viewportPos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * viewportSize.y;
    const float barHeight = 6.0f;
    const float fraction = std::clamp(GetCurrentHealth() / std::max(m_maxHealth, 0.0001f), 0.0f, 1.0f);

    const ImVec2 topLeft(centerX - m_barWidth * 0.5f, centerY - barHeight * 0.5f);
    const ImVec2 bottomRight(centerX + m_barWidth * 0.5f, centerY + barHeight * 0.5f);

    // Foreground list so the bar also shows inside the editor's game panel; clipped to the game view.
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    drawList->PushClipRect(viewportPos, ImVec2(viewportPos.x + viewportSize.x, viewportPos.y + viewportSize.y), true);
    drawList->AddRectFilled(topLeft, bottomRight, IM_COL32(20, 20, 20, 200));
    drawList->AddRectFilled(topLeft, ImVec2(topLeft.x + m_barWidth * fraction, bottomRight.y),
        IM_COL32(static_cast<int>(255 * (1.0f - fraction)), static_cast<int>(220 * fraction), 40, 255));
    drawList->AddRect(topLeft, bottomRight, IM_COL32(0, 0, 0, 255));
    drawList->PopClipRect();
}

void Health::Heal(float amount) {
    if (m_depleted || amount <= 0.0f) {
        return;
    }
    m_currentHealth = std::min(m_maxHealth, GetCurrentHealth() + amount);
}
