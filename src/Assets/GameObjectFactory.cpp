#include "GameObjectFactory.hpp"
#include "ModelLibrary.hpp"

using namespace Assets;

GameObject::GameObjectPtr GameObjectFactory::CreateEmpty(const String& name)
{
    return std::make_unique<GameObject>(name, GameObject::NONE);
}

GameObject::GameObjectPtr GameObjectFactory::CreateFromModelFile(const String& filepath, const String& name)
{
    auto results = ModelLibrary::getInstance()->LoadModel(filepath);

    // No meshes loaded - create empty GameObject with nullptr model
    if (results.modelsData.empty())
    {
        String finalName = name.empty() ? "mesh" : name;
        return std::make_unique<GameObject>(finalName, GameObject::MESH, nullptr);
    }

    // Single mesh - create single GameObject
    else if (results.modelsData.size() == 1)
    {
        String finalName = name.empty() ? results.modelsData[0]->GetName() : name;
        auto gameObject = std::make_unique<GameObject>(finalName, GameObject::MESH, results.modelsData[0]);
        gameObject->setLocalPosition(results.originalPositions[0]);
        return gameObject;
    }

    else
    {
        // Multiple meshes - create parent with children
        String parentName = name.empty() ? "mesh" : name;

        auto parent = std::make_unique<GameObject>(parentName, GameObject::NONE);
        int childCounter = 0;

        for (const auto& model : results.modelsData)
        {
            String childName = parentName + "_" + std::to_string(childCounter);
            auto child = std::make_unique<GameObject>(childName, GameObject::MESH, model);
            auto reference = child.get();
            parent->addChild(std::move(child));
            reference->setLocalPosition(results.originalPositions[childCounter]);
            childCounter++;
        }

        return parent;
    }
    
}

GameObject::GameObjectPtr GameObjectFactory::CreateCube(const String& name)
{
    auto modelResult = ModelLibrary::getInstance()->GetModel("CUBE");
    GameObject::GameObjectPtr ret = std::make_unique<GameObject>(name, GameObject::CUBE, modelResult.modelsData[0]);

    TracerPhysics::GetInstance().AddBox(ret.get(), false);

    return ret;
}

GameObject::GameObjectPtr GameObjectFactory::CreatePlane(const String& name)
{
    auto modelResult = ModelLibrary::getInstance()->GetModel("PLANE");
    return std::make_unique<GameObject>(name, GameObject::PLANE, modelResult.modelsData[0]);
}

GameObject::GameObjectPtr GameObjectFactory::CreateSphere(const String& name)
{
    auto modelResult = ModelLibrary::getInstance()->GetModel("SPHERE");
    GameObject::GameObjectPtr ret = std::make_unique<GameObject>(name, GameObject::SPHERE, modelResult.modelsData[0]);

    TracerPhysics::GetInstance().AddSphere(ret.get());

    return ret;
}

GameObject::GameObjectPtr GameObjectFactory::CreateSphere(const String& name, float radius)
{
    auto modelResult = ModelLibrary::getInstance()->GetModel("SPHERE");
    GameObject::GameObjectPtr ret = std::make_unique<GameObject>(name, GameObject::SPHERE, modelResult.modelsData[0]);

    ret->setLocalScale(glm::vec3(radius));

    TracerPhysics::GetInstance().AddSphere(ret.get());

    return ret;
}

GameObject::GameObjectPtr GameObjectFactory::CreateCylinder(const String& name)
{
    auto modelResult = ModelLibrary::getInstance()->GetModel("CYLINDER");
    return std::make_unique<GameObject>(name, GameObject::CYLINDER, modelResult.modelsData[0]);
}

GameObject::GameObjectPtr GameObjectFactory::CreateCapsule(const String& name)
{
    auto modelResult = ModelLibrary::getInstance()->GetModel("CAPSULE");
    return std::make_unique<GameObject>(name, GameObject::CAPSULE, modelResult.modelsData[0]);
}

GameObject::GameObjectPtr GameObjectFactory::CreateCornellBox(const String& name)
{
    auto modelResult = ModelLibrary::getInstance()->GetModel("CORNELL_BOX");
    return std::make_unique<GameObject>(name, GameObject::CORNELL_BOX, modelResult.modelsData[0]);
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
