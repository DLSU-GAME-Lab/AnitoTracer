#pragma once

#include "AutoSerializer.hpp"
#include "AssetRef.hpp"

namespace gbe {

    template <typename T>
    class AutoSerializer<AssetRef<T>> : public AutoSerializerBase<AssetRef<T>> {
    public:
        // Automatically inherits constructors, destructor, Get(), and DrawInspector()!
        using AutoSerializerBase<AssetRef<T>>::AutoSerializerBase;

        inline void Serialize(SerializedData& data) override {
            data.serialized_variables.insert_or_assign(this->m_id, this->m_target.GetGUID().ToString());
        }

        inline void Deserialize(SerializedData& data) override {
            auto it = data.serialized_variables.find(this->m_id);
            if (it != data.serialized_variables.end()) {
                this->m_target.SetGUID(GUID::FromString(it->second));
            }

            if (this->m_on_init) {
                this->m_on_init(this->m_target);
            }
        }
    };

}