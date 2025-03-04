#include "GameObject.h"

#include <iostream>
#include <glm/gtx/euler_angles.hpp>

#include "EventBroadcaster.h"
#include "ModelManager.h"

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
		this->localScale = glm::inverse(glm::scale(glm::mat4(1.0f), newParent->worldScale)) * glm::vec4(this->worldScale, 1.0f);

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

void GameObject::updateWorldTransform()
{

	if (this->parent)
	{
		this->worldPosition = this->parent->worldPosition + this->localPosition;
		this->worldRotation = this->parent->worldRotation + this->localRotation;
		this->worldScale = this->parent->worldScale * this->localScale;
	}
	else
	{
		this->worldPosition = this->localPosition;
		this->worldRotation = this->localRotation;
		this->worldScale = this->localScale;
	}

	// Update children recursively
	for (GameObject* child : this->children)
	{
		if (child)
		{
			child->updateWorldTransform();
		}
	}


	if (type != CAMERA)
	{
		
		this->performModelTransform();
		this->performModelRotate();
		this->performModelScale();

		if (this->modelRef && !this->modelRef->Vertices().empty())
		{
			glm::mat4 worldTransform = glm::translate(glm::mat4(1.0f), this->worldPosition);

			worldTransform *= glm::eulerAngleYXZ(glm::radians(this->worldRotation.y),
				glm::radians(this->worldRotation.x),
				glm::radians(this->worldRotation.z));

			worldTransform = glm::scale(worldTransform, this->worldScale);

			std::vector<glm::vec3> worldPositions;

			worldPositions.reserve(this->modelRef->Vertices().size());

			for (const auto& vertex : this->modelRef->Vertices())
			{
				glm::vec3 posWorld = glm::vec3(worldTransform * glm::vec4(vertex.Position, 1.0f));
				worldPositions.push_back(posWorld);
			}

			glm::mat4 rotMat = glm::eulerAngleYXZ(glm::radians(this->worldRotation.y),
				glm::radians(this->worldRotation.x),
				glm::radians(this->worldRotation.z));

			glm::vec3 axisX = glm::normalize(glm::vec3(rotMat[0]));
			glm::vec3 axisY = glm::normalize(glm::vec3(rotMat[1]));
			glm::vec3 axisZ = glm::normalize(glm::vec3(rotMat[2]));

			std::array<glm::vec3, 3> axes = { axisX, axisY, axisZ };

			glm::vec3 computedCenter(0.0f);
			for (const auto& pos : worldPositions)
			{
				computedCenter += pos;
			}
			computedCenter /= static_cast<float>(worldPositions.size());

			BoundingBox newOBB(this->worldPosition, worldPositions, axes);

			setOBB(newOBB);
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

	mat4 scaleOp = glm::scale(mat4(1), scaleOffset);
	if (modelRef)
		this->modelRef->Transform(scaleOp);

	this->originScale = this->worldScale;

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}
