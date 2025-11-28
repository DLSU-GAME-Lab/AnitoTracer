#include "GameObjectFactory.hpp"
#include "ModelLibrary.hpp"

using namespace Assets;

GameObjectFactory* GameObjectFactory::sharedInstance = nullptr;

GameObjectFactory* GameObjectFactory::getInstance()
{
    return sharedInstance;
}

void GameObjectFactory::initialize()
{
    if (!sharedInstance)
    {
        sharedInstance = new GameObjectFactory();
    }
}

void GameObjectFactory::destroy()
{
    delete sharedInstance;
    sharedInstance = nullptr;
}

GameObjectFactory::GameObjectFactory()
{
}

GameObject::GameObjectPtr GameObjectFactory::CreateEmpty(const String& name)
{
    return std::make_unique<GameObject>(name, GameObject::NONE);
}

GameObject::GameObjectPtr GameObjectFactory::CreateFromModelFile(const String& filepath, const String& name)
{
    auto modelPtr = ModelLibrary::getInstance()->LoadModel(filepath);
    String finalName = name.empty() ? (modelPtr ? modelPtr->GetName() : "mesh") : name;
    return std::make_unique<GameObject>(finalName, GameObject::MESH, modelPtr);
}

GameObject::GameObjectPtr GameObjectFactory::CreateCube(const String& name)
{
    auto modelPtr = ModelLibrary::getInstance()->GetModel("CUBE");
    return std::make_unique<GameObject>(name, GameObject::CUBE, modelPtr);
}

GameObject::GameObjectPtr GameObjectFactory::CreatePlane(const String& name)
{
    auto modelPtr = ModelLibrary::getInstance()->GetModel("PLANE");
    return std::make_unique<GameObject>(name, GameObject::PLANE, modelPtr);
}

GameObject::GameObjectPtr GameObjectFactory::CreateSphere(const String& name)
{
    auto modelPtr = ModelLibrary::getInstance()->GetModel("SPHERE");
    return std::make_unique<GameObject>(name, GameObject::SPHERE, modelPtr);
}

GameObject::GameObjectPtr GameObjectFactory::CreateCylinder(const String& name)
{
    auto modelPtr = ModelLibrary::getInstance()->GetModel("CYLINDER");
    return std::make_unique<GameObject>(name, GameObject::CYLINDER, modelPtr);
}

GameObject::GameObjectPtr GameObjectFactory::CreateCapsule(const String& name)
{
    auto modelPtr = ModelLibrary::getInstance()->GetModel("CAPSULE");
    return std::make_unique<GameObject>(name, GameObject::CAPSULE, modelPtr);
}

GameObject::GameObjectPtr GameObjectFactory::CreateCornellBox(const String& name)
{
    auto modelPtr = ModelLibrary::getInstance()->GetModel("CORNELL_BOX");
    return std::make_unique<GameObject>(name, GameObject::CORNELL_BOX, modelPtr);
}

GameObject::GameObjectPtr GameObjectFactory::CreatePrimitive(GameObject::PrimitiveType type, const String& name)
{
    switch (type)
    {
    case GameObject::CUBE:
        return CreateCube();
    case GameObject::PLANE:
    case GameObject::QUAD:
        return CreatePlane();
    case GameObject::SPHERE:
        return CreateSphere();
    case GameObject::CYLINDER:
        return CreateCylinder();
    case GameObject::CAPSULE:
        return CreateCapsule();
    case GameObject::CORNELL_BOX:
        return CreateCornellBox();
    case GameObject::MESH:
    case GameObject::NONE:
    default:
        return CreateEmpty(name);
    }
}

GameObjectFactory::LightPtr GameObjectFactory::CreateLight(Light::LightType type, const String& name)
{
    auto light = std::make_unique<Light>(name, type);
    return std::move(light);
}
