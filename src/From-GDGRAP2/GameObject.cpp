#include "GameObject.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "EventBroadcaster.h"
#include "ModelManager.h"
#include "RayTracer.hpp"
#include "Engine/Physics/PhysicsComponent.hpp"


static glm::vec3 ExtractScale(const glm::mat4& m)
{
	glm::vec3 sx = glm::vec3(m[0][0], m[0][1], m[0][2]);
	glm::vec3 sy = glm::vec3(m[1][0], m[1][1], m[1][2]);
	glm::vec3 sz = glm::vec3(m[2][0], m[2][1], m[2][2]);
	return glm::vec3(glm::length(sx), glm::length(sy), glm::length(sz));
}

static glm::quat ExtractRotation(const glm::mat4& m)
{
	glm::mat3 rot{};
	glm::vec3 scale = ExtractScale(m);
	if (scale.x != 0.0f) rot[0] = glm::vec3(m[0]) / scale.x; else rot[0] = glm::vec3(m[0]);
	if (scale.y != 0.0f) rot[1] = glm::vec3(m[1]) / scale.y; else rot[1] = glm::vec3(m[1]);
	if (scale.z != 0.0f) rot[2] = glm::vec3(m[2]) / scale.z; else rot[2] = glm::vec3(m[2]);
	return glm::quat_cast(rot);
}

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

GameObject::~GameObject()
{
	// Clean up physics component if attached
	RemovePhysicsComponent();
}

GameObject::GameObject(const GameObject& other) : name(other.name), type(other.type), active(other.active), visible(other.visible), pickable(other.pickable), 
localPosition(other.localPosition), localRotationQuat(other.localRotationQuat), localRotationEuler(other.localRotationEuler), localScale(other.localScale), localMatrix(other.localMatrix), 
worldPosition(other.worldPosition), worldRotationQuat(other.worldRotationQuat), worldRotationEuler(other.worldRotationEuler),worldScale(other.worldScale), worldMatrix(other.worldMatrix), 
localDirty(other.localDirty), worldDirty(other.worldDirty), m_wasDirty(other.m_wasDirty), isHierarchyNodeOpen(other.isHierarchyNodeOpen)
{
	this->parent = nullptr;
	this->modelRef = other.modelRef->Clone();
	this->modelRef->SetOwner(this);
	this->mPhysicsComponent = nullptr;  // Don't copy physics component; it should be added separately

	for (const auto& child : other.children)
	{
		std::unique_ptr<GameObject> clonedChild = child->Clone();
		addChild(std::move(clonedChild));
	}
}

GameObject::GameObjectPtr GameObject::Clone() const
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
	this->setLocalDirty();
}

void GameObject::setLocalPosition(float x, float y, float z)
{
	this->localPosition = vec3(x, y, z);
	this->setLocalDirty();
}

GameObject::vec3 GameObject::getLocalPosition() const
{
	return this->localPosition;
}

GameObject::vec3 GameObject::getWorldPosition() const
{
	return this->worldPosition;
}

void GameObject::setLocalRotationEuler(vec3 newRot)
{
	localRotationEuler = newRot;
	localRotationQuat = glm::quat(glm::radians(newRot));
	setLocalDirty();
}

void GameObject::setLocalRotationEuler(float x, float y, float z)
{
	setLocalRotationEuler(vec3(x, y, z));
}

void GameObject::setLocalRotationQuat(quat newRot)
{
	localRotationQuat = newRot;
	localRotationEuler = glm::degrees(glm::eulerAngles(newRot));
	setLocalDirty();
}

GameObject::vec3 GameObject::getLocalRotationEuler() const
{
	return this->localRotationEuler;
}

GameObject::vec3 GameObject::getWorldRotationEuler() const
{
	return this->worldRotationEuler;
}

GameObject::quat GameObject::getLocalRotationQuat() const
{
	return this->localRotationQuat;
}

GameObject::quat GameObject::getWorldRotationQuat() const
{
	return this->worldRotationQuat;
}

void GameObject::setLocalScale(vec3 newScale)
{
	this->localScale = newScale;
	this->setLocalDirty();
}

void GameObject::setLocalScale(float x, float y, float z)
{
	this->localScale = vec3(x, y, z);
	this->setLocalDirty();
}

GameObject::vec3 GameObject::getLocalScale() const
{
	return this->localScale;
}

GameObject::vec3 GameObject::getWorldScale() const
{
	return this->worldScale;
}

std::shared_ptr<Assets::Model> GameObject::getModel()
{
	this->updateWorldMatrix();
	return this->modelRef;
}

void GameObject::setModel(std::shared_ptr<Assets::Model> modelRef)
{
	this->modelRef = modelRef;
	if (this->modelRef)
		this->modelRef->SetOwner(this);
}

void GameObject::addChild(GameObject::GameObjectPtr child)
{
	if (!child || child.get() == this || child->parent == this)
		return;

	if (child->parent)
		child->parent->removeChild(child.get());

	child->parent = this;

	glm::mat4 parentWorldInverse = glm::inverse(this->getWorldMatrix());
	child->localMatrix = parentWorldInverse * child->getWorldMatrix();

	// Decompose to update local position, rotation, scale
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::quat rotationQuat;
	glm::decompose(child->localMatrix, child->localScale, rotationQuat,
		child->localPosition, skew, perspective);
	child->localRotationEuler = glm::degrees(glm::eulerAngles(rotationQuat));

	child->localDirty = false;
	child->setWorldDirty();

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
	child->setWorldDirty();
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
	(*found)->setWorldDirty();
	
	GameObjectPtr result = std::move(*found);

	children.erase(found);

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

int GameObject::getChildIndex(GameObject* child) const
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
void GameObject::setParent(GameObject* newParent)
{
	if (newParent == this || (newParent && isDescendantOf(newParent)))
		return;

	parent = newParent;
	this->setWorldDirty();
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

void GameObject::updateLocalMatrix()
{
	if (this->localDirty == false)	return;

	glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), this->localScale);
	glm::mat4 rotationMat = glm::toMat4(this->localRotationQuat);
	glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), this->localPosition);

	this->localMatrix = translationMat * rotationMat * scaleMat;
	this->localDirty = false;
}

glm::mat4 GameObject::getLocalMatrix() const
{
	return this->localMatrix;
}

void GameObject::updateWorldMatrix()
{
	if(this->worldDirty == false)	return;

	this->updateLocalMatrix();
	
	auto localMat = this->getLocalMatrix();

	if (parent)
	{
		this->worldMatrix = parent->getWorldMatrix() * localMat;
		this->worldRotationQuat = parent->getWorldRotationQuat() * this->localRotationQuat;
	}
	else
	{
		this->worldMatrix = localMat;
		this->worldRotationQuat = this->localRotationQuat;
	}

	worldPosition = glm::vec3(worldMatrix[3]);
	worldScale = ExtractScale(worldMatrix);
	worldRotationEuler = glm::degrees(glm::eulerAngles(worldRotationQuat));

	this->worldDirty = false;
}

glm::mat4 GameObject::getWorldMatrix() const
{
	return this->worldMatrix;
}

glm::mat4 GameObject::getWorldMatrixInverse() const
{
	return glm::inverse(this->worldMatrix);
}

// for Child Gameobjects
void GameObject::updateSceneView()
{
	if (RayTracer::getInstance()->getUserSettings().IsRayTraced) {
		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	}
}

void GameObject::setLocalDirty()
{
	this->localDirty = true;
	this->worldDirty = true;

	for(const auto& child : this->children)
	{
		if (child)
			child->setLocalDirty();
	}
}

bool GameObject::isLocalDirty() const
{
	return localDirty;
}

void GameObject::setWorldDirty()
{
	this->worldDirty = true;
	for (const auto& child : this->children)
	{
		if (child)
			child->setWorldDirty();
	}
}

bool GameObject::isWorldDirty() const
{
	return this->worldDirty;
}

void GameObject::clearDirtyFlag()
{
	this->m_wasDirty = true;
}

bool GameObject::wasDirty() const
{
	return this->m_wasDirty;
}

bool GameObject::IsHierarchyNodeOpen() const
{
	return this->isHierarchyNodeOpen;
}

void GameObject::SetHierarchyNodeOpen(bool isOpen)
{
	this->isHierarchyNodeOpen = isOpen;
}

// --- Physics Component Methods ---

void GameObject::AddPhysicsComponent(const Anito::Physics::PhysicsBodySettings& settings)
{
	if (mPhysicsComponent) {
		return; // Already has a physics component
	}

	auto* physicsComp = new Anito::Physics::PhysicsComponent(this, settings);
	mPhysicsComponent = physicsComp;

	// Initialize creates the body and registers it with the physics world
	physicsComp->Initialize();

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void GameObject::RemovePhysicsComponent()
{
	if (mPhysicsComponent) {
		auto* physicsComp = static_cast<Anito::Physics::PhysicsComponent*>(mPhysicsComponent);
		physicsComp->Cleanup();
		delete physicsComp;
		mPhysicsComponent = nullptr;
		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	}
}

bool GameObject::HasPhysicsComponent() const
{
	return mPhysicsComponent != nullptr;
}

Anito::Physics::PhysicsComponent* GameObject::GetPhysicsComponent()
{
	return static_cast<Anito::Physics::PhysicsComponent*>(mPhysicsComponent);
}

const Anito::Physics::PhysicsComponent* GameObject::GetPhysicsComponent() const
{
	return static_cast<const Anito::Physics::PhysicsComponent*>(mPhysicsComponent);
}
