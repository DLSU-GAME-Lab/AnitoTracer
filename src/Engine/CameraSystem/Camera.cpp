#include "Camera.h"

#include <iostream>
#include <glm/fwd.hpp>
#include <glm/gtx/string_cast.hpp>

#include "From-GDGRAP2/Debug.h"
#include "From-GDGRAP2/ModelManager.h"
#include "OBB/Ray.hpp"
#include "Vulkan/Vulkan.hpp"

Camera::Camera(std::string name, ProjectionMode proj) : GameObject(name, PrimitiveType::CAMERA)
{
	this->name = name;
	this->projMode = proj;
}

Camera::~Camera() {}

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
	switch (key)
	{
	case GLFW_KEY_S: cameraMovingBackward_ = action != GLFW_RELEASE; return true;
	case GLFW_KEY_W: cameraMovingForward_ = action != GLFW_RELEASE; return true;
	case GLFW_KEY_A: cameraMovingLeft_ = action != GLFW_RELEASE; return true;
	case GLFW_KEY_D: cameraMovingRight_ = action != GLFW_RELEASE; return true;
	case GLFW_KEY_LEFT_CONTROL: cameraMovingDown_ = action != GLFW_RELEASE; return true;
	case GLFW_KEY_LEFT_SHIFT: cameraMovingUp_ = action != GLFW_RELEASE; return true;

	case GLFW_KEY_F: {

		auto selected = ModelManager::getInstance()->getSelectedObject();

		this->Reset(lookAt(selected->getWorldPosition(), selected->getWorldPosition() - vec3(0, 0, 100), vec3(0, 1, 0)));

		break;
	}

	default: return false;
	}
}

bool Camera::OnCursorPosition(const double xpos, const double ypos)
{
	const auto deltaX = static_cast<float>(xpos - mousePosX_);
	const auto deltaY = static_cast<float>(ypos - mousePosY_);

	const auto limit = 360 * 2;
	if (mouseRightPressed_)
	{
		cameraRotX_ += deltaX;
		this->localRotation.x -= deltaX;

		cameraRotY_ += deltaY;
		this->localRotation.y += deltaY;
		if (localRotation.y > limit) { cameraRotY_ = 0; this->localRotation.y -= deltaY; }
		if (localRotation.y < -limit) { cameraRotY_ = 0; this->localRotation.y -= deltaY; }

		this->setLocalRotation(glm::vec3(localRotation));
	}
	//if (mouseRightPressed_)
	//{
	//	modelRotX_ += deltaX;
	//	modelRotY_ += deltaY;
	//}

	mousePosX_ = xpos;
	mousePosY_ = ypos;

	return mouseLeftPressed_ || mouseRightPressed_;
}

bool Camera::OnMouseButton(const int button, const int action, const int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		float ndcX = (2.0f * static_cast<float>(mousePosX_)) / windowWidth_ - 1.0f;
		float ndcY = 1.0f - (2.0f * static_cast<float>(mousePosY_)) / windowHeight_;
		glm::vec2 mouseNDC(ndcX, ndcY);

		glm::mat4 view = orientation_ * glm::translate(glm::mat4(1), -glm::vec3(position_));
		;
		glm::mat4 proj = projection_;
		proj[1][1] *= -1;

		glm::mat4 invVP = glm::inverse(proj * view);

		glm::vec4 rayStartNDC(mouseNDC, 0.0f, 1.0f);
		glm::vec4 rayEndNDC(mouseNDC, 1.0f, 1.0f);

		glm::vec4 rayStartWorld = invVP * rayStartNDC;
		glm::vec4 rayEndWorld = invVP * rayEndNDC;
		rayStartWorld /= rayStartWorld.w;
		rayEndWorld /= rayEndWorld.w;

		glm::vec3 rayOrigin = glm::vec3(rayStartWorld);
		glm::vec rayEnd = glm::vec3(rayEndWorld);
		glm::vec3 rayDirection = glm::normalize(glm::vec3(rayEndWorld - rayStartWorld));

		Ray pickingRay(rayOrigin, rayDirection);

		//std::cout << "Ray Origin: " << glm::to_string(rayOrigin) << std::endl;
		//std::cout << "Ray Direction: " << glm::to_string(rayDirection) << std::endl;

		auto objects = ModelManager::getInstance()->getAllObjects();
		float closestT = std::numeric_limits<float>::max();
		std::shared_ptr<GameObject> selectedObject = nullptr;

		for (auto& obj : objects)
		{
			if (!obj->isEnabled())
				continue;

			auto obb = obj->getOBB();
			if (obb)
			{

				glm::vec3 minCorner = obb->center - obb->halfExtents;
				glm::vec3 maxCorner = obb->center + obb->halfExtents;

				//if (obj->getName().find("Light") == std::string::npos)
				//{
				//	std::cout << obj->getName() << " center: " << glm::to_string(obb->center) << std::endl;
				//	std::cout << "Min Corner: " << glm::to_string(minCorner) << std::endl;
				//	std::cout << "Max Corner: " << glm::to_string(maxCorner) << std::endl;

				//	std::cout << "----------------\n\n" << std::endl;
				//}

				float tHit = 0.0f;
				if (pickingRay.intersects(*obb, tHit))
				{
					if (tHit < closestT)
					{
						closestT = tHit;
						selectedObject = obj;
					}
				}
			}
		}

		if (selectedObject)
		{
			glm::vec3 hitPoint = rayOrigin + rayDirection * closestT;
			//std::cout << "Intersection at (" << hitPoint.x << ", "
			//	<< hitPoint.y << ", " << hitPoint.z << ")\n";
			ModelManager::getInstance()->setSelectedObject(selectedObject);
		}
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		mouseLeftPressed_ = action == GLFW_PRESS;
	}


	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		mouseRightPressed_ = action == GLFW_PRESS;
	}

	return true;
}

bool Camera::UpdateCamera(const double speed, const double timeDelta)
{
	const auto d = static_cast<float>(speed * timeDelta);

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

	return updated;
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

void Camera::MoveForward(const float d)
{
	position_ += d * forward_;
	GameObject::setLocalPosition(position_.x, position_.y, position_.z);
}

void Camera::MoveRight(const float d)
{
	position_ += d * right_;
	GameObject::setLocalPosition(position_.x, position_.y, position_.z);
}

void Camera::MoveUp(const float d)
{
	position_ += d * up_;
	GameObject::setLocalPosition(position_.x, position_.y, position_.z);
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
