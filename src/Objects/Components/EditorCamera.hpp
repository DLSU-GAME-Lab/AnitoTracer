#pragma once

#include <glm/glm.hpp>
#include "Camera.hpp"
#include "Types/OnGUI_Editor.hpp"

class EditorCamera : public CameraComponent, public gbe::IInstanceManager<EditorCamera>, public gbe::ITrigger<OnGUI_Editor> {
public:
    EditorCamera(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~EditorCamera() override = default;

    EditorCamera(const EditorCamera&) = delete;
    EditorCamera& operator=(const EditorCamera&) = delete;

    EditorCamera(EditorCamera&&) = default;
    EditorCamera& operator=(EditorCamera&&) = default;

    void OnGUI_EditorEvent(float deltaTime) override;

    void FocusOn(const glm::vec3& position);

    void SetPivotPosition(const glm::vec3& pivot) { m_pivotPosition = pivot; }
    const glm::vec3& GetPivotPosition() const { return m_pivotPosition; }

private:
    glm::vec3 m_pivotPosition{ 0.0f, 0.0f, 0.0f };
    float m_distance = 5.0f;
    static constexpr float kDefaultDistance = 5.0f;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(EditorCamera, CameraComponent);
};

GBE_REGISTER_SERIALIZED_TYPE(EditorCamera, ComponentBase);