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

	this->updateWorldMatrix();
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

GameObject::GameObject(String name, PrimitiveType type)
{
	this->name = name;
	this->type = type;
	this->modelRef = nullptr;

	this->updateWorldMatrix();
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

GameObject::GameObject(String name, PrimitiveType type, std::shared_ptr<Assets::Model> modelRef)
{
	this->name = name;
	this->type = type;
	this->modelRef = modelRef;
	this->modelRef->SetOwner(this);

	this->updateWorldMatrix();
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

GameObject::GameObject(const GameObject& other) : name(other.name), m_id(other.m_id), type(other.type),
	localPosition(other.localPosition), localRotation(other.localRotation), localScale(other.localScale),
	worldPosition(other.worldPosition), worldRotation(other.worldRotation), worldScale(other.worldScale),
	localMatrix(other.localMatrix), worldMatrix(other.worldMatrix),
	isActive(other.isActive), isVisible(other.isVisible), isPickable(other.isPickable),
	debugCube(other.debugCube), modelRef(other.modelRef), parent(nullptr), 
	isLocalDirty(other.isLocalDirty),isWorldDirty(other.isWorldDirty),
	isHierarchyNodeOpen(other.isHierarchyNodeOpen)
{
	children.reserve(other.children.size());
	for (const auto& child : other.children)
	{
		this->AddChild(std::make_unique<GameObject>(*child));
	}
}

std::unique_ptr<GameObject> GameObject::Copy(GameObject original)
{
	return std::make_unique<GameObject>(*this);
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

bool GameObject::IsActive()
{
	return this->isActive;
}

void GameObject::SetActive(bool flag)
{
	this->isActive = flag;

	for(const auto& child : this->children)
	{
		if (child)
			child->SetActive(flag);
	}
}

bool GameObject::IsVisible()
{
	return this->isVisible;
}

void GameObject::SetVisible(bool flag)
{
	this->isVisible = flag;

	for (const auto& child : this->children)
	{
		if (child)
			child->SetVisible(flag);
	}
}

bool GameObject::IsPickable()
{
	return this->isPickable;
}

void GameObject::SetPickable(bool flag)
{
	this->isPickable = flag;

	for (const auto& child : this->children)
	{
		if (child)
			child->SetPickable(flag);
	}
}

void GameObject::SetLocalPosition(vec3 newPos)
{
	this->localPosition = newPos;
	this->SetLocalDirty();
}

void GameObject::SetLocalPosition(float x, float y, float z)
{
	this->localPosition = vec3(x, y, z);
	this->SetLocalDirty();
}

GameObject::vec3 GameObject::GetLocalPosition() const
{
	return this->localPosition;
}

GameObject::vec3 GameObject::GetWorldPosition() const
{
	return this->worldPosition;
}

void GameObject::SetLocalRotation(vec3 newRot)
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
	this->SetLocalDirty();
}

void GameObject::SetLocalRotation(float x, float y, float z)
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
	this->SetLocalDirty();
}

GameObject::vec3 GameObject::GetLocalRotation() const
{
	return this->localRotation;
}

GameObject::vec3 GameObject::GetWorldRotation() const
{
	return this->worldRotation;
}

void GameObject::SetLocalScale(vec3 newScale)
{
	this->localScale = newScale;
	this->SetLocalDirty();
}

void GameObject::SetLocalScale(float x, float y, float z)
{
	this->localScale = vec3(x, y, z);
	this->SetLocalDirty();
}

GameObject::vec3 GameObject::GetLocalScale() const
{
	return this->localScale;
}

GameObject::vec3 GameObject::GetWorldScale() const
{
	return this->worldScale;
}

std::shared_ptr<Assets::Model> GameObject::GetModel() const
{
	return this->modelRef;
}

void GameObject::SetModel(std::shared_ptr<Assets::Model> model)
{
	this->modelRef = model;
}

void GameObject::AddChild(GameObject::GameObjectPtr child)
{
	if (!child || child.get() == this || child->parent == this)
		return;

	if (child->parent)
		child->parent->RemoveChild(child.get());

	child->parent = this;

	glm::mat4 parentWorldInverse = glm::inverse(this->getWorldMatrix());
	child->localMatrix = parentWorldInverse * child->getWorldMatrix();

	// Decompose to update local position, rotation, scale
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::quat rotationQuat;
	glm::decompose(child->localMatrix, child->localScale, rotationQuat,
		child->localPosition, skew, perspective);
	child->localRotation = glm::degrees(glm::eulerAngles(rotationQuat));

	child->isLocalDirty = false;
	child->SetWorldDirty();

	children.push_back(std::move(child));
}

void GameObject::AddChildAtIndex(GameObjectPtr child, int index)
{
	if (!child || child.get() == this || child->parent == this)
		return;

	if (child->parent)
		child->parent->RemoveChild(child.get());

	// Clamp index to valid range [0, children.size()]
	size_t idx = 0;
	if (index > 0)
		idx = static_cast<size_t>(index);
	if (idx > this->children.size()) idx = this->children.size();

	child->parent = this;
	child->SetWorldDirty();
	children.insert(children.begin() + idx, std::move(child));
}

std::unique_ptr<GameObject> GameObject::RemoveChild(GameObject* node)
{
	if (!node) return nullptr;

	auto found = std::find_if(children.begin(), children.end(),
		[node](const GameObjectPtr& child)
		{
			return child.get() == node;
		});

	if (found == children.end())	return nullptr;
	
	(*found)->parent = nullptr; //clear parent
	(*found)->SetWorldDirty();
	
	GameObjectPtr result = std::move(*found);

	children.erase(found);

	return result;
}

std::vector<GameObject*> GameObject::GetChildren() const
{
	std::vector<GameObject*> result;

	for(const auto& childPtr : this->children)
	{
		result.push_back(childPtr.get());
	}

	return result;
}

std::vector<GameObject*> GameObject::GetChildrenRecursive() const
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

		auto direct = node->GetChildren();
		for (const auto& grandChildPtr : node->children)
		{
			if (grandChildPtr)
				stack.push_back(grandChildPtr.get());
		}
	}

	return result;
}

int GameObject::GetChildIndex(GameObject* child) const
{
	if (!child) return -1;

	for (size_t i = 0; i < children.size(); ++i)
	{
		if (children[i].get() == child)
		{
			return static_cast<int>(i);
		}
	}

	return -1;
}

/* Only adds parent ref, unique ptr still needs to be moved to children of parent */
void GameObject::SetParent(GameObject* newParent)
{
	if (newParent == this || (newParent && IsDescendantOf(newParent)))
		return;

	parent = newParent;
	this->SetWorldDirty();
}

GameObject* GameObject::GetParent() const
{
	return this->parent;
}

bool GameObject::IsDescendantOf(const GameObject* potentialParent) const
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

void GameObject::updateLocalMatrix()
{
	if (this->isLocalDirty == false)	return;

	glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), this->localScale);
	glm::mat4 rotationMat = glm::yawPitchRoll(glm::radians(localRotation.y), glm::radians(localRotation.x), glm::radians(localRotation.z));
	glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), this->localPosition);

	this->localMatrix = translationMat * rotationMat * scaleMat;
	this->isLocalDirty = false;
}

glm::mat4 GameObject::getLocalMatrix() const
{
	return this->localMatrix;
}

void GameObject::updateWorldMatrix()
{
	if(this->isWorldDirty == false)	return;

	this->updateLocalMatrix();
	
	auto localMat = this->getLocalMatrix();

	if (parent) 
	{
		this->worldMatrix = parent->getWorldMatrix() * localMat;
	}
	else
	{
		this->worldMatrix = localMat;
	}

	this->isWorldDirty = false;

	// Decompose mat_ to get world position, rotation, scale
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::quat rotationQuat;
	glm::decompose(worldMatrix, worldScale, rotationQuat, worldPosition, skew, perspective);
	worldRotation = glm::degrees(glm::eulerAngles(rotationQuat));
}

glm::mat4 GameObject::getWorldMatrix() 
{
	this->updateWorldMatrix();
	return this->worldMatrix;
}

glm::mat4 GameObject::getWorldMatrixInverse() const
{
	return glm::inverse(this->worldMatrix);
}

void GameObject::SetLocalDirty()
{
	this->isLocalDirty = true;
	this->isWorldDirty = true;

	for(const auto& child : this->children)
	{
		if (child) child->SetWorldDirty();
	}
}

bool GameObject::IsLocalDirty() const
{
	return isLocalDirty;
}

void GameObject::SetWorldDirty()
{
	this->isWorldDirty = true;

	for (const auto& child : this->children)
	{
		if (child) child->SetWorldDirty();
	}
}

bool GameObject::IsWorldDirty() const
{
	return this->isWorldDirty;
}

bool GameObject::IsHierarchyNodeOpen() const
{
	return this->isHierarchyNodeOpen;
}

void GameObject::SetHierarchyNodeOpen(bool isOpen)
{
	this->isHierarchyNodeOpen = isOpen;
}

void GameObject::SetId(uint32_t id)
{
	this->m_id = id;
}

uint32_t GameObject::GetId()
{
	return this->m_id;
}

void GameObject::updateSceneView()
{
	if (RayTracer::getInstance()->getUserSettings().IsRayTraced) {
		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	}
}
