#pragma once

#include "GUID.hpp"
#include "SceneRegistry.hpp"

#include "AutoSerializer.hpp"
#include "ObjectRef.hpp"
#include "File/Parser.hpp"

namespace gbe {

    template <typename T>
    class ObjectRef {
    public:
        ObjectRef() = default;

        ObjectRef(T* instance) {
            Set(instance);
        }

        ObjectRef(const GUID& guid) : m_targetGuid(guid) {}

        // Sets or updates the referenced object
        void Set(T* instance) {
            m_cachedPtr = instance;
            m_targetGuid = instance ? instance->GetGUID() : GUID::Empty();
        }

        // Sets the target GUID and invalidates cached pointer for re-resolution
        void SetGUID(const GUID& guid) {
            m_targetGuid = guid;
            m_cachedPtr = nullptr;
        }

        // Lazy-resolves target pointer via SceneRegistry
        T* Get() const {
            if (!m_cachedPtr && m_targetGuid != GUID::Empty()) {
                m_cachedPtr = SceneRegistry::GetInstance().Resolve<T>(m_targetGuid);
            }
            return m_cachedPtr;
        }

        T* operator->() const { return Get(); }
        T& operator*() const { return *Get(); }
        explicit operator bool() const { return Get() != nullptr; }

        bool operator==(const ObjectRef<T>& other) const { return GetTargetGUID() == other.GetTargetGUID(); }
        bool operator!=(const ObjectRef<T>& other) const { return !(*this == other); }

        GUID GetTargetGUID() const { return m_targetGuid; }

    private:
        GUID m_targetGuid = GUID::Empty();
        mutable T* m_cachedPtr = nullptr;
    };

} // namespace gbe


namespace gbe {

    template <typename T>
    class AutoSerializer<ObjectRef<T>> : public AutoSerializerBase<ObjectRef<T>> {
    public:
        using AutoSerializerBase<ObjectRef<T>>::AutoSerializerBase;

        void Serialize(SerializedData& data) override {
            // Serialize the target object's GUID as a string
            GUID targetGuid = this->m_target.GetTargetGUID();
            data.serialized_variables.insert_or_assign(this->m_id, Parser::ExportClassStr(targetGuid));
        }

        void Deserialize(SerializedData& data) override {
            auto it = data.serialized_variables.find(this->m_id);
            if (it != data.serialized_variables.end()) {
                GUID restoredGuid = GUID::Empty();
                Parser::PopulateClassStr(restoredGuid, it->second);
                this->m_target.SetGUID(restoredGuid);
            }

            if (this->m_on_init) {
                this->m_on_init(this->m_target);
            }
        }
    };

} // namespace gbe