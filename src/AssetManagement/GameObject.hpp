#pragma once
#include <memory>
#include <string>
#include <vector>

#include "glm/gtx/quaternion.hpp"
#include "From-GDGRAP2/VectorUtils.h"

namespace Assets
{
    class Model;
    class Material;
}

class GameObject
{
public:
    using GameObjectPtr = std::unique_ptr<GameObject>;
	using ModelPtr = std::shared_ptr<Assets::Model>;
	using MaterialPtr = std::shared_ptr<Assets::Material>;
    using vec3 = glm::vec3;
    using mat4 = glm::mat4;
	using quat = glm::quat;
    using String = std::string;

    enum PrimitiveType {
        CAMERA, CUBE, OBJECT_GROUP, QUAD, PLANE, CYLINDER, CAPSULE, SPHERE,
        POINT_LIGHT, DIRECTIONAL_LIGHT, SPOT_LIGHT, MESH, CORNELL_BOX, NONE
    };

    GameObject();
    GameObject(String name, PrimitiveType type);
    GameObject(String name, PrimitiveType type, ModelPtr modelRef, MaterialPtr materialRef);
    ~GameObject() = default;

    GameObject(const GameObject& other);
    virtual GameObjectPtr Clone() const;

	void SetName(std::string name) { m_name = name; }
	String GetName() const { return m_name; }

	PrimitiveType GetType() const { return m_type; }

	bool IsActive() const { return m_active; }
    void SetActive(bool flag);

	bool IsVisible() const { return m_visible; }
    void SetVisible(bool flag);

	bool IsPickable() const { return m_pickable; }
    void SetPickable(bool flag);

    virtual void SetLocalPosition(vec3 newPos);
    virtual void SetLocalPosition(float x, float y, float z);
	vec3 GetLocalPosition() const { return m_localPosition; }
	vec3 GetWorldPosition() const { return m_worldPosition; }

    void SetLocalRotationEuler(const vec3& eulerDeg);
    void SetLocalRotationQuat(const quat& q);
    virtual void SetLocalRotation(float x, float y, float z);
    vec3 GetLocalRotationEuler() const { return m_localRotationEuler; }
    quat GetLocalRotationQuat() const { return m_localRotationQuat; }
    vec3 GetWorldRotationEuler() const { return m_worldRotationEuler; }
    quat GetWorldRotationQuat() const { return m_worldRotationQuat; }

    void SetLocalScale(vec3 newScale);
    void SetLocalScale(float x, float y, float z);
	vec3 GetLocalScale() const { return m_localScale; }
	vec3 GetWorldScale() const { return m_worldScale; }

    void AddChild(GameObjectPtr child);
    void AddChildAtIndex(GameObjectPtr child, int index);
    GameObjectPtr RemoveChild(GameObject* child);
	std::vector<GameObject*> GetChildren() const; // gets direct children
    std::vector<GameObject*> GetChildrenRecursive() const; // gets all descendants
	int GetChildIndex(GameObject* child) const;

    void SetParent(GameObject* newParent);
	GameObject* GetParent() const { return m_parent; }

    glm::mat4 GetLocalMatrix();
    glm::mat4 GetWorldMatrix();
    glm::mat4 GetWorldMatrixInverse();

	bool IsHierarchyNodeOpen() const { return m_isHierarchyNodeOpen; }
	void SetHierarchyNodeOpen(bool isOpen) { m_isHierarchyNodeOpen = isOpen; }

	void SetModel(ModelPtr modelReference) { m_model = modelReference; }
    ModelPtr GetModel() const { return m_model; }

	void SetMaterial(MaterialPtr materialReference) { m_material = materialReference; }
    MaterialPtr GetMaterial() const { return m_material; }

	void SetID(uint32_t id) { m_id = id; }
	uint32_t GetID() const { return m_id; }


private:
    bool IsDescendantOf(const GameObject* potentialParent) const;

    void SetLocalDirty();
	bool IsLocalDirty() const { return m_isLocalDirty; }
    void SetWorldDirty();
	bool IsWorldDirty() const { return m_isWorldDirty; }

    String m_name;
    PrimitiveType m_type;

    bool m_active = true;
    bool m_visible = true;
    bool m_pickable = true;

    vec3 m_localPosition = VectorUtils::zeros();
    vec3 m_localRotationEuler = VectorUtils::zeros();
    quat m_localRotationQuat = quat(1.0f, 0.0f, 0.0f, 0.0f);
    vec3 m_localScale = VectorUtils::ones();

    vec3 m_worldPosition = VectorUtils::zeros();
    vec3 m_worldRotationEuler = VectorUtils::zeros();
    quat m_worldRotationQuat = quat(1.0f, 0.0f, 0.0f, 0.0f);
    vec3 m_worldScale = VectorUtils::ones();

    glm::mat4 m_localMatrix = glm::mat4(1.0);
    glm::mat4 m_worldMatrix = glm::mat4(1.0);

    /* Extend to use IDs to reference models and materials */
    ModelPtr m_model;
    MaterialPtr m_material;

	uint32_t m_id = 0;

    GameObject* m_parent = nullptr;
    std::vector<GameObjectPtr> m_children;

	bool m_isLocalDirty = true;
	bool m_isWorldDirty = true;
    bool m_isHierarchyNodeOpen = true;
};


