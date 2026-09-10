#pragma once

#include "GUID.hpp"
#include "SceneRegistry.hpp"

#include "AutoSerializer.hpp"
#include "File/Parser.hpp"

#include <cstddef>

namespace gbe {

    template <typename T>
    class ObjectRef {
    public:
        ObjectRef() = default;

        ObjectRef(std::nullptr_t) {}

        ObjectRef(T* instance) {
            Set(instance);
        }

        ObjectRef(const GUID& guid) : m_targetGuid(guid) {}

        // Sets or updates the referenced object
        void Set(T* instance) {
            m_targetGuid = instance ? instance->GetGUID() : GUID::Empty();
        }

        // Sets the target GUID for lazy resolution through the live scene registry
        void SetGUID(const GUID& guid) {
            m_targetGuid = guid;
        }

        // Resolve on every access so destroyed or replaced objects cannot leave a
        // stale cached pointer behind.
        T* Get() const {
            return SceneRegistry::GetInstance().Resolve<T>(m_targetGuid);
        }

        T* operator->() const { return Get(); }
        T& operator*() const { return *Get(); }
        explicit operator bool() const { return Get() != nullptr; }

        bool operator==(const ObjectRef<T>& other) const { return GetTargetGUID() == other.GetTargetGUID(); }
        bool operator!=(const ObjectRef<T>& other) const { return !(*this == other); }

        GUID GetTargetGUID() const { return m_targetGuid; }

    private:
        GUID m_targetGuid = GUID::Empty();
    };

} // namespace gbe


namespace gbe {

    template <typename T>
    class AutoSerializer<ObjectRef<T>> : public AutoSerializerBase<ObjectRef<T>> {
    public:
        using AutoSerializerBase<ObjectRef<T>>::AutoSerializerBase;

        void Serialize(SerializedData& data) override {
            // Serialize the target object's GUID as a string
            GUID targetGuid = this->Get().GetTargetGUID();
            data.serialized_variables.insert_or_assign(this->m_id, Parser::ExportClassStr(targetGuid));
        }

        void Deserialize(SerializedData& data) override {
            auto it = data.serialized_variables.find(this->m_id);
            if (it != data.serialized_variables.end()) {
                GUID restoredGuid = GUID::Empty();
                Parser::PopulateClassStr(restoredGuid, it->second);
                this->Get().SetGUID(restoredGuid);
            }

            if (this->m_on_init) {
                this->m_on_init(this->Get());
            }
        }
    };

} // namespace gbe