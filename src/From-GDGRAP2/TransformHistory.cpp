#include "TransformHistory.h"

TransformHistory& TransformHistory::getInstance() 
{
    static TransformHistory instance;
    return instance;
}

void TransformHistory::recordChange(GameObject* obj, const TransformState& before, const TransformState& after) 
{
    if (!obj) return;

    if (undoStack.size() >= MaxSteps)
        undoStack.erase(undoStack.begin()); 

    undoStack.push_back({ obj, before, after });
    redoStack.clear(); 
}

bool TransformHistory::undo() 
{
    if (undoStack.empty())
        return false;

    TransformAction action = undoStack.back();
    undoStack.pop_back();

    if (action.object) {
        redoStack.push_back({ action.object, action.after, action.before });
        applyState(action.object, action.before);
    }

    return true;
}

bool TransformHistory::redo() 
{
    if (redoStack.empty())
        return false;

    TransformAction action = redoStack.back();
    redoStack.pop_back();

    if (action.object) {
        undoStack.push_back({ action.object, action.before, action.after });
        applyState(action.object, action.after);
    }

    return true;
}

void TransformHistory::applyState(GameObject* obj, const TransformState& state)
{
    if (!obj) return;

    obj->setLocalPosition(state.position);
    obj->setLocalRotation(state.rotation);
    obj->setLocalScale(state.scale);
}
