#include "GameObject.h"

#include <iostream>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "EventBroadcaster.h"
#include "ModelManager.h"
#include "RayTracer.hpp"

GameObject::GameObject()
{
	this->name = "No-name";
	this->type = NONE;
	this->modelRef = nullptr;
}

GameObject::GameObject(String name, PrimitiveType type)
{
	this->name = name;
	this->type = type;
	this->modelRef = nullptr;
}

GameObject::GameObject(String name, PrimitiveType type, std::shared_ptr<Assets::Model> modelRef)
{
	this->name = name;
	this->type = type;
	this->modelRef = modelRef;

	this->updateWorldTransform();
}

GameObject::String GameObject::getName() const
{
	return this->name;
}

GameObject::PrimitiveType GameObject::getType() const
{
	return this->type;
}

bool GameObject::isEnabled()
{
	return this->enabled;
}

void GameObject::setEnabled(bool flag)
{
	this->enabled = flag;
}

GameObject::vec3 GameObject::getLocalPosition() const
{
	return this->localPosition;
}

GameObject::vec3 GameObject::getWorldPosition() const
{
	return this->worldPosition;
}

GameObject::vec3 GameObject::getLocalRotation() const
{
	return this->localRotation;
}

GameObject::vec3 GameObject::getWorldRotation() const
{
	return this->worldRotation;
}

GameObject::vec3 GameObject::getLocalScale() const
{
	return this->localScale;
}

GameObject::vec3 GameObject::getWorldScale() const
{
	return this->worldScale;
}

void GameObject::setName(std::string name)
{
	this->name = name;
}

void GameObject::setLocalPosition(vec3 newPos)
{
	this->localPosition = newPos;
	this->updateWorldTransform();
}

void GameObject::setLocalPosition(float x, float y, float z)
{
	this->localPosition = vec3(x, y, z);
	this->updateWorldTransform();
}

void GameObject::setLocalRotation(vec3 newRot)
{
	newRot.x = fmod(newRot.x + 180.0f, 360.0f);

	if (newRot.x < 0)
		newRot.x += 360.0f;
	newRot.x -= 180.0f;

	newRot.y = fmod(newRot.y + 180.0f, 360.0f);

	if (newRot.y < 0)
		newRot.y += 360.0f;
	newRot.y -= 180.0f;

	newRot.z = fmod(newRot.z + 180.0f, 360.0f);

	if (newRot.z < 0)
		newRot.z += 360.0f;
	newRot.z -= 180.0f;

	this->localRotation = newRot;
	this->updateWorldTransform();
}

void GameObject::setLocalRotation(float x, float y, float z)
{
	vec3 newRot(x, y, z);

	newRot.x = fmod(newRot.x + 180.0f, 360.0f);

	if (newRot.x < 0)
		newRot.x += 360.0f;
	newRot.x -= 180.0f;

	newRot.y = fmod(newRot.y + 180.0f, 360.0f);

	if (newRot.y < 0)
		newRot.y += 360.0f;
	newRot.y -= 180.0f;

	newRot.z = fmod(newRot.z + 180.0f, 360.0f);

	if (newRot.z < 0)
		newRot.z += 360.0f;
	newRot.z -= 180.0f;

	this->localRotation = newRot;
	this->updateWorldTransform();
}

void GameObject::setLocalScale(vec3 newScale)
{
	this->localScale = newScale;
	this->updateWorldTransform();
}

void GameObject::setLocalScale(float x, float y, float z)
{
	this->localScale = vec3(x, y, z);
	this->updateWorldTransform();
}

glm::mat4& GameObject::getObjectMatrix()
{
	return this->mat_;
}

std::shared_ptr<Assets::Model> GameObject::getModel()
{
	return this->modelRef;
}

void GameObject::addChild(GameObject* child)
{
	if (!child || child == this || child->parent == this)
		return;

	if (child->parent)
		child->parent->removeChild(child);

	child->parent = this;
	children.push_back(child);

	child->localPosition = glm::inverse(glm::translate(glm::mat4(1.0f), this->worldPosition)) * glm::vec4(child->worldPosition, 1.0f);
	child->localRotation = child->worldRotation - this->worldRotation;
	child->localScale = glm::inverse(glm::scale(glm::mat4(1.0f), this->worldScale)) * glm::vec4(child->worldScale, 1.0f);

	child->updateWorldTransform();
}


void GameObject::removeChild(GameObject* child)
{
	if (!child) return;

	auto it = std::find(children.begin(), children.end(), child);
	if (it != children.end())
	{
		children.erase(it);
		child->parent = nullptr;
	}
}

std::vector<GameObject*> GameObject::getChildren() const
{
	return this->children;
}

GameObject* GameObject::getParent() const
{
	return this->parent;
}

void GameObject::setParent(GameObject* newParent)
{
	if (newParent == this || (newParent && isDescendantOf(newParent)))
		return;

	if (parent)
		parent->removeChild(this);

	if (newParent)
	{
		this->localPosition = this->worldPosition - newParent->worldPosition;
		this->localRotation = this->worldRotation - newParent->worldRotation;
		this->localScale = this->worldScale / this->localScale;

	}
	else
	{
		this->localPosition = this->worldPosition;
		this->localRotation = this->worldRotation;
		this->localScale = this->worldScale;
	}

	parent = newParent;

	if (parent)
		parent->addChild(this);

	updateWorldTransform();
}

bool GameObject::isDescendantOf(const GameObject* potentialParent) const
{
	const GameObject* current = this;
	while (current)
	{
		if (current == potentialParent)
		{
			return true;
		}
		current = current->parent;
	}
	return false;
}

uint32_t GameObject::getID() const
{
	return this->id;
}

void GameObject::setID(uint32_t newID)
{
	this->id = newID;
}

void GameObject::setOBB(const BoundingBox& obb)
{
	this->obb = std::make_shared<BoundingBox>(obb);
}

std::shared_ptr<BoundingBox> GameObject::getOBB() const
{
	return this->obb;
}

void GameObject::updateObjectMatrix()
{
	glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), this->worldScale);

	glm::mat4 rotateZ = glm::rotate(glm::mat4(1.0f), glm::radians(this->worldRotation.z), glm::vec3(0, 0, 1));
	glm::mat4 rotateY = glm::rotate(glm::mat4(1.0f), glm::radians(this->worldRotation.y), glm::vec3(0, 1, 0));
	glm::mat4 rotateX = glm::rotate(glm::mat4(1.0f), glm::radians(this->worldRotation.x), glm::vec3(1, 0, 0));

	glm::mat4 rotationMat = rotateZ * rotateY * rotateX;
	glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), this->worldPosition);

	this->mat_ = translationMat * rotationMat * scaleMat;

}


void GameObject::updateWorldTransform()
{
	// Construct local transformation matrix: T * R * S
	glm::mat4 localMat = glm::translate(glm::mat4(1.0f), localPosition) *
		glm::yawPitchRoll(glm::radians(localRotation.y), glm::radians(localRotation.x), glm::radians(localRotation.z)) *
		glm::scale(glm::mat4(1.0f), localScale);

	// Combine with parent transform if exists
	if (parent) {
		mat_ = parent->mat_ * localMat;
	}
	else {
		mat_ = localMat;
	}

	// Decompose mat_ to get world position, rotation, scale
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::quat rotationQuat;
	glm::decompose(mat_, worldScale, rotationQuat, worldPosition, skew, perspective);
	worldRotation = glm::degrees(glm::eulerAngles(rotationQuat)); // Convert quat to Euler in degrees

	// Update children recursively
	for (GameObject* child : children) {
		if (child) {
			child->updateWorldTransform();
		}
	}

	// Update bounding box and model matrix
	if (modelRef && !modelRef->Vertices().empty()) {
		std::vector<glm::vec3> worldPositions;
		worldPositions.reserve(modelRef->Vertices().size());

		for (const auto& vertex : modelRef->Vertices()) {
			glm::vec3 posWorld = glm::vec3(mat_ * glm::vec4(vertex.Position, 1.0f));
			worldPositions.push_back(posWorld);
		}

		// Calculate OBB axes from rotation matrix
		glm::mat3 rotMat = glm::mat3_cast(rotationQuat);
		glm::vec3 axisX = glm::normalize(rotMat[0]);
		glm::vec3 axisY = glm::normalize(rotMat[1]);
		glm::vec3 axisZ = glm::normalize(rotMat[2]);

		std::array<glm::vec3, 3> axes = { axisX, axisY, axisZ };

		// Calculate OBB center
		glm::vec3 computedCenter(0.0f);
		for (const auto& pos : worldPositions) {
			computedCenter += pos;
		}
		computedCenter /= static_cast<float>(worldPositions.size());

		// Set the new OBB
		BoundingBox newOBB(worldPosition, worldPositions, axes);
		setOBB(newOBB);
	}

	// Apply transformation to model
	if (modelRef) {
		modelRef->Transform(mat_);

		if (RayTracer::getInstance()->getUserSettings().IsRayTraced) {
			EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
		}
	}
}


/**
 * \brief Performs the model transform via model-view-projection matrix form
 */
void GameObject::performModelTransform()
{
	mat4 translateOp = glm::translate(mat4(1), this->worldPosition - this->origin);
	this->origin = this->worldPosition;
	if (modelRef)
		this->modelRef->Transform(translateOp);

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void GameObject::performModelRotate()
{
	vec3 rotOffset = this->worldRotation - this->originRot;

	mat4 translateToOrigin = glm::translate(mat4(1.0f), -this->worldPosition);

	mat4 rotateXOp = glm::rotate(mat4(1), glm::radians(rotOffset.x), vec3(1, 0, 0));
	mat4 rotateYOp = glm::rotate(mat4(1), glm::radians(rotOffset.y), vec3(0, 1, 0));
	mat4 rotateZOp = glm::rotate(mat4(1), glm::radians(rotOffset.z), vec3(0, 0, 1));

	mat4 translateBack = glm::translate(mat4(1.0f), this->worldPosition);

	mat4 finalRotation = translateBack * rotateZOp * rotateYOp * rotateXOp * translateToOrigin;

	if (modelRef)
		this->modelRef->Transform(finalRotation);

	this->originRot = this->worldRotation;
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void GameObject::performModelScale()
{
	vec3 scaleOffset = this->worldScale / this->originScale;

	mat4 translateToOrigin = glm::translate(mat4(1.0f), -this->worldPosition);
	mat4 scaleOp = glm::scale(mat4(1.0f), scaleOffset);
	mat4 translateBack = glm::translate(mat4(1.0f), this->worldPosition);

	mat4 finalScale = translateBack * scaleOp * translateToOrigin;

	if (modelRef)
		this->modelRef->Transform(finalScale);

	this->originScale = this->worldScale;

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}
