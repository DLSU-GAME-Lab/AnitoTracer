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

    static GameObjectFactory* getInstance();
    static void initialize();
    static void destroy();

    // Create an empty game object (no model)
    GameObjectPtr CreateEmpty(const String& name = "GameObject");
    GameObjectPtr CreateFromModelFile(const String& filepath, const String& name = "");

    GameObjectPtr CreateCube(const String& name = "Cube");
    GameObjectPtr CreatePlane(const String& name = "Plane");
    GameObjectPtr CreateSphere(const String& name = "Sphere");
    GameObjectPtr CreateCylinder(const String& name = "Cylinder");
    GameObjectPtr CreateCapsule(const String& name = "Capsule");
    GameObjectPtr CreateCornellBox(const String& name = "Cornell_Box");

    // Convenience: create based on GameObject::PrimitiveType
    GameObjectPtr CreatePrimitive(GameObject::PrimitiveType type, const String& name = "Primitive");

    LightPtr CreateLight(Light::LightType type, const String& name = "Light_Source");

private:
    GameObjectFactory();
    ~GameObjectFactory() = default;
    GameObjectFactory(GameObjectFactory const&) {};             // copy constructor is private
    GameObjectFactory& operator=(GameObjectFactory const&) {};  // assignment operator is private*/

    static GameObjectFactory* sharedInstance;
};