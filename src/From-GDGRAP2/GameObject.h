#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Assets/Model.hpp"
#include "From-GDGRAP2/VectorUtils.h"
#include "OBB/AABB.hpp"

class GameObject
{
public:
    using GameObjectPtr = std::unique_ptr<GameObject>;
    using vec3 = glm::vec3;
    using String = std::string;
    using mat4 = glm::mat4;

    enum PrimitiveType {
        CAMERA, CUBE, OBJECT_GROUP, QUAD, PLANE, CYLINDER, CAPSULE, SPHERE,
        POINT_LIGHT, DIRECTIONAL_LIGHT, SPOT_LIGHT, MESH, CORNELL_BOX, NONE
    };

    GameObject();
    GameObject(String name, PrimitiveType type);
    GameObject(String name, PrimitiveType type, std::shared_ptr<Assets::Model> modelRef);
    GameObject(const GameObject& other);
    ~GameObject() = default;

    virtual std::unique_ptr<GameObject> Clone();

    void setName(std::string name);
    String getName() const;

    PrimitiveType getType() const;

    bool IsActive();
    void SetActive(bool flag);

    bool IsVisible();
    void SetVisible(bool flag);

    bool IsPickable();
    void SetPickable(bool flag);

    virtual void SetLocalPosition(vec3 newPos);
    virtual void SetLocalPosition(float x, float y, float z);
    vec3 GetLocalPosition() const;
    vec3 GetWorldPosition() const;

    virtual void SetLocalRotation(vec3 newRot);
    virtual void SetLocalRotation(float x, float y, float z);
    vec3 GetLocalRotation() const;
    vec3 GetWorldRotation() const;

    void SetLocalScale(vec3 newScale);
    void SetLocalScale(float x, float y, float z);
    vec3 GetLocalScale() const;
    vec3 GetWorldScale() const;

    std::shared_ptr<Assets::Model> GetModel() const;
    void SetModel(std::shared_ptr<Assets::Model> model);

    void AddChild(GameObjectPtr child);
    void AddChildAtIndex(GameObjectPtr child, int index);
    GameObjectPtr RemoveChild(GameObject* child);
    std::vector<GameObject*> GetChildren() const;
    std::vector<GameObject*> GetChildrenRecursive() const; // gets all descendants
	int GetChildIndex(GameObject* child) const;

    void SetParent(GameObject* newParent);
    GameObject* GetParent() const;

    bool IsDescendantOf(const GameObject* potentialParent) const;

    void updateLocalMatrix();
    glm::mat4 getLocalMatrix() const;
    void updateWorldMatrix();
    glm::mat4 getWorldMatrix();
	glm::mat4 getWorldMatrixInverse() const;

	void SetLocalDirty();
	bool IsLocalDirty() const;
	void SetWorldDirty();
	bool IsWorldDirty() const;
    void ClearDirtyFlag();
    bool WasDirty() const;

	bool IsHierarchyNodeOpen() const;
	void SetHierarchyNodeOpen(bool isOpen);

    void SetId(uint32_t id);
    uint32_t GetId();

    void updateSceneView();

	AABB& GetAABB() { return *aabb; }

protected:
    String name;

    vec3 localPosition = VectorUtils::zeros();
    vec3 localRotation = VectorUtils::zeros();
    vec3 localScale = VectorUtils::ones();

    vec3 worldPosition = VectorUtils::zeros();
    vec3 worldRotation = VectorUtils::zeros();
    vec3 worldScale = VectorUtils::ones();

    glm::mat4 localMatrix = glm::mat4(1.0);
    glm::mat4 worldMatrix = glm::mat4(1.0);

    PrimitiveType type;

private:
    bool isActive = true;
    bool isVisible = true;
    bool isPickable = true;

    bool wasDirty = true; //TLAS update flag

    std::shared_ptr<GameObject> debugCube = nullptr;

    std::shared_ptr<Assets::Model> modelRef;
	std::shared_ptr<AABB> aabb = nullptr;

    GameObject* parent = nullptr;
    std::vector<GameObjectPtr> children;

    bool isLocalDirty = true;
    bool isWorldDirty = true;

    bool isHierarchyNodeOpen = true;

    uint32_t m_id = 0;
};


