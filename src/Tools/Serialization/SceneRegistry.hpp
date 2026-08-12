#pragma once

#include <unordered_map>
#include "GUID.hpp"

namespace gbe {

    class ISerializable;

    class SceneRegistry {
    public:
        static SceneRegistry& GetInstance() {
            static SceneRegistry instance;
            return instance;
        }

        /// Returns reference to all registered objects currently in memory
        const std::unordered_map<GUID, ISerializable*>& GetRegistry() const {
            return m_registry;
        }

        void Register(const GUID& guid, ISerializable* ptr) {
            if (guid != GUID::Empty() && ptr != nullptr) {
                m_registry[guid] = ptr;
            }
        }

        void Unregister(const GUID& guid) {
            if (guid != GUID::Empty()) {
                m_registry.erase(guid);
            }
        }

        template<typename T>
        T* Resolve(const GUID& guid) const {
            if (guid == GUID::Empty()) return nullptr;

            auto it = m_registry.find(guid);
            if (it != m_registry.end()) {
                return dynamic_cast<T*>(it->second);
            }
            return nullptr;
        }

        void Clear() {
            m_registry.clear();
        }

    private:
        SceneRegistry() = default;
        ~SceneRegistry() = default;

        std::unordered_map<GUID, ISerializable*> m_registry;
    };

} // namespace gbe