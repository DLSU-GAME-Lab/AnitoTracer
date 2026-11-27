#pragma once

#include "LightBase.hpp"
#include "../Transform.hpp"

// A rectangular light: a panel in the object's X and Z plane that shines along its -Y axis, on one side only.
// Its width and height are in world units; scaling the object does not change them.
class AreaLight : public LightBase {
public:
    AreaLight(gbe::IInstanceManager<HierarchyObject>::Ref owner = nullptr)
        : LightBase("AreaLight", owner) {}

    ~AreaLight() override = default;

    AreaLight(const AreaLight&) = delete;
    AreaLight& operator=(const AreaLight&) = delete;

    AreaLight(AreaLight&&) = default;
    AreaLight& operator=(AreaLight&&) = default;

    float GetWidth() const { return m_width; }
    void SetWidth(float width) { m_width = width; }
    float GetHeight() const { return m_height; }
    void SetHeight(float height) { m_height = height; }

    // How far its light reaches. It fades out smoothly toward this distance.
    float GetRange() const { return m_range; }
    void SetRange(float range) { m_range = range; }

    // Brightness in real units: the light the whole panel sends out. A bigger panel is dimmer per square meter.
    float GetLumens() const { return m_lumens; }
    void SetLumens(float lumens) { m_lumens = lumens; }

private:
    // Defaults live here, not in a constructor, so the generated serializer constructor gets them too.
    float m_width = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_width, "Width");
    float m_height = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_height, "Height");
    float m_range = 10.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_range, "Range");
    float m_lumens = 800.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_lumens, "Lumens");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(AreaLight, LightBase);
};

GBE_REGISTER_SERIALIZED_TYPE(AreaLight, ComponentBase);
