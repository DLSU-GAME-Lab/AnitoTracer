#pragma once

#include <memory>
#include <string>
#include <glm/vec3.hpp>

#include "From-GDGRAP2/GameObject.h"
#include "Engine/LightSystem/Light.h"
#include "Assets/Material.hpp"
#include "Assets/ModelLibrary.hpp"

class GameObjectFactory
{
public:
    using GameObjectPtr = GameObject::GameObjectPtr;
    using LightPtr = std::unique_ptr<Light>;
    using vec3 = glm::vec3;
    using String = std::string;

    // Create an empty game object (no model)
    static GameObjectPtr CreateEmpty(const String& name = "GameObject");
    static GameObjectPtr CreateFromModelFile(const String& filepath, const String& name = "");

    static GameObjectPtr CreateCube(const String& name = "Cube");
    static GameObjectPtr CreatePlane(const String& name = "Plane");
    static GameObjectPtr CreateSphere(const String& name = "Sphere");
    static GameObjectPtr CreateCylinder(const String& name = "Cylinder");
    static GameObjectPtr CreateCapsule(const String& name = "Capsule");
    static GameObjectPtr CreateCornellBox(const String& name = "Cornell_Box");

   // Convenience: create based on GameObject::PrimitiveType
   static GameObjectPtr CreatePrimitive(GameObject::PrimitiveType type, const String& name = "Primitive");
   static GameObjectPtr CreateGameObjectCopy(GameObject* original);

   static LightPtr CreateLight(Light::LightType type, const String& name = "Light_Source");

   static uint32_t AcquireId();
   static void ReleaseId(uint32_t id);

private:
    static inline uint32_t nextId = 1; // 0 reserved for null
    static inline std::vector<uint32_t> freeIds;
};