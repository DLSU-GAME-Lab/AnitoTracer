#include "TransformHistory.h"
#include <iostream>

TransformHistory& TransformHistory::getInstance() 
{
    static TransformHistory instance;
    return instance;
}

void TransformHistory::recordChange(GameObject* obj, const TransformState& before, const TransformState& after) 
{
    if (!obj || suppressRecording) return;

    if (undoStack.size() >= MaxSteps)
        undoStack.erase(undoStack.begin()); 

    undoStack.push_back({ obj, before, after });

    if (!suppressRecording && !undoOrRedoInProgress)
        redoStack.clear();
}

bool TransformHistory::undo()
{
    if (undoStack.empty())
        return false;

    undoOrRedoInProgress = true;

    TransformAction action = undoStack.back();
    undoStack.pop_back();

    if (action.object) {
        redoStack.push_back({ action.object, action.after, action.before });
        applyState(action.object, action.before);
    }

    undoOrRedoInProgress = false;
    return true;
}

bool TransformHistory::redo()
{
    if (redoStack.empty())
        return false;

    undoOrRedoInProgress = true;

    TransformAction action = redoStack.back();
    redoStack.pop_back();

    if (action.object) {
        undoStack.push_back({ action.object, action.before, action.after });
        applyState(action.object, action.after);
    }

    undoOrRedoInProgress = false;
    return true;
}


void TransformHistory::applyState(GameObject* obj, const TransformState& state)
{
    if (!obj) return;

    suppressRecording = true;
    obj->setLocalPosition(state.position);
    obj->setLocalRotation(state.rotation);
    obj->setLocalScale(state.scale);
    suppressRecording = false;
}

bool TransformHistory::isDifferent(const TransformState& a, const TransformState& b)
{
    return a.position != b.position || a.rotation != b.rotation || a.scale != b.scale;
}

