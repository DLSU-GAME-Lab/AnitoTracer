#pragma once
#include <memory>
#include <string>
#include <vector>

#include <glm/gtx/quaternion.hpp>
#include "Assets/Model.hpp"
#include "From-GDGRAP2/VectorUtils.h"

// Forward declaration for PhysicsComponent
namespace Anito::Physics {
	class PhysicsComponent;
	struct PhysicsBodySettings;
}



class GameObject
{
public:
    using GameObjectPtr = std::unique_ptr<GameObject>;
    using vec3 = glm::vec3;
	using quat = glm::quat;
    using mat4 = glm::mat4;
    using String = std::string;

    enum PrimitiveType {
        CAMERA, CUBE, OBJECT_GROUP, QUAD, PLANE, CYLINDER, CAPSULE, SPHERE,
        POINT_LIGHT, DIRECTIONAL_LIGHT, SPOT_LIGHT, MESH, CORNELL_BOX, NONE
    };

    GameObject();
    GameObject(String name, PrimitiveType type);
    GameObject(String name, PrimitiveType type, std::shared_ptr<Assets::Model> modelRef);
    ~GameObject();

    GameObject(const GameObject& other);
    virtual GameObject::GameObjectPtr Clone() const;

    void setName(std::string name);
    String getName() const;

    PrimitiveType getType() const;

    bool isActive();
    void setActive(bool flag);

    bool isVisible();
    void setVisible(bool flag);

    bool isPickable();
    void setPickable(bool flag);

    virtual void setLocalPosition(vec3 newPos);
    virtual void setLocalPosition(float x, float y, float z);
    vec3 getLocalPosition() const;
    vec3 getWorldPosition() const;

    virtual void setLocalRotationEuler(vec3 newRot);
    virtual void setLocalRotationEuler(float x, float y, float z);
    virtual void setLocalRotationQuat(quat newRot);
    vec3 getLocalRotationEuler() const;
    vec3 getWorldRotationEuler() const;
	quat getLocalRotationQuat() const;
	quat getWorldRotationQuat() const;

    void setLocalScale(vec3 newScale);
    void setLocalScale(float x, float y, float z);
    vec3 getLocalScale() const;
    vec3 getWorldScale() const;

    std::shared_ptr<Assets::Model> getModel();
	void setModel(std::shared_ptr<Assets::Model> modelRef);

    void addChild(GameObjectPtr child);
    void addChildAtIndex(GameObjectPtr child, int index);
    GameObjectPtr removeChild(GameObject* child);
    std::vector<GameObject*> getChildren() const;
    std::vector<GameObject*> getChildrenRecursive() const; // gets all descendants
	int getChildIndex(GameObject* child) const;

    void setParent(GameObject* newParent);
    GameObject* getParent() const;

    bool isDescendantOf(const GameObject* potentialParent) const;

    void updateLocalMatrix();
    glm::mat4 getLocalMatrix() const;
    void updateWorldMatrix();
    glm::mat4 getWorldMatrix() const;
	glm::mat4 getWorldMatrixInverse() const;

	void setLocalDirty();
	bool isLocalDirty() const;
	void setWorldDirty();
	bool isWorldDirty() const;
    void clearDirtyFlag();
    bool wasDirty() const;

	bool IsHierarchyNodeOpen() const;
	void SetHierarchyNodeOpen(bool isOpen);

	// --- Physics Component ---

	/**
	 * @brief Add a physics component to this GameObject
	 * @param settings The physics body settings for this component
	 */
	void AddPhysicsComponent(const Anito::Physics::PhysicsBodySettings& settings);

	/**
	 * @brief Remove the physics component from this GameObject
	 */
	void RemovePhysicsComponent();

	/**
	 * @brief Check if this GameObject has a physics component
	 */
	bool HasPhysicsComponent() const;

	/**
	 * @brief Get the physics component attached to this GameObject
	 * @return Pointer to the PhysicsComponent, or nullptr if not attached
	 */
	Anito::Physics::PhysicsComponent* GetPhysicsComponent();
	const Anito::Physics::PhysicsComponent* GetPhysicsComponent() const;

protected:
    String name;
    PrimitiveType type;

    bool active = true;
    bool visible = true;
    bool pickable = true;

    vec3 localPosition = VectorUtils::zeros();
    quat localRotationQuat = glm::quat(1,0,0,0);
	vec3 localRotationEuler = VectorUtils::zeros();
    vec3 localScale = VectorUtils::ones();

    vec3 worldPosition = VectorUtils::zeros();
	quat worldRotationQuat = glm::quat(1, 0, 0, 0);
    vec3 worldRotationEuler = VectorUtils::zeros();
    vec3 worldScale = VectorUtils::ones();

    glm::mat4 localMatrix = glm::mat4(1.0);
    glm::mat4 worldMatrix = glm::mat4(1.0);

    std::shared_ptr<Assets::Model> modelRef;

    GameObject* parent = nullptr;
    std::vector<GameObjectPtr> children;

    void updateSceneView();

	bool localDirty = true;
	bool worldDirty = true;
    bool m_wasDirty = true;

    bool isHierarchyNodeOpen = true;

    // Physics component (optional) - stored as void* to avoid including full header
    // Access through GetPhysicsComponent() methods
    mutable void* mPhysicsComponent = nullptr;

    friend class ModelManager;
};


