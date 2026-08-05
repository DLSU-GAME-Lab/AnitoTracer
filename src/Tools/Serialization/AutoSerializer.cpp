#include "AutoSerializer.hpp"
#include "ISerializable.hpp"

namespace gbe {

    void IAutoSerializer::RemoveFromInspector(ISerializable* owner) {
        owner->UnRegisterProperty(this);
    }

    template class AutoSerializer<float>;
    template class AutoSerializer<int>;
    template class AutoSerializer<bool>;
    template class AutoSerializer<std::string>;

    // INT
    template<>
    void AutoSerializer<int>::Serialize(SerializedData& data) {
        data.serialized_variables.insert_or_assign(m_id, std::to_string(m_target));
    }

    template<>
    void AutoSerializer<int>::Deserialize(SerializedData& data) {
        if (data.serialized_variables.find(m_id) != data.serialized_variables.end()) {
            m_target = std::stoi(data.serialized_variables[m_id]);
        }
        if (m_on_init) {
            m_on_init(m_target);
        }
    }

    // BOOL
    template<>
    void AutoSerializer<bool>::Serialize(SerializedData& data) {
        data.serialized_variables.insert_or_assign(m_id, m_target ? "1" : "0");
    }

    template<>
    void AutoSerializer<bool>::Deserialize(SerializedData& data) {
        if (data.serialized_variables.find(m_id) != data.serialized_variables.end()) {
            std::string val = data.serialized_variables[m_id];
            m_target = (val == "1" || val == "true");
        }
        if (m_on_init) {
            m_on_init(m_target);
        }
    }

    // FLOAT
    template<>
    void AutoSerializer<float>::Serialize(SerializedData& data) {
        data.serialized_variables.insert_or_assign(m_id, std::to_string(m_target));
    }

    template<>
    void AutoSerializer<float>::Deserialize(SerializedData& data) {
        if (data.serialized_variables.find(m_id) != data.serialized_variables.end()) {
            m_target = std::stof(data.serialized_variables[m_id]);
        }
        if (m_on_init) {
            m_on_init(m_target);
        }
    }

    // STRING
    template<>
    void AutoSerializer<std::string>::Serialize(SerializedData& data) {
        data.serialized_variables.insert_or_assign(m_id, m_target);
    }

    template<>
    void AutoSerializer<std::string>::Deserialize(SerializedData& data) {
        if (data.serialized_variables.find(m_id) != data.serialized_variables.end()) {
            m_target = data.serialized_variables[m_id];
        }
        if (m_on_init) {
            m_on_init(m_target);
        }
    }

}