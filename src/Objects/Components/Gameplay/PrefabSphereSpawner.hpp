#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"
#include "AssignableEvent/MethodRegistry.hpp"
#include "HierarchyFeatures/PrefabRef.hpp"

#include <vector>

// Spawns copies of a prefab at random positions inside (or on) a sphere centered on this object.
class PrefabSphereSpawner : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    PrefabSphereSpawner(gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~PrefabSphereSpawner() override = default;

    PrefabSphereSpawner(const PrefabSphereSpawner&) = delete;
    PrefabSphereSpawner& operator=(const PrefabSphereSpawner&) = delete;

    PrefabSphereSpawner(PrefabSphereSpawner&&) = default;
    PrefabSphereSpawner& operator=(PrefabSphereSpawner&&) = default;

    void OnUpdate(float deltaTime) override {}
    void OnStart() override;

    // Exposed as a UnityFunction; spawns the configured batch.
    void Spawn();

private:
    PrefabRef m_prefab;
    GBE_SERIALIZE_FIELD_W_NAME(m_prefab, "Prefab");

    int m_count = 20;
    GBE_SERIALIZE_FIELD_W_NAME(m_count, "Count");

    float m_radius = 20.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_radius, "Radius");

    bool m_surfaceOnly = false;
    GBE_SERIALIZE_FIELD_W_NAME(m_surfaceOnly, "Surface Only");

    bool m_randomRotation = true;
    GBE_SERIALIZE_FIELD_W_NAME(m_randomRotation, "Random Rotation");

    float m_minScale = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_minScale, "Min Scale");

    float m_maxScale = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_maxScale, "Max Scale");

    bool m_spawnOnStart = true;
    GBE_SERIALIZE_FIELD_W_NAME(m_spawnOnStart, "Spawn On Start");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(PrefabSphereSpawner, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(PrefabSphereSpawner, ComponentBase);

GBE_REGISTER_METHOD(PrefabSphereSpawner, Spawn);
