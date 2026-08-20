#include "CameraProximityTrigger.hpp"

#include "PropertyDrawers/event_drawer.hpp"

#include "HierarchyManager.hpp"
#include "Components/Transform.hpp"
#include "Components/Camera.hpp"

#include <iostream>

CameraProximityTrigger::CameraProximityTrigger(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("CameraProximityTrigger", owner), m_transform(transform) {}

CameraProximityTrigger::~CameraProximityTrigger() = default;

void CameraProximityTrigger::OnUpdate(float /*deltaTime*/) {
    // 1. Fetch main camera and target object transform
    CameraComponent* mainCamera = HierarchyManager::GetInstance().GetMainCamera();
    if (!mainCamera) return;

    Transform* objTransform = m_transform;
    if (!objTransform && GetOwner().GetPtr()) {
        objTransform = GetOwner().GetPtr()->GetTransform();
    }
    if (!objTransform) return;

    Transform* cameraTransform = mainCamera->GetOwner().GetPtr()
        ? mainCamera->GetOwner().GetPtr()->GetTransform()
        : nullptr;
    if (!cameraTransform) return;

    // 2. Measure distance between main camera and this object
    glm::vec3 cameraPos = cameraTransform->GetPosition();
    glm::vec3 objectPos = objTransform->GetPosition();
    float distance = glm::distance(cameraPos, objectPos);

    // 3. Proximity state update and trigger invocation
    bool currentlyInRange = (distance <= m_triggerDistance);

    if (currentlyInRange && !m_isInRange) {
        m_isInRange = true;
        OnProximityEnter();
    }
    else if (!currentlyInRange && m_isInRange) {
        m_isInRange = false;
    }
}

void CameraProximityTrigger::OnProximityEnter() {
    std::cout << "CameraProximityTrigger: Camera entered threshold distance!" << std::endl;

    // Execute callback if assigned
    m_onProximityCallback.Invoke();

    // ==========================================
    // INSERT YOUR CUSTOM FUNCTION CALL HERE
    // ==========================================
}