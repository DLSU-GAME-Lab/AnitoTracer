#include "GameObject.hpp"

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

GameObject::GameObject() : m_name("GameObject"), m_type(NONE), m_model(nullptr), m_material(nullptr) {}
GameObject::GameObject(String name, PrimitiveType type) : m_name(name), m_type(type), m_model(nullptr), m_material(nullptr) {}
GameObject::GameObject(String name, PrimitiveType type, ModelPtr modelRef, MaterialPtr materialRef) : m_name(name), m_type(type), m_model(modelRef), m_material(materialRef) {}

GameObject::GameObject(const GameObject& other) : m_name(other.m_name), m_type(other.m_type), m_active(other.m_active), m_visible(other.m_visible),
	m_pickable(other.m_pickable), m_localPosition(other.m_localPosition), m_localRotationEuler(other.m_localRotationEuler), m_localRotationQuat(other.m_localRotationQuat),
	m_localScale(other.m_localScale), m_localMatrix(other.m_localMatrix), m_worldPosition(other.m_worldPosition), m_worldRotationEuler(other.m_worldRotationEuler),
	m_worldRotationQuat(other.m_worldRotationQuat), m_worldScale(other.m_worldScale), m_worldMatrix(other.m_worldMatrix), m_isLocalDirty(other.m_isLocalDirty),
	m_isWorldDirty(other.m_isWorldDirty), m_isHierarchyNodeOpen(other.m_isHierarchyNodeOpen), m_model(other.m_model), m_material(other.m_material)
{
	this->m_parent = nullptr;

	for (const auto& child : other.m_children)
	{
		std::unique_ptr<GameObject> clonedChild = child->Clone();
		AddChild(std::move(clonedChild));
	}
}

GameObject::GameObjectPtr GameObject::Clone() const
{
	return std::make_unique<GameObject>(*this);
}

void GameObject::SetActive(bool flag)
{
	this->m_active = flag;

	/* Propagate flag to descendants */
	for(const auto& child : this->m_children)
	{
		if (child)	child->SetActive(flag);
	}
}

void GameObject::SetVisible(bool flag)
{
	this->m_visible	 = flag;

	/* Propagate flag to descendants */
	for (const auto& child : this->m_children)
	{
		if (child)	child->SetVisible(flag);
	}
}

void GameObject::SetPickable(bool flag)
{
	this->m_pickable = flag;

	/* Propagate flag to descendants */
	for (const auto& child : this->m_children)
	{
		if (child)	child->SetPickable(flag);
	}
}

void GameObject::SetLocalPosition(vec3 newPos)
{
	m_localPosition = newPos; 
	SetLocalDirty();
}

void GameObject::SetLocalPosition(float x, float y, float z)
{
	SetLocalPosition(vec3(x, y, z));
}

void GameObject::SetLocalRotationEuler(const vec3& eulerDeg)
{
	m_localRotationEuler = eulerDeg;
	m_localRotationQuat = glm::quat(glm::radians(eulerDeg)); 
	SetLocalDirty();
}

void GameObject::SetLocalRotationQuat(const quat& q)
{
	m_localRotationQuat = q; 
	m_localRotationEuler = glm::degrees(glm::eulerAngles(q)); 
	SetLocalDirty();
}

void GameObject::SetLocalRotation(float x, float y, float z)
{
	SetLocalRotationEuler(vec3(x, y, z));
}

void GameObject::SetLocalScale(vec3 newScale)
{
	m_localScale = newScale; 
	SetLocalDirty();
}

void GameObject::SetLocalScale(float x, float y, float z)
{
	SetLocalScale(vec3(x, y, z));
}

void GameObject::AddChild(GameObject::GameObjectPtr child)
{
	if (!child) return;
	if (child.get() == this) return;
	if (child->IsDescendantOf(this)) return;
	if (child->m_parent == this) return;

	if (child->m_parent)
		child->m_parent->RemoveChild(child.get());

	child->m_parent = this;

	mat4 parentWorldInverse = glm::inverse(this->GetWorldMatrix());
	mat4 childWorld = child->GetWorldMatrix();
	mat4 localMat = parentWorldInverse * childWorld;

	child->m_localMatrix = localMat;
	child->m_localPosition = glm::vec3(localMat[3]);
	child->m_localScale = ExtractScale(localMat);
	child->m_localRotationQuat = ExtractRotation(localMat);
	child->m_localRotationEuler = glm::degrees(glm::eulerAngles(child->m_localRotationQuat));

	child->m_isLocalDirty = false;
	child->SetWorldDirty();

	m_children.push_back(std::move(child));
}

void GameObject::AddChildAtIndex(GameObjectPtr child, int index)
{
	if (!child) return;
	if (index < 0) index = 0;
	if (index > static_cast<int>(m_children.size())) index = static_cast<int>(m_children.size());
	if (child.get() == this) return;
	if (child->IsDescendantOf(this)) return;
	if (child->m_parent == this) return;

	if (child->m_parent)
		child->m_parent->RemoveChild(child.get());

	child->m_parent = this;

	glm::mat4 parentWorldInverse = glm::inverse(this->GetWorldMatrix());
	child->m_localMatrix = parentWorldInverse * child->GetWorldMatrix();

	mat4 parentWorldInverse = glm::inverse(this->GetWorldMatrix());
	mat4 childWorld = child->GetWorldMatrix();
	mat4 localMat = parentWorldInverse * childWorld;

	child->m_localMatrix = localMat;
	child->m_localPosition = glm::vec3(localMat[3]);
	child->m_localScale = ExtractScale(localMat);
	child->m_localRotationQuat = ExtractRotation(localMat);
	child->m_localRotationEuler = glm::degrees(glm::eulerAngles(child->m_localRotationQuat));

	child->m_isLocalDirty = false;
	child->SetWorldDirty();

	m_children.insert(m_children.begin() + index, std::move(child));
}

std::unique_ptr<GameObject> GameObject::RemoveChild(GameObject* node)
{
	if (!node) return nullptr;
	auto it = std::find_if(m_children.begin(), m_children.end(), [&](const GameObjectPtr& p) { return p.get() == node; });
	if (it == m_children.end()) return nullptr;
	GameObjectPtr removed = std::move(*it);
	m_children.erase(it);
	removed->m_parent = nullptr;
	removed->SetWorldDirty();
	return removed;
}

std::vector<GameObject*> GameObject::GetChildren() const
{
	std::vector<GameObject*> result;
	result.reserve(m_children.size());

	for (const auto& child : m_children)
	{
		if (child) result.push_back(child.get());
	}
		
	return result;
}

std::vector<GameObject*> GameObject::GetChildrenRecursive() const
{
	std::vector<GameObject*> result;
	result.reserve(m_children.size());

	for (const auto& child : this->m_children)
	{
		if (!child) continue;
		result.push_back(child.get());
		auto sub = child->GetChildrenRecursive();
		result.insert(result.end(), sub.begin(), sub.end());
	}

	return result;
}

int GameObject::GetChildIndex(GameObject* child) const
{
	if (!child) return -1;

	for (size_t i = 0; i < m_children.size(); ++i)
	{
		if (m_children[i].get() == child)
		{
			return static_cast<int>(i);
		}
	}

	return -1;
}

/* Only adds parent ref, unique ptr still needs to be moved to children of parent */
void GameObject::SetParent(GameObject* newParent)
{
	if (newParent == m_parent) return;
	if (newParent == this) return;
	if (newParent && newParent->IsDescendantOf(this)) return;

	if (m_parent)
	{
		m_parent->RemoveChild(this);
	}

	if (newParent)
	{
		GameObjectPtr me = std::make_unique<GameObject>(std::move(*this));
		newParent->AddChild(std::move(me));
	}
	else
	{
		m_parent = nullptr;
		SetWorldDirty();
	}
}

glm::mat4 GameObject::GetLocalMatrix()
{
	if (!IsLocalDirty()) return m_localMatrix;

	glm::mat4 t = glm::translate(glm::mat4(1.0f), this->m_localPosition);
	glm::mat4 r = glm::toMat4(this->m_localRotationQuat);
	glm::mat4 s = glm::scale(glm::mat4(1.0f), this->m_localScale);

	this->m_localMatrix = t * r * s;
	this->m_isLocalDirty = false;
	return this->m_localMatrix;
}

glm::mat4 GameObject::GetWorldMatrix()
{
	if (!this->IsWorldDirty())	return this->m_worldMatrix;

	auto local = this->GetLocalMatrix();

	if (this->m_parent)
	{
		m_worldMatrix = m_parent->GetWorldMatrix() * local;
		m_worldRotationQuat = m_parent->m_worldRotationQuat * m_localRotationQuat;
	}
	else
	{
		m_worldMatrix = local;
		m_worldRotationQuat = m_localRotationQuat;
	}

	m_worldPosition = glm::vec3(m_worldMatrix[3]);
	m_worldScale = ExtractScale(m_worldMatrix);
	m_worldRotationEuler = glm::degrees(glm::eulerAngles(m_worldRotationQuat));

	m_isWorldDirty = false;
	return m_worldMatrix;
}

glm::mat4 GameObject::GetWorldMatrixInverse()
{
	return glm::inverse(this->GetWorldMatrix());
}

bool GameObject::IsDescendantOf(const GameObject* potentialParent) const
{
	const GameObject* current = this->m_parent;
    while (current)
    {
        if (current == potentialParent) return true;
		current = current->m_parent;
    }
    return false;
}

void GameObject::SetLocalDirty()
{
	if (m_isLocalDirty) return;
	m_isLocalDirty = true;
	SetWorldDirty();
}

void GameObject::SetWorldDirty()
{
	if (m_isWorldDirty) return;

	m_isWorldDirty = true;
	for (auto& child : m_children)
	{
		if (child) child->SetWorldDirty();
	}
}
