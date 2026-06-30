#pragma once

#include <memory>
#include <string>
#include <glm/vec3.hpp>

#include "From-GDGRAP2/GameObject.h"
#include "Engine/LightSystem/Light.h"
#include "Assets/Material.hpp"
#include "Assets/ModelLibrary.hpp"
#include "../Engine/Physics/TracerPhysics.h"

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
    static GameObjectPtr CreateSphere(const String& name, float radius);

    static GameObjectPtr CreateCylinder(const String& name = "Cylinder");
    static GameObjectPtr CreateCapsule(const String& name = "Capsule");
    static GameObjectPtr CreateCornellBox(const String& name = "Cornell_Box");

   // Convenience: create based on GameObject::PrimitiveType
   static GameObjectPtr CreatePrimitive(GameObject::PrimitiveType type, const String& name = "Primitive");

   static LightPtr CreateLight(Light::LightType type, const String& name = "Light_Source");

private:
    //GameObjectFactory();
    //~GameObjectFactory() = default;
    //GameObjectFactory(GameObjectFactory const&) {};             // copy constructor is private
    //GameObjectFactory& operator=(GameObjectFactory const&) {};  // assignment operator is private*/

};