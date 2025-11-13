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
	this->name = "no-name";
	this->type = NONE;
	this->modelRef = nullptr;

	this->setLocalDirty(true);
	this->getWorldMatrix();
}

GameObject::GameObject(String name, PrimitiveType type)
{
	this->name = name;
	this->type = type;
	this->modelRef = nullptr;

	this->setLocalDirty(true);
	this->getWorldMatrix();
}

GameObject::GameObject(String name, PrimitiveType type, std::shared_ptr<Assets::Model> modelRef)
{
	this->name = name;
	this->type = type;
	this->modelRef = modelRef;

	this->setLocalDirty(true);
	this->getWorldMatrix();
}

void GameObject::setName(std::string name)
{
	this->name = name;
}

GameObject::String GameObject::getName() const
{
	return this->name;
}

GameObject::PrimitiveType GameObject::getType() const
{
	return this->type;
}

bool GameObject::isActive()
{
	return this->active;
}

void GameObject::setActive(bool flag)
{
	this->active = flag;

	for(const auto& child : this->children)
	{
		if (child)
			child->setActive(flag);
	}
}

bool GameObject::isVisible()
{
	return this->visible;
}

void GameObject::setVisible(bool flag)
{
	this->visible = flag;

	for (const auto& child : this->children)
	{
		if (child)
			child->setVisible(flag);
	}
}

bool GameObject::isPickable()
{
	return this->pickable;
}

void GameObject::setPickable(bool flag)
{
	this->pickable = flag;

	for (const auto& child : this->children)
	{
		if (child)
			child->setPickable(flag);
	}
}

void GameObject::setLocalPosition(vec3 newPos)
{
	this->localPosition = newPos;
	this->setLocalDirty(true);
}

void GameObject::setLocalPosition(float x, float y, float z)
{
	this->localPosition = vec3(x, y, z);
	this->setLocalDirty(true);
}

GameObject::vec3 GameObject::getLocalPosition() const
{
	return this->localPosition;
}

GameObject::vec3 GameObject::getWorldPosition() const
{
	return this->worldPosition;
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
	this->setLocalDirty(true);
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
	this->setLocalDirty(true);
}

GameObject::vec3 GameObject::getLocalRotation() const
{
	return this->localRotation;
}

GameObject::vec3 GameObject::getWorldRotation() const
{
	return this->worldRotation;
}

void GameObject::setLocalScale(vec3 newScale)
{
	this->localScale = newScale;
	this->setLocalDirty(true);
}

void GameObject::setLocalScale(float x, float y, float z)
{
	this->localScale = vec3(x, y, z);
	this->setLocalDirty(true);
}

GameObject::vec3 GameObject::getLocalScale() const
{
	return this->localScale;
}

GameObject::vec3 GameObject::getWorldScale() const
{
	return this->worldScale;
}

glm::mat4& GameObject::getObjectMatrix()
{
	return this->localMatrix;
}

std::shared_ptr<Assets::Model> GameObject::getModel()
{
	this->getWorldMatrix(); //updates

	return this->modelRef;
}

void GameObject::addChild(GameObject::GameObjectPtr child)
{
	if (!child || child.get() == this || child->parent == this)
		return;

	if (child->parent)
		child->parent->removeChild(child.get());

	child->parent = this;
	child->setWorldDirty(true);
	children.push_back(std::move(child));
}

void GameObject::addChildAtIndex(GameObjectPtr child, int index)
{
	if (!child || child.get() == this || child->parent == this)
		return;

	if (child->parent)
		child->parent->removeChild(child.get());

	// Clamp index to valid range [0, children.size()]
	size_t idx = 0;
	if (index > 0)
		idx = static_cast<size_t>(index);
	if (idx > this->children.size()) idx = this->children.size();

	child->parent = this;
	child->setWorldDirty(true);
	children.insert(children.begin() + idx, std::move(child));
}

std::unique_ptr<GameObject> GameObject::removeChild(GameObject* node)
{
	if (!node) return nullptr;

	auto found = std::find_if(children.begin(), children.end(),
		[node](const GameObjectPtr& child)
		{
			return child.get() == node;
		});

	if (found == children.end())	return nullptr;
	
	(*found)->parent = nullptr; //clear parent
	
	GameObjectPtr result = std::move(*found);

	children.erase(found);

	result->setWorldDirty(true);

	return result;
}

std::vector<GameObject*> GameObject::getChildren() const
{
	std::vector<GameObject*> result;

	for(const auto& childPtr : this->children)
	{
		result.push_back(childPtr.get());
	}

	return result;
}

std::vector<GameObject*> GameObject::getChildrenRecursive() const
{
	std::vector<GameObject*> result;
	std::vector<GameObject*> stack;

	for (const auto& childPtr : this->children)
	{
		if (childPtr)
			stack.push_back(childPtr.get());
	}

	while (!stack.empty())
	{
		GameObject* node = stack.back();
		stack.pop_back();

		if (!node) continue;

		result.push_back(const_cast<GameObject*>(node));

		auto direct = node->getChildren();
		for (const auto& grandChildPtr : node->children)
		{
			if (grandChildPtr)
				stack.push_back(grandChildPtr.get());
		}
	}

	return result;
}

void GameObject::setParent(GameObject* newParent)
{
	if (newParent == this || (newParent && isDescendantOf(newParent)))
		return;

	GameObjectPtr thisUnique;

	if (parent)
		thisUnique = parent->removeChild(this);
	else
		thisUnique = ModelManager::getInstance()->removeObject(this);

	parent = newParent;

	if (parent)
		parent->addChild(std::move(thisUnique));
	else
	{
		ModelManager::getInstance()->addObject(std::move(thisUnique));
	}

	this->setWorldDirty(true);
}

GameObject* GameObject::getParent() const
{
	return this->parent;
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

void GameObject::setOBB(const BoundingBox& obb)
{
	this->obb = std::make_shared<BoundingBox>(obb);
}

std::shared_ptr<BoundingBox> GameObject::getOBB() const
{
	return this->obb;
}

GameObject::mat4 GameObject::getLocalMatrix()
{
	if(this->localDirty)
	{
		glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), this->localScale);
		glm::mat4 rotationMat = glm::yawPitchRoll(glm::radians(localRotation.y), glm::radians(localRotation.x), glm::radians(localRotation.z));
		glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), this->localPosition);

		this->localMatrix = translationMat * rotationMat * scaleMat;
		this->localDirty = false;
		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	}

	return this->localMatrix;
}

GameObject::mat4 GameObject::getWorldMatrix()
{
	if (this->worldDirty)
	{
		glm::mat4 localMat = getLocalMatrix();

		if (parent) 
		{
			this->worldMatrix = parent->getWorldMatrix() * localMat;
		}
		else 
		{
			this->worldMatrix = localMat;
		}

		// Decompose mat_ to get world position, rotation, scale
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::quat rotationQuat;
		glm::decompose(localMatrix, worldScale, rotationQuat, worldPosition, skew, perspective);
		worldRotation = glm::degrees(glm::eulerAngles(rotationQuat));

		this->worldDirty = false;

		// Update bounding box and model matrix
		if (modelRef && !modelRef->Vertices().empty()) 
		{
			std::vector<glm::vec3> worldPositions;
			worldPositions.reserve(modelRef->Vertices().size());

			for (const auto& vertex : modelRef->Vertices()) 
			{
				glm::vec3 posWorld = glm::vec3(localMatrix * glm::vec4(vertex.Position, 1.0f));
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
		if (modelRef) 
		{
			modelRef->Transform(localMatrix);

			if (RayTracer::getInstance()->getUserSettings().IsRayTraced)
			{
				EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
			}
		}
	}

	return this->worldMatrix;

}

bool GameObject::isLocalDirty()
{
	return this->localDirty;
}

void GameObject::setLocalDirty(bool propagate)
{
	this->localDirty = true;
	this->worldDirty = true;

	if(propagate)
	{
		for (auto& child : children)
		{
			if (child)
			{
				child->setLocalDirty(true);
			}
		}
	}
}

bool GameObject::isWorldDirty()
{
	return this->worldDirty;
}

void GameObject::setWorldDirty(bool propagate)
{
	this->worldDirty = true;
	if (propagate)
	{
		for (auto& child : children)
		{
			if (child)
			{
				child->setWorldDirty(true);
			}
		}
	}
}

// for Child Gameobjects
void GameObject::updateSceneView()
{
	if (RayTracer::getInstance()->getUserSettings().IsRayTraced) {
		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
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

