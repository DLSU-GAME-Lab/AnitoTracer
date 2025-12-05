#include "SceneCamera.h"
#include "AssetManagement/GameObjectManager.hpp"
#include "HotkeySystem/HotkeySystem.hpp"
#include "From-GDGRAP2/EventBroadcaster.h"

SceneCamera::SceneCamera(std::string name) : Camera(name)
{
	this->m_isSceneCamera = true;
	HotkeySystem::getInstance()->addListener(this);
}

SceneCamera::SceneCamera(const SceneCamera& other) : Camera(other),
    cameraMovingForward_(other.cameraMovingForward_),
    cameraMovingBackward_(other.cameraMovingBackward_),
    cameraMovingLeft_(other.cameraMovingLeft_),
    cameraMovingRight_(other.cameraMovingRight_),
    cameraMovingUp_(other.cameraMovingUp_),
    cameraMovingDown_(other.cameraMovingDown_),
    camSpedUp(other.camSpedUp),
    camSlowed(other.camSlowed),
    camSpeed_(other.camSpeed_),
    orbitSpeed_(other.orbitSpeed_),
    m_defaultPivotDistance(other.m_defaultPivotDistance),
    orbitYaw_(other.orbitYaw_),
    orbitPitch_(other.orbitPitch_),
    orbitPivot_(other.orbitPivot_),
    mousePosX_(other.mousePosX_),
    mousePosY_(other.mousePosY_),
	m_currentMode(other.m_currentMode)
{
    HotkeySystem::getInstance()->addListener(this);
}

SceneCamera::~SceneCamera()
{
	HotkeySystem::getInstance()->removeListener(this);
}

GameObject::GameObjectPtr SceneCamera::Clone() const
{
	return std::make_unique<SceneCamera>(*this);
}

bool SceneCamera::OnCursorPosition(double xpos, double ypos)
{
	const float deltaX = static_cast<float>(xpos - mousePosX_);
	const float deltaY = static_cast<float>(ypos - mousePosY_);

	mousePosX_ = xpos;
	mousePosY_ = ypos;

	switch (m_currentMode)
	{
	case FPS:
		HandleFPSRotation(deltaX, deltaY);
		return true;
	case PAN:
	{
		float multiplier = 1.0f;
		if (camSpedUp) multiplier = 2.0f;
		if (camSlowed) multiplier = 0.25f;
		HandlePan(deltaX * multiplier, deltaY * multiplier);
	}
	return true;
	case ZOOM:
		HandleZoom(deltaX, deltaY);
		return true;
	case ORBIT:
		HandleOrbit(deltaX, deltaY);
		return true;
	default:
		return false;
	}
}

bool SceneCamera::Update(double speed, double timeDelta)
{
	if (m_currentMode != FPS)
		return false;

	float currentSpeed = camSpeed_;
	if (camSpedUp) currentSpeed *= 2.0f;
	if (camSlowed) currentSpeed *= 0.25f;

	float moveAmount = currentSpeed * static_cast<float>(timeDelta);

	if (cameraMovingForward_)
		MoveForward(moveAmount);
	if (cameraMovingBackward_)
		MoveForward(-moveAmount);
	if (cameraMovingRight_)
		MoveRight(moveAmount);
	if (cameraMovingLeft_)
		MoveRight(-moveAmount);
	if (cameraMovingUp_)
		MoveUp(moveAmount);
	if (cameraMovingDown_)
		MoveUp(-moveAmount);

	return cameraMovingForward_ || cameraMovingBackward_ ||
		cameraMovingRight_ || cameraMovingLeft_ ||
		cameraMovingUp_ || cameraMovingDown_;
}

void SceneCamera::OnActionPressed(Hotkey::Action action)
{
    if (action == Hotkey::Action::Camera_FPSMode)
    {
        m_currentMode = FPS;
        return;
    }
    if (action == Hotkey::Action::Camera_NormalPanMode)
    {
        m_currentMode = PAN;
        camSlowed = false;
        camSpedUp = false;
        return;
    }
    if (action == Hotkey::Action::Camera_SlowPanMode)
    {
        m_currentMode = PAN;
        camSlowed = true;
        camSpedUp = false;
        return;
    }
    if (action == Hotkey::Action::Camera_FastPanMode)
    {
        m_currentMode = PAN;
        camSpedUp = true;
        camSlowed = false;
        return;
    }
    if (action == Hotkey::Action::Camera_ZoomMode)
    {
        m_currentMode = ZOOM;
        return;
    }
    if (action == Hotkey::Action::Camera_OrbitMode)
    {
        m_currentMode = ORBIT;
        return;
    }

    // FPS movement
    if (m_currentMode == FPS)
    {
        if (action == Hotkey::Action::Camera_Forward)
            cameraMovingForward_ = true;
        if (action == Hotkey::Action::Camera_Backward)
            cameraMovingBackward_ = true;
        if (action == Hotkey::Action::Camera_Down)
            cameraMovingDown_ = true;
        if (action == Hotkey::Action::Camera_Up)
            cameraMovingUp_ = true;
        if (action == Hotkey::Action::Camera_StrafeLeft)
            cameraMovingLeft_ = true;
        if (action == Hotkey::Action::Camera_StrafeRight)
            cameraMovingRight_ = true;
        if (action == Hotkey::Action::Camera_SpeedUp)
            camSpedUp = true;
        if (action == Hotkey::Action::Camera_SlowDown)
            camSlowed = true;
    }

    // Special actions
    if (action == Hotkey::Action::Camera_MoveObjectToView)
    {
        auto currentObj = GameObjectManager::getInstance()->GetSelectedObject();
        if (!currentObj) return;

        currentObj->SetLocalPosition(
            this->GetLocalPosition() +
            glm::normalize(glm::vec3(GetForward())) * m_defaultPivotDistance
        );
    }

    if (action == Hotkey::Action::Camera_Reset)
    {
        auto selected = GameObjectManager::getInstance()->GetSelectedObject();
        if (selected)
            this->Reset();
    }
}

void SceneCamera::OnActionReleased(Hotkey::Action action)
{
    if (action == Hotkey::Action::Camera_Forward)
        cameraMovingForward_ = false;
    if (action == Hotkey::Action::Camera_Backward)
        cameraMovingBackward_ = false;
    if (action == Hotkey::Action::Camera_Down)
        cameraMovingDown_ = false;
    if (action == Hotkey::Action::Camera_Up)
        cameraMovingUp_ = false;
    if (action == Hotkey::Action::Camera_StrafeLeft)
        cameraMovingLeft_ = false;
    if (action == Hotkey::Action::Camera_StrafeRight)
        cameraMovingRight_ = false;
    if (action == Hotkey::Action::Camera_SpeedUp)
        camSpedUp = false;
    if (action == Hotkey::Action::Camera_SlowDown)
        camSlowed = false;
}

void SceneCamera::HandleFPSRotation(float deltaX, float deltaY)
{
    constexpr float limit = 360.0f * 2.0f;
    constexpr float sensitivity = 0.1f;
    
    auto localRotation = GetLocalRotationEuler();
    localRotation.x -= deltaX * sensitivity;
    localRotation.y += deltaY * sensitivity;

    if (localRotation.y > limit)  localRotation.y = limit;
    if (localRotation.y < -limit)  localRotation.y = -limit;

    SetLocalRotationEuler(localRotation);
}

void SceneCamera::HandlePan(float dx, float dy)
{
    constexpr float panSensitivity = 0.01f;
    MoveRight(dx * panSensitivity);
    MoveUp(-dy * panSensitivity);
}

void SceneCamera::HandleZoom(float dx, float dy)
{
    constexpr float zoomSensitivity = 0.1f;
    float zoomAmount = (dx + dy) * zoomSensitivity;
    MoveForward(zoomAmount);
}

void SceneCamera::HandleOrbit(float dx, float dy)
{
    constexpr float orbitSensitivity = 0.5f;

    orbitYaw_ -= dx * orbitSensitivity;
    orbitPitch_ += dy * orbitSensitivity;

    // Clamp pitch to avoid gimbal lock
    orbitPitch_ = glm::clamp(orbitPitch_, -89.0f, 89.0f);

    // Calculate offset from pivot
    glm::vec3 offset;
    offset.x = cos(glm::radians(orbitPitch_)) * sin(glm::radians(orbitYaw_));
    offset.y = sin(glm::radians(orbitPitch_));
    offset.z = cos(glm::radians(orbitPitch_)) * cos(glm::radians(orbitYaw_));

    SetLocalPosition(orbitPivot_ - offset * m_defaultPivotDistance);
    lookAt(orbitPivot_);
}
