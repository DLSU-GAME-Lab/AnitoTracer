#pragma once

#include <string>
#include <memory>
#include "HierarchyObject.hpp"

#include "ObjectFactory.hpp"
#include "HierarchyManager.hpp"
#include "Components/Transform.hpp"
#include "Components/Camera.hpp"
#include "Components/EditorCamera.hpp"
#include "Components/GameCamera.hpp"
#include "Components/ModelComponent.hpp"
#include "Components/Lights/DirectionLight.hpp"
#include "Components/Lights/PointLight.hpp"

class ObjectFactory {
public:
    static ObjectFactory& GetInstance() {
        static ObjectFactory instance;
        return instance;
    }

    ObjectFactory(const ObjectFactory&) = delete;
    ObjectFactory& operator=(const ObjectFactory&) = delete;
    ObjectFactory(ObjectFactory&&) = delete;
    ObjectFactory& operator=(ObjectFactory&&) = delete;

    HierarchyObject::Ref CreateRootObject(const std::string& name);
    HierarchyObject::Ref CreateRootObjectWithTransform(const std::string& name);
    HierarchyObject::Ref CreateRootCameraObject(const std::string& name);
    HierarchyObject::Ref CreateModelObject(const std::string& name, const std::string& filepath);

    HierarchyObject::Ref CreateDirectionalLightObject(const std::string& name);
    HierarchyObject::Ref CreatePointLightObject(const std::string& name);

    HierarchyObject::Ref CreateSpherePrimitive(const std::string& name);
    HierarchyObject::Ref CreateCubePrimitive(const std::string& name);

private:
    ObjectFactory() = default;
    ~ObjectFactory() = default;
};