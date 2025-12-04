#include "Camera.h"

#include <iostream>
#include <glm/fwd.hpp>
#include <glm/gtx/string_cast.hpp>

#include "From-GDGRAP2/Debug.h"
#include "From-GDGRAP2/ModelManager.h"
#include "OBB/Ray.hpp"
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

void Camera::Reset(const glm::mat4& modelView)
{
	const auto inverse = glm::inverse(modelView);

	position_ = inverse * glm::vec4(0, 0, 0, 1);
	orientation_ = glm::mat4(glm::mat3(modelView));

	cameraRotX_ = 0;
	cameraRotY_ = 0;
	modelRotX_ = 0;
	modelRotY_ = 0;

	mouseLeftPressed_ = false;
	mouseRightPressed_ = false;

	UpdateVectors();
}

glm::mat4 Camera::ModelView()
{
	auto cameraRotX = static_cast<float>(modelRotY_ / 300.0);
	auto cameraRotY = static_cast<float>(modelRotX_ / 300.0);

	auto model =
		glm::rotate(glm::mat4(1.0f), cameraRotY * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), cameraRotX * glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	auto view = orientation_ * glm::translate(glm::mat4(1), -glm::vec3(position_));

	view_ = view;

	return view * model;
}

bool Camera::OnKey(const int key, const int scancode, const int action, const int mods)
{
	if (key == GLFW_KEY_ESCAPE && action != GLFW_REPEAT) 
	{
		exit(0); // temp exit lol 
	}
}

bool Camera::OnCursorPosition(const double xpos, const double ypos)
{
	const auto deltaX = static_cast<float>(xpos - mousePosX_);
	const auto deltaY = static_cast<float>(ypos - mousePosY_);

	const auto limit = 360 * 2;

	if (m_currentMode == FPS)
	{
		cameraRotX_ += deltaX;
		this->localRotation.x -= deltaX;

		cameraRotY_ += deltaY;
		this->localRotation.y += deltaY;
		if (localRotation.y > limit) { cameraRotY_ = 0; this->localRotation.y -= deltaY; }
		if (localRotation.y < -limit) { cameraRotY_ = 0; this->localRotation.y -= deltaY; }

		this->SetLocalRotation(glm::vec3(localRotation));
		UpdateVectors();
	}

	if (m_currentMode == PAN)
	{
		// Needs Sensitivity Settings
		MoveRight(deltaX * camSpeed_);
		MoveUp(deltaY * camSpeed_);
		UpdateVectors();
	}

	if (m_currentMode == ZOOM)
	{
		MoveForward(deltaX);
		MoveForward(deltaY);
		UpdateVectors();
	}

	mousePosX_ = xpos;
	mousePosY_ = ypos;

	return m_currentMode != NONE;
}

bool Camera::OnMouseButton(const int button, const int action, const int mods)
{
	return false;
}

bool Camera::UpdateCamera(const double speed, const double timeDelta)
{
	if (camSlowed) camSpeed_ = camSlowSpeed;
	else if (camSpedUp) camSpeed_ = camFastSpeed;
	else camSpeed_ = camNormalSpeed;

	const auto d = static_cast<float>(speed * timeDelta) * this->camSpeed_;

	if (cameraMovingLeft_) MoveRight(-d);
	if (cameraMovingRight_) MoveRight(d);
	if (cameraMovingBackward_) MoveForward(-d);
	if (cameraMovingForward_) MoveForward(d);
	if (cameraMovingDown_) MoveUp(-d);
	if (cameraMovingUp_) MoveUp(d);

	const float rotationDiv = 300;
	Rotate(cameraRotX_ / rotationDiv, cameraRotY_ / rotationDiv);

	const bool updated =
		cameraMovingLeft_ ||
		cameraMovingRight_ ||
		cameraMovingBackward_ ||
		cameraMovingForward_ ||
		cameraMovingDown_ ||
		cameraMovingUp_ ||
		cameraRotY_ != 0 ||
		cameraRotX_ != 0;

	cameraRotY_ = 0;
	cameraRotX_ = 0;

	if (updated)
		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_RESET_ACCUMULATOR);

	return updated;
}

void Camera::OnActionPressed(Hotkey::Action action)
{
	UpdateVectors();

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
		auto currentObj = GameObjectManager::getInstance()->GetSelectedObject();
		if (!currentObj) return;

		currentObj->setLocalPosition(this->GetLocalPosition() + glm::normalize(glm::vec3(forward_)) * m_defaultPivotDistance );

		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	}

	if (action == Hotkey::Action::Camera_Reset)
	{
		auto selected = GameObjectManager::getInstance()->GetSelectedObject();

		if (selected) {
			this->Reset(glm::lookAt(
				selected->GetWorldPosition() - glm::vec3(0, 0, 1000),
				selected->GetWorldPosition(),
				glm::vec3(0, 1, 0)
			));
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

void Camera::setLocalPosition(float x, float y, float z)
{
	this->position_ = glm::vec4(x, y, z, 1.0);
	GameObject::setLocalPosition(x, y, z);
}

void Camera::setLocalPosition(glm::vec3 pos)
{
	this->position_ = glm::vec4(pos, 1.0);
	GameObject::setLocalPosition(pos);
}

Camera::CameraMoveMode Camera::getCurrentMoveMode() const
{
	return this->m_currentMode;
}

void Camera::MoveForward(const float d)
{
	position_ += d * forward_;
	GameObject::setLocalPosition(position_.x, position_.y, position_.z);
	UpdateVectors();
}

void Camera::MoveRight(const float d)
{
	position_ += d * right_;
	GameObject::setLocalPosition(position_.x, position_.y, position_.z);
	UpdateVectors();
}

void Camera::MoveUp(const float d)
{
	position_ += d * up_;
	GameObject::setLocalPosition(position_.x, position_.y, position_.z);
	UpdateVectors();
}

void Camera::Rotate(const float y, const float x)
{
	orientation_ =
		glm::rotate(glm::mat4(1), x, glm::vec3(1, 0, 0)) *
		orientation_ *
		glm::rotate(glm::mat4(1), y, glm::vec3(0, 1, 0));

	UpdateVectors();
}

void Camera::UpdateVectors()
{
	// Given the ortientation matrix, find out the x,y,z vector orientation.
	const auto inverse = glm::inverse(orientation_);

	right_ = inverse * glm::vec4(1, 0, 0, 0);
	up_ = inverse * glm::vec4(0, 1, 0, 0);
	forward_ = inverse * glm::vec4(0, 0, -1, 0);
}
