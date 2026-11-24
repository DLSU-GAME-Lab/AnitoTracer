#include "ScriptApiMetadata.hpp"

#include "Components/Audio/AudioSourceComponent.hpp"
#include "Components/Camera/Camera.hpp"
#include "Components/Physics/Collider.hpp"
#include "Components/Physics/RigidBody.hpp"
#include "Components/Transform.hpp"
#include "Components/Animation/AnimationComponent.hpp"

namespace anito::script {

const TypeRegistry& RegisteredTypes() {
    return GetTypeRegistry();
}

} // namespace anito::script
