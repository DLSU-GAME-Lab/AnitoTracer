#include "ScriptApiMetadata.hpp"

#include "Components/Audio/AudioSourceComponent.hpp"
#include "Components/Camera.hpp"
#include "Components/Physics/Collider.hpp"
#include "Components/Physics/RigidBody.hpp"
#include "Components/Transform.hpp"
#include "sdk/anito/api/AudioSource.hpp"
#include "sdk/anito/api/Camera.hpp"
#include "sdk/anito/api/Collider.hpp"
#include "sdk/anito/api/RaycastHit.hpp"
#include "sdk/anito/api/Rigidbody.hpp"
#include "sdk/anito/api/Transform.hpp"

ANITO_SCRIPT_TYPE_MAPPING(anito::Vec3, "vec3")
ANITO_SCRIPT_TYPE_MAPPING(anito::Quat, "quat")
ANITO_SCRIPT_TYPE_MAPPING(anito::RaycastHit, "raycasthit")

ANITO_SCRIPT_CONFORMS_VALID(anito::AudioSource);
ANITO_SCRIPT_CONFORMS_VALID(anito::Camera);
ANITO_SCRIPT_CONFORMS_VALID(anito::Collider);
ANITO_SCRIPT_CONFORMS_VALID(anito::Rigidbody);
ANITO_SCRIPT_CONFORMS_VALID(anito::Transform);

ANITO_SCRIPT_CONFORMS_READ_ONLY_PROPERTY(AudioSourceComponent, HasAudioClip, anito::AudioSource, hasClip);
ANITO_SCRIPT_CONFORMS_PROPERTY(AudioSourceComponent, GetVolume, SetVolume, anito::AudioSource, volume, setVolume);
ANITO_SCRIPT_CONFORMS_PROPERTY(AudioSourceComponent, GetLoop, SetLoop, anito::AudioSource, loop, setLoop);
ANITO_SCRIPT_CONFORMS_PROPERTY(AudioSourceComponent, GetPitch, SetPitch, anito::AudioSource, pitch, setPitch);
ANITO_SCRIPT_CONFORMS_PROPERTY(AudioSourceComponent, GetSpatialized, SetSpatialized, anito::AudioSource, spatialized,
                               setSpatialized);
ANITO_SCRIPT_CONFORMS_METHOD(AudioSourceComponent, Play, anito::AudioSource, play);
ANITO_SCRIPT_CONFORMS_METHOD(AudioSourceComponent, Stop, anito::AudioSource, stop);
ANITO_SCRIPT_CONFORMS_METHOD(AudioSourceComponent, PlayDetached, anito::AudioSource, playDetached);
ANITO_SCRIPT_CONFORMS_METHOD(AudioSourceComponent, SetDistance, anito::AudioSource, setDistance);

ANITO_SCRIPT_CONFORMS_PROPERTY(Transform, GetPosition, SetWorldPosition, anito::Transform, position, setPosition);
ANITO_SCRIPT_CONFORMS_PROPERTY(Transform, GetLocalPosition, SetPosition, anito::Transform, localPosition,
                               setLocalPosition);
ANITO_SCRIPT_CONFORMS_PROPERTY(Transform, GetRotation, SetWorldRotation, anito::Transform, rotation, setRotation);
ANITO_SCRIPT_CONFORMS_PROPERTY(Transform, GetLocalRotation, SetRotation, anito::Transform, localRotation,
                               setLocalRotation);
ANITO_SCRIPT_CONFORMS_PROPERTY(Transform, GetLocalScale, SetScale, anito::Transform, localScale, setLocalScale);
ANITO_SCRIPT_CONFORMS_READ_ONLY_PROPERTY(Transform, GetScale, anito::Transform, lossyScale);
ANITO_SCRIPT_CONFORMS_PROPERTY(Transform, GetEulerAnglesDegrees, SetEulerAnglesDegrees, anito::Transform,
                               eulerAngles, setEulerAngles);

ANITO_SCRIPT_CONFORMS_PROPERTY(RigidBody, GetVelocity, SetVelocity, anito::Rigidbody, velocity, setVelocity);
ANITO_SCRIPT_CONFORMS_PROPERTY(RigidBody, GetAngularVelocity, SetAngularVelocity, anito::Rigidbody, angularVelocity,
                               setAngularVelocity);
ANITO_SCRIPT_CONFORMS_PROPERTY(RigidBody, GetMass, SetMass, anito::Rigidbody, mass, setMass);
ANITO_SCRIPT_CONFORMS_METHOD(RigidBody, ApplyForce, anito::Rigidbody, addForce);
ANITO_SCRIPT_CONFORMS_METHOD(RigidBody, ApplyTorque, anito::Rigidbody, addTorque);
ANITO_SCRIPT_CONFORMS_METHOD(RigidBody, ApplyImpulse, anito::Rigidbody, addImpulse);
ANITO_SCRIPT_CONFORMS_METHOD(RigidBody, Teleport, anito::Rigidbody, teleport);
ANITO_SCRIPT_CONFORMS_PROPERTY(RigidBody, GetWorldGravity, SetWorldGravity, anito::Rigidbody, worldGravity,
                               setWorldGravity);
ANITO_SCRIPT_CONFORMS_METHOD(RigidBody, Raycast, anito::Rigidbody, raycast);

ANITO_SCRIPT_CONFORMS_PROPERTY(CameraComponent, GetFOV, SetFOV, anito::Camera, fov, setFov);
ANITO_SCRIPT_CONFORMS_PROPERTY(CameraComponent, GetAspect, SetAspect, anito::Camera, aspect, setAspect);
ANITO_SCRIPT_CONFORMS_PROPERTY(CameraComponent, GetNearPlane, SetNearPlane, anito::Camera, nearPlane, setNearPlane);
ANITO_SCRIPT_CONFORMS_PROPERTY(CameraComponent, GetFarPlane, SetFarPlane, anito::Camera, farPlane, setFarPlane);

namespace anito::script {

const TypeRegistry& RegisteredTypes() {
    return GetTypeRegistry();
}

} // namespace anito::script
