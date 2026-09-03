#pragma once
#include "ObjectRef.hpp"
#include <string>

#include "MethodRegistry.hpp"

namespace gbe {

    struct UnityEvent {
        gbe::ObjectRef<ISerializable> targetObject;
        std::string targetMethodName;

        void Invoke() {
            ISerializable* obj = targetObject.Get();
            if (obj && !targetMethodName.empty()) {
                MethodRegistry::GetInstance().Invoke(obj, targetMethodName);
            }
        }
    };

    // --- Explicit Specialization for Serialization ---
    template<>
    inline void AutoSerializer<UnityEvent>::Serialize(SerializedData& data) {
        // We serialize both the GUID and the Method Name using suffixes
        data.serialized_variables.insert_or_assign(m_id + "_guid", this->Get().targetObject.GetTargetGUID().ToString());
        data.serialized_variables.insert_or_assign(m_id + "_method", this->Get().targetMethodName);
    }

    template<>
    inline void AutoSerializer<UnityEvent>::Deserialize(SerializedData& data) {
        if (data.serialized_variables.find(m_id + "_guid") != data.serialized_variables.end()) {
            this->Get().targetObject.SetGUID(GUID::FromString(data.serialized_variables[m_id + "_guid"]));
        }
        if (data.serialized_variables.find(m_id + "_method") != data.serialized_variables.end()) {
            this->Get().targetMethodName = data.serialized_variables[m_id + "_method"];
        }

        if (m_on_init) {
            m_on_init(this->Get());
        }
    }
}