#include "ContactListenerManager.h"

// Add a sub-listener to receive events
 void ContactListenerManager::AddListener(JPH::ContactListener* listener) {
    if (listener && std::find(mListeners.begin(), mListeners.end(), listener) == mListeners.end()) {
        mListeners.push_back(listener);
    }
}

// Remove a sub-listener
 void ContactListenerManager::RemoveListener(JPH::ContactListener* listener) {
    mListeners.erase(std::remove(mListeners.begin(), mListeners.end(), listener), mListeners.end());
}

// Forwarding OnContactValidate
 JPH::ValidateResult ContactListenerManager::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) {
    JPH::ValidateResult combinedResult = JPH::ValidateResult::AcceptContact;

    for (auto* listener : mListeners) {
        JPH::ValidateResult result = listener->OnContactValidate(inBody1, inBody2, inBaseOffset, inCollisionResult);
        // If any listener rejects the contact, reject it entirely
        if (result == JPH::ValidateResult::RejectContact) {
            return JPH::ValidateResult::RejectContact;
        }
        else if (result == JPH::ValidateResult::RejectAllContactsForThisBodyPair) {
            combinedResult = JPH::ValidateResult::RejectAllContactsForThisBodyPair;
        }
    }
    return combinedResult;
}

// Forwarding OnContactAdded
 void ContactListenerManager::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) {
    for (auto* listener : mListeners) {
        listener->OnContactAdded(inBody1, inBody2, inManifold, ioSettings);
    }
}

// Forwarding OnContactPersisted
 void ContactListenerManager::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) {
    for (auto* listener : mListeners) {
        listener->OnContactPersisted(inBody1, inBody2, inManifold, ioSettings);
    }
}

// Forwarding OnContactRemoved
 void ContactListenerManager::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) {
    for (auto* listener : mListeners) {
        listener->OnContactRemoved(inSubShapePair);
    }
}

