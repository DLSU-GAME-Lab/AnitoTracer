#pragma once

#include <functional>
#include <string>
#include <utility>

#include "SerializedData.hpp"
#include "File/Parser.hpp"

#include "PropertyDrawer.hpp"
#include "IAutoSerializer.hpp"

#include <typeinfo>

namespace gbe {

    // =======================================================
    // PRIMARY TEMPLATE: Only handles default Serialization
    // =======================================================
    template <typename T>
    class AutoSerializer : public AutoSerializerBase<T> {
    public:
        // Inherit constructors from AutoSerializerBase
        using AutoSerializerBase<T>::AutoSerializerBase;

        inline void Serialize(SerializedData& data) override {
            data.serialized_variables.insert_or_assign(this->m_id, Parser::ExportClassStr(this->m_target));
        }

        inline void Deserialize(SerializedData& data) override {
            auto it = data.serialized_variables.find(this->m_id);
            if (it != data.serialized_variables.end()) {
                Parser::PopulateClassStr(this->m_target, it->second);
            }

            if (this->m_on_init) {
                this->m_on_init(this->m_target);
            }
        }
    };

    // --- Explicit Specialization Declarations ---
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

    // Macro with display name AND callback
#define GBE_SERIALIZE_FIELD_W_NAME_CB(var_name, display_name, callback) \
        gbe::AutoSerializer<std::decay_t<decltype(var_name)>> _auto_serializer_##var_name{this, #var_name, display_name, var_name, callback}

    // Macro with display name (no callback)
#define GBE_SERIALIZE_FIELD_W_NAME(var_name, display_name) \
        GBE_SERIALIZE_FIELD_W_NAME_CB(var_name, display_name, nullptr)

    // Macro with variable name as default display name AND callback
#define GBE_SERIALIZE_FIELD_W_CB(var_name, callback) \
        GBE_SERIALIZE_FIELD_W_NAME_CB(var_name, #var_name, callback)

    // Macro using variable name as default display name (no callback)
#define GBE_SERIALIZE_FIELD(var_name) \
        GBE_SERIALIZE_FIELD_W_NAME(var_name, #var_name)
}