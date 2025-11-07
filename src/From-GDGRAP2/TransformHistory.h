#pragma once

#include "GameObject.h"

struct TransformState {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    bool operator==(const TransformState& other) const
    {
        return position == other.position && rotation == other.rotation && scale == other.scale;
    }
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

    static bool isDifferent(const TransformState& a, const TransformState& b);

    bool isUndoOrRedoInProgress() const { return undoOrRedoInProgress; }
    bool isUndoOrRedoFinished() const { return undoOrRedoJustFinished; }
    void resetUndoRedoFlag();
private:
    std::vector<TransformAction> undoStack;
    std::vector<TransformAction> redoStack;
    static constexpr size_t MaxSteps = 15;

    void applyState(GameObject* obj, const TransformState& state);

    bool suppressRecording = false;

    bool undoOrRedoInProgress = false;
    bool undoOrRedoJustFinished = true;
};
