#include "GameObjectFactory.hpp"
#include "ModelLibrary.hpp"

using namespace Assets;

GameObject::GameObjectPtr GameObjectFactory::CreateEmpty(const String& name)
{
    auto obj = std::make_unique<GameObject>(name, GameObject::NONE);
    obj->SetID(AcquireId());
    return obj;
}

GameObject::GameObjectPtr GameObjectFactory::CreateFromModelFile(const String& filepath, const String& name)
{
    auto results = ModelLibrary::getInstance()->LoadModel(filepath);

    // No meshes loaded - create empty GameObject with nullptr model
    if (results.modelsData.empty())
    {
        String finalName = name.empty() ? "mesh" : name;
		auto obj = std::make_unique<GameObject>(finalName, GameObject::MESH, nullptr);
        obj->SetID(AcquireId());
        return obj;
    }

    // Single mesh - create single GameObject
    else if (results.modelsData.size() == 1)
    {
        String finalName = name.empty() ? results.modelsData[0]->GetFilePath() : name;
        auto gameObject = std::make_unique<GameObject>(finalName, GameObject::MESH, results.modelsData[0]);
        gameObject->SetLocalPosition(results.originalPositions[0]);
        gameObject->SetID(AcquireId());
        return gameObject;
    }

    else
    {
        // Multiple meshes - create parent with children
        String parentName = name.empty() ? "mesh" : name;

        auto parent = std::make_unique<GameObject>(parentName, GameObject::NONE);
        parent->SetID(AcquireId());
        int childCounter = 0;

        for (const auto& model : results.modelsData)
        {
            String childName = parentName + "_" + std::to_string(childCounter);
            auto child = std::make_unique<GameObject>(childName, GameObject::MESH, model);
            auto reference = child.get();
            parent->AddChild(std::move(child));
            reference->SetLocalPosition(results.originalPositions[childCounter]);
            reference->SetID(AcquireId());
            childCounter++;
        }

        return parent;
    }
    
}

GameObject::GameObjectPtr GameObjectFactory::CreateCube(const String& name)
{
    auto modelResult = ModelLibrary::getInstance()->GetModel("CUBE");
    auto obj = std::make_unique<GameObject>(name, GameObject::CUBE, modelResult.modelsData[0]);
    obj->SetID(AcquireId());
    return obj;
}

GameObject::GameObjectPtr GameObjectFactory::CreatePlane(const String& name)
{
    auto modelResult = ModelLibrary::getInstance()->GetModel("PLANE");
    auto obj = std::make_unique<GameObject>(name, GameObject::CUBE, modelResult.modelsData[0]);
    obj->SetID(AcquireId());
    return obj;
}

GameObject::GameObjectPtr GameObjectFactory::CreateSphere(const String& name)
{
    auto modelResult = ModelLibrary::getInstance()->GetModel("SPHERE");
    auto obj = std::make_unique<GameObject>(name, GameObject::CUBE, modelResult.modelsData[0]);
    obj->SetID(AcquireId());
    return obj;
}

GameObject::GameObjectPtr GameObjectFactory::CreateCylinder(const String& name)
{
    auto modelResult = ModelLibrary::getInstance()->GetModel("CYLINDER");
    auto obj = std::make_unique<GameObject>(name, GameObject::CUBE, modelResult.modelsData[0]);
    obj->SetID(AcquireId());
    return obj;
}

GameObject::GameObjectPtr GameObjectFactory::CreateCapsule(const String& name)
{
    auto modelResult = ModelLibrary::getInstance()->GetModel("CAPSULE");
    auto obj = std::make_unique<GameObject>(name, GameObject::CUBE, modelResult.modelsData[0]);
    obj->SetID(AcquireId());
    return obj;
}

GameObject::GameObjectPtr GameObjectFactory::CreateCornellBox(const String& name)
{
	auto modelResult = ModelLibrary::getInstance()->GetModel("CORNELL_BOX");
	auto obj = std::make_unique<GameObject>(name, GameObject::CUBE, modelResult.modelsData[0]);
	obj->SetID(AcquireId());
	return obj;
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

GameObject::GameObjectPtr GameObjectFactory::CreateGameObjectCopy(GameObject* original)
{
    if (!original) return nullptr;
    auto obj = original->Clone();
    obj->SetID(AcquireId());
    return obj;
}

GameObjectFactory::LightPtr GameObjectFactory::CreateLight(Light::LightType type, const String& name)
{
    auto light = std::make_unique<Light>(name, type);
	light->SetID(AcquireId());
    return std::move(light);
}

uint32_t GameObjectFactory::AcquireId()
{
    if (!freeIds.empty())
    {
        uint32_t id = freeIds.back();
        freeIds.pop_back();
        return id;
    }
    return nextId++;
}

void GameObjectFactory::ReleaseId(uint32_t id)
{
    freeIds.push_back(id);
}
