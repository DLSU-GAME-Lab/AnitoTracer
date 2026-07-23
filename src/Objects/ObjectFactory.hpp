#pragma once

#include <string>
#include <memory>
#include "HierarchyObject.hpp"

#include "ObjectFactory.hpp"
#include "HierarchyManager.hpp"
#include "Components/Transform.hpp"
#include "Components/Camera.hpp"
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

    HierarchyObject* CreateRootObject(const std::string& name);
    HierarchyObject* CreateRootObjectWithTransform(const std::string& name);
    HierarchyObject* CreateRootCameraObject(const std::string& name);
    HierarchyObject* CreateModelObject(const std::string& name, const std::string& filepath);

    HierarchyObject* CreateDirectionalLightObject(const std::string& name);
    HierarchyObject* CreatePointLightObject(const std::string& name);

    HierarchyObject* CreateSpherePrimitive(const std::string& name);
    HierarchyObject* CreateCubePrimitive(const std::string& name);

private:
    ObjectFactory() = default;
    ~ObjectFactory() = default;
};