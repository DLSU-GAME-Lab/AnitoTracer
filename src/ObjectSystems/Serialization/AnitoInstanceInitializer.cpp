#include "AnitoInstanceInitializer.hpp"

#include "HierarchyManager.hpp"

HierarchyObject* AnitoInstanceInitializer::InitializeImpl(std::unique_ptr<HierarchyObject> newobj)
{
    // Instantiate the Transform component using the correct default constructor.
    // The constructor defaults to a nullptr owner and sets the name internally.
    auto transform = std::make_unique<Transform>();

    // Finally, attach the component to the newly created object.
    HierarchyManager::GetInstance().AddComponentToObject(newobj.get(), std::move(transform));

    return HierarchyManager::GetInstance().AddRootObject(std::move(newobj));
}