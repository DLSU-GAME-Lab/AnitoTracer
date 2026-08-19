#include "PickupComponent.hpp"

#include "HierarchyManager.hpp"
#include "Components/Transform.hpp"
#include "Components/Camera.hpp"

#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>

#include "Example/PlayerInput.hpp"

PickupComponent::PickupComponent(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner)
	: ComponentBase("PickupComponent", owner), m_transform(transform) {
	GBE_Init();
}

PickupComponent::~PickupComponent() = default;

void PickupComponent::OnUpdate(float /*deltaTime") */) {
	// 1. Fetch main camera and valid target transform
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

	// 2. Calculate Camera Position and Forward Vector (+Z in left-handed system)
	glm::vec3 cameraPos = cameraTransform->GetPosition();
	glm::vec3 cameraForward = cameraTransform->GetRotation() * glm::vec3(0.0f, 0.0f, 1.0f);

	// 3. Process Input Trigger toggle logic
	if (m_primaryPressedThisFrame) {
		m_primaryPressedThisFrame = false;

		std::cout << "PickupComponent: Primary input pressed. Current state: " << (m_isPickedUp ? "Picked Up" : "Not Picked Up") << std::endl;

		if (m_isPickedUp) {
			// Drop object at current location
			m_isPickedUp = false;
		}
		else {
			// Evaluate distance and dot product alignment
			glm::vec3 objectPos = objTransform->GetPosition();
			glm::vec3 toObject = objectPos - cameraPos;
			float distance = glm::length(toObject);

			if (distance <= m_maxPickupDistance && distance > 0.0001f) {
				glm::vec3 dirToObject = toObject / distance;
				float dot = glm::dot(cameraForward, dirToObject);

				// Check if camera is looking at the object within the angle threshold
				if (dot >= m_facingThreshold) {
					m_isPickedUp = true;
					m_holdDistance = distance;
				}
			}
		}
	}

	// 4. Drag object relative to camera forward vector while held
	if (m_isPickedUp) {
		glm::vec3 targetPosition = cameraPos + (cameraForward * m_holdDistance);
		objTransform->SetPosition(targetPosition);
	}
}

void PickupComponent::GBE_Init()
{
	std::string key = INPUTKEY_PRIMARY;
	key.append(":Down");

	SubscribeTo(key, [this](const std::unique_ptr<gbe::EventArgs>&) {
		m_primaryPressedThisFrame = true;
		});
}
