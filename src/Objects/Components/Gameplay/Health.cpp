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

namespace {
constexpr float kHealthBarVisibleDuration = 3.0f;
}

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

    const float previousHealth = GetCurrentHealth();
    m_currentHealth = std::max(0.0f, previousHealth - amount);
    if (m_currentHealth != previousHealth) {
        m_healthBarVisibleTime = kHealthBarVisibleDuration;
    }
    if (m_currentHealth > 0.0f) {
        return false;
    }

    m_depleted = true;
    m_onDepleted.Invoke();
    return true;
}

void Health::OnGUI_ReleaseEvent(float deltaTime) {
    m_healthBarVisibleTime = std::max(0.0f, m_healthBarVisibleTime - std::max(deltaTime, 0.0f));
    if (!m_showHealthBar || m_depleted || m_healthBarVisibleTime <= 0.0f) {
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
    const float barHeight = 4.0f;
    const float fraction = std::clamp(GetCurrentHealth() / std::max(m_maxHealth, 0.0001f), 0.0f, 1.0f);

    const ImVec2 topLeft(centerX - m_barWidth * 0.5f, centerY - barHeight * 0.5f);
    const ImVec2 bottomRight(centerX + m_barWidth * 0.5f, centerY + barHeight * 0.5f);

    // Foreground list so the bar also shows inside the editor's game panel; clipped to the game view.
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    drawList->PushClipRect(viewportPos, ImVec2(viewportPos.x + viewportSize.x, viewportPos.y + viewportSize.y), true);
    drawList->AddRectFilled(topLeft, bottomRight, IM_COL32(18, 20, 22, 145));
    const ImU32 healthColor = fraction < 0.25f ? IM_COL32(185, 92, 82, 220)
        : fraction < 0.6f ? IM_COL32(190, 157, 88, 220)
        : IM_COL32(112, 166, 126, 220);
    drawList->AddRectFilled(topLeft, ImVec2(topLeft.x + m_barWidth * fraction, bottomRight.y),
        healthColor);
    drawList->AddRect(topLeft, bottomRight, IM_COL32(0, 0, 0, 150));
    drawList->PopClipRect();
}

void Health::Heal(float amount) {
    if (m_depleted || amount <= 0.0f) {
        return;
    }
    const float previousHealth = GetCurrentHealth();
    m_currentHealth = std::min(m_maxHealth, previousHealth + amount);
    if (m_currentHealth != previousHealth) {
        m_healthBarVisibleTime = kHealthBarVisibleDuration;
    }
}
