#include "Camera.h"

#include <glm/fwd.hpp>
#include <glm/gtx/orthonormalize.hpp>

#include "From-GDGRAP2/ModelManager.h"
#include "Vulkan/Vulkan.hpp"

#include "HotkeySystem\HotkeySystem.hpp"
#include <From-GDGRAP2/EventBroadcaster.h>

Camera::Camera(std::string name, ProjectionMode proj) : GameObject(name, PrimitiveType::CAMERA)
{
	this->name = name;
	this->projMode = proj;

	HotkeySystem::getInstance()->addListener(this);
}

Camera::~Camera() 
{
	HotkeySystem::getInstance()->removeListener(this);
}

GameObject::GameObjectPtr Camera::Clone() const
{
	return std::make_unique<Camera>(*this);
}

void Camera::Reset()
{
	const glm::vec3 defaultPosition = glm::vec3(0.0f, 0.0f, 500.0f);
	const glm::quat defaultRotation = glm::quat(glm::vec3(0.0f));

	Reset(defaultPosition, defaultRotation);
}

void Camera::Reset(const glm::vec3& position, const glm::quat& orientation)
{
	this->setLocalPosition(position);
	this->setLocalRotationQuat(orientation);

	cameraRotX_ = 0.0f;
	cameraRotY_ = 0.0f;
	modelRotX_ = 0.0f;
	modelRotY_ = 0.0f;

	mouseLeftPressed_ = false;
	mouseRightPressed_ = false;
}

glm::mat4 Camera::getViewMatrix()
{
	const glm::quat q = glm::normalize(getLocalRotationQuat());

	const glm::mat4 Rinv = glm::toMat4(glm::conjugate(q));
	const glm::mat4 Tinv = glm::translate(glm::mat4(1.0f), -getLocalPosition());

	view_ = Rinv * Tinv;
	return view_;
}

bool Camera::OnKey(const int key, const int scancode, const int action, const int mods)
{
	return false;
}

bool Camera::OnCursorPosition(const double xpos, const double ypos)
{
	const float dx = static_cast<float>(xpos - mousePosX_);
	const float dy = static_cast<float>(ypos - mousePosY_);

	mousePosX_ = xpos;
	mousePosY_ = ypos;

	if (m_currentMode == FPS)
	{
		const float sensitivity = 0.0025f;
		cameraRotX_ += -dx * sensitivity;
		cameraRotY_ += -dy * sensitivity;
		return true;
	}

	if (m_currentMode == PAN)
	{

		return true;
	}

	if (m_currentMode == ZOOM)
	{

		return true;
	}

	return false;
}

bool Camera::OnMouseButton(const int button, const int action, const int mods)
{
	return false;
}

bool Camera::UpdateCamera(const double speed, const double timeDelta)
{
	bool moved = false;
	bool rotated = false;

	if (camSlowed) camSpeed_ = camSlowSpeed;
	else if (camSpedUp) camSpeed_ = camFastSpeed;
	else camSpeed_ = camNormalSpeed;

	this->updateWorldMatrix();

	glm::vec3 moveDirection = glm::vec3(0.0f, 0.0f, 0.0f);

	if (cameraMovingForward_)	moveDirection -= this->getForward();
	if (cameraMovingBackward_)	moveDirection += this->getForward();
	if (cameraMovingRight_)		moveDirection += this->getRight();
	if (cameraMovingLeft_)		moveDirection -= this->getRight();
	if (cameraMovingUp_)		moveDirection += this->getUp();
	if (cameraMovingDown_)		moveDirection -= this->getUp();

	if (glm::length2(moveDirection) > 1e-8f)
	{
		moved = true;
		moveDirection = glm::normalize(moveDirection);
		auto d = moveDirection * this->camSpeed_ * (float)timeDelta * 500.0f;
		this->setLocalPosition(this->getLocalPosition() + d);
	}

	if (m_currentMode == FPS && (cameraRotX_ != 0.0f || cameraRotY_ != 0.0f))
	{
		yaw_ += cameraRotX_;
		pitch_ += cameraRotY_;

		constexpr float maxPitch = glm::radians(89.0f);
		pitch_ = glm::clamp(pitch_, -maxPitch, maxPitch);

		const glm::quat qYaw = glm::angleAxis(yaw_, glm::vec3(0, 1, 0));
		const glm::vec3 right = glm::normalize(qYaw * glm::vec3(1, 0, 0));
		const glm::quat qPitch = glm::angleAxis(pitch_, right);

		const glm::quat orientation = glm::normalize(qPitch * qYaw);

		setLocalRotationQuat(orientation);
		rotated = true;
	}

	const bool updated = moved || rotated;

	// consume deltas
	cameraRotX_ = 0.0f;
	cameraRotY_ = 0.0f;

	if (updated)
	{
		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_RESET_ACCUMULATOR);
	}

	return updated;
}


void Camera::OnActionPressed(Hotkey::Action action)
{
	if (action == Hotkey::Action::Camera_FPSMode)
	{
		m_currentMode = FPS;
	}

	if (action == Hotkey::Action::Camera_NormalPanMode)
	{
		m_currentMode = PAN;
	}

	if (action == Hotkey::Action::Camera_SlowPanMode)
	{
		m_currentMode = PAN;
		camSlowed = true;
	}

	if (action == Hotkey::Action::Camera_FastPanMode)
	{
		m_currentMode = PAN;
		camSpedUp = true;
	}

	if (action == Hotkey::Action::Camera_ZoomMode)
	{
		m_currentMode = ZOOM;
	}

	if (action == Hotkey::Action::Camera_OrbitMode)
	{
		m_currentMode = ORBIT;
	}

	if (m_currentMode == FPS)
	{
		if (action == Hotkey::Action::Camera_Forward)
		{
			cameraMovingForward_ = true;
		}

		if (action == Hotkey::Action::Camera_Backward)
		{
			cameraMovingBackward_ = true;
		}

		if (action == Hotkey::Action::Camera_Down)
		{
			cameraMovingDown_ = true;
		}

		if (action == Hotkey::Action::Camera_Up)
		{
			cameraMovingUp_ = true;
		}

		if (action == Hotkey::Action::Camera_StrafeLeft)
		{
			cameraMovingLeft_ = true;
		}

		if (action == Hotkey::Action::Camera_StrafeRight)
		{
			cameraMovingRight_ = true;
		}

		if (action == Hotkey::Action::Camera_SpeedUp)
		{
			camSpedUp = true;
		}

		if (action == Hotkey::Action::Camera_SlowDown)
		{
			camSlowed = true;
		}
	}

	if (action == Hotkey::Action::Camera_MoveObjectToView)
	{
		auto currentObj = ModelManager::getInstance()->getSelectedObject();
		if (!currentObj) return;

		currentObj->setLocalPosition(this->getLocalPosition() + glm::normalize(this->getForward() * m_defaultPivotDistance));

		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	}

	if (action == Hotkey::Action::Camera_Reset)
	{
		auto selected = ModelManager::getInstance()->getSelectedObject();

		if (selected) {
			this->Reset();
		}

		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	}
}

void Camera::OnActionReleased(Hotkey::Action action)
{
	if (action == Hotkey::Action::Camera_FPSMode)
	{
		m_currentMode = NONE;
	}

	if (action == Hotkey::Action::Camera_NormalPanMode)
	{
		m_currentMode = NONE;
		cameraMovingLeft_ = false;
		cameraMovingRight_ = false;
		cameraMovingUp_ = false;
		cameraMovingDown_ = false;
	}

	if (action == Hotkey::Action::Camera_SlowPanMode)
	{
		m_currentMode = NONE;
		camSlowed = false;
		cameraMovingLeft_ = false;
		cameraMovingRight_ = false;
		cameraMovingUp_ = false;
		cameraMovingDown_ = false;
	}

	if (action == Hotkey::Action::Camera_FastPanMode)
	{
		m_currentMode = NONE;
		camSpedUp = false;
		cameraMovingLeft_ = false;
		cameraMovingRight_ = false;
		cameraMovingUp_ = false;
		cameraMovingDown_ = false;
	}

	if (action == Hotkey::Action::Camera_ZoomMode)
	{
		m_currentMode = NONE;
	}

	if (action == Hotkey::Action::Camera_OrbitMode)
	{
		m_currentMode = NONE;
	}

	if (action == Hotkey::Action::Camera_Forward)
	{
		cameraMovingForward_ = false;
	}

	if (action == Hotkey::Action::Camera_Backward)
	{
		cameraMovingBackward_ = false;
	}

	if (action == Hotkey::Action::Camera_Down)
	{
		cameraMovingDown_ = false;
	}

	if (action == Hotkey::Action::Camera_Up)
	{
		cameraMovingUp_ = false;
	}

	if (action == Hotkey::Action::Camera_StrafeLeft)
	{
		cameraMovingLeft_ = false;
	}

	if (action == Hotkey::Action::Camera_StrafeRight)
	{
		cameraMovingRight_ = false;
	}

	if (action == Hotkey::Action::Camera_SpeedUp)
	{
		camSpedUp = false;
	}

	if (action == Hotkey::Action::Camera_SlowDown)
	{
		camSlowed = false;
	}
}

glm::mat4 Camera::GetProjection(UserSettings settings, const VkExtent2D extent)
{
	switch (projMode)
	{
	case ProjectionMode::orthographic:
		projection_ = glm::ortho(-1000.0f, 1000.0f, 1000.0f, -1000.0f, 0.1f, 1000.0f);
		break;

	case ProjectionMode::perspective:
		projection_ = glm::perspective(glm::radians(settings.FieldOfView), extent.width / static_cast<float>(extent.height), 0.1f, 10000.0f);
		break;
	}

	windowWidth_ = extent.width;
	windowHeight_ = extent.height;

	return projection_;
}

glm::mat4 Camera::GetProjection()
{
	return this->projection_;
}

void Camera::SetProjectionType(ProjectionMode type)
{
	this->projMode = type;
}

glm::mat4 Camera::GetView()
{
	return this->view_;
}

Camera::CameraMoveMode Camera::getCurrentMoveMode() const
{
	return this->m_currentMode;
}

void Camera::lookAt(const glm::vec3& target)
{
	const glm::vec3 position = this->getLocalPosition();
	const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::vec3 dir = target - position;
	const float len2 = glm::dot(dir, dir);
	if (len2 < 1e-12f) return;

	const glm::mat4 view = glm::lookAt(position, target, worldUp);
	const glm::mat4 inv = glm::inverse(view);

	glm::mat3 rotM = glm::mat3(inv);

	rotM = glm::orthonormalize(rotM);

	const glm::quat q = glm::quat_cast(rotM);
	this->setLocalRotationQuat(q);
}