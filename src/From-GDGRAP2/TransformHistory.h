#pragma once

#include "GameObject.h"

struct TransformState {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
};

struct TransformAction {
    GameObject* object;
    TransformState before;
    TransformState after;
};

class TransformHistory {
public:
    static TransformHistory& getInstance();

    void recordChange(GameObject* obj, const TransformState& before, const TransformState& after);

    bool undo();
    bool redo();

private:
    std::vector<TransformAction> undoStack;
    std::vector<TransformAction> redoStack;
    static constexpr size_t MaxSteps = 15;

    void applyState(GameObject* obj, const TransformState& state);
};
