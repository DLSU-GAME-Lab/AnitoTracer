#include "Bindings/Fields.hpp"
#include "Common.hpp"

#include "Components/Transform.hpp"
#include "HierarchyFeatures/PrefabRef.hpp"

namespace ScriptHost::Bindings::field {
using detail::GetId;

anito::ObjectId ResolveTransform(void* storage) {
    const auto& ref = *static_cast<gbe::ObjectRef<Transform>*>(storage);
    Transform* transform = ref.Get();
    return transform ? GetId(transform->GetOwner().GetPtr()) : 0;
}

const char* ResolvePrefab(void* storage) {
    thread_local std::string path;
    path = static_cast<PrefabRef*>(storage)->Resolve().string();
    return path.c_str();
}

anito::FieldAPI GetAPI() {
    return {ResolveTransform, ResolvePrefab};
}
} // namespace ScriptHost::Bindings::field
