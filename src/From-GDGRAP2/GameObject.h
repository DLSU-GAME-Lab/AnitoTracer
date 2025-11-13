#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Assets/Model.hpp"
#include "From-GDGRAP2/VectorUtils.h"
#include "OBB/BoundingBox.hpp"


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
    ~GameObject() = default;

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

    virtual void setLocalRotation(vec3 newRot);
    virtual void setLocalRotation(float x, float y, float z);
    vec3 getLocalRotation() const;
    vec3 getWorldRotation() const;

    void setLocalScale(vec3 newScale);
    void setLocalScale(float x, float y, float z);
    vec3 getLocalScale() const;
    vec3 getWorldScale() const;

    std::shared_ptr<Assets::Model> getModel();

    void addChild(GameObjectPtr child);
    void addChildAtIndex(GameObjectPtr child, int index);
    GameObjectPtr removeChild(GameObject* child);
    std::vector<GameObject*> getChildren() const;
    std::vector<GameObject*> getChildrenRecursive() const; // gets all descendants
	int getChildIndex(GameObject* child) const;

    void setParent(GameObject* newParent);
    GameObject* getParent() const;

    bool isDescendantOf(const GameObject* potentialParent) const;

    void setOBB(const BoundingBox& obb);
    std::shared_ptr<BoundingBox> getOBB() const;

    void updateLocalMatrix();
    glm::mat4 getLocalMatrix() const;
    void updateWorldMatrix();
    glm::mat4 getWorldMatrix() const;

protected:
    String name;
    PrimitiveType type;

    bool active = true;
    bool visible = true;
    bool pickable = true;

    std::shared_ptr<GameObject> debugCube = nullptr;

    vec3 origin = VectorUtils::zeros();
    vec3 originRot = VectorUtils::zeros();
    vec3 originScale = VectorUtils::ones();
    vec3 localPosition = VectorUtils::zeros();
    vec3 localRotation = VectorUtils::zeros();
    vec3 localScale = VectorUtils::ones();

    vec3 worldPosition = VectorUtils::zeros();
    vec3 worldRotation = VectorUtils::zeros();
    vec3 worldScale = VectorUtils::ones();

    glm::mat4 localMatrix = glm::mat4(1.0);
    glm::mat4 worldMatrix = glm::mat4(1.0);

    std::shared_ptr<Assets::Model> modelRef;

    GameObject* parent = nullptr;
    std::vector<GameObjectPtr> children;

    std::shared_ptr<BoundingBox> obb;

    virtual void performModelTransform();
    virtual void performModelRotate();
    virtual void performModelScale();

    void updateSceneView();

    friend class ModelManager;
};


