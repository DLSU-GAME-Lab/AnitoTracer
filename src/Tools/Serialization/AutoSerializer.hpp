#pragma once

#include <functional>
#include <string>
#include <utility>

#include "SerializedData.hpp"
#include "File/Parser.hpp"

namespace gbe {

    class ISerializable;

    struct IAutoSerializer {

        std::string m_id;
        std::string m_display_name;

        virtual void Serialize(SerializedData& data) = 0;
        virtual void Deserialize(SerializedData& data) = 0;

        void RemoveFromInspector(ISerializable* _owner);

        virtual ~IAutoSerializer() = default;
    };

    template <typename T>
    class AutoSerializer : public IAutoSerializer {
    public:
        using InitCallback = std::function<void(T&)>;

    private:
        T& m_target;
        ISerializable* m_owner;
        InitCallback m_on_init;

    public:
        AutoSerializer(ISerializable* owner,
            std::string id,
            std::string display_name,
            T& target,
            InitCallback on_init = nullptr)
            : m_owner(owner), m_target(target), m_on_init(std::move(on_init))
        {
            m_id = std::move(id);
            m_display_name = std::move(display_name);

            if (m_owner) {
                m_owner->RegisterProperty(this);
            }
        }

        // Strictly delete copy operations on the serializer object itself
        AutoSerializer(const AutoSerializer&) = delete;
        AutoSerializer& operator=(const AutoSerializer&) = delete;

        ~AutoSerializer() override {
            if (m_owner) {
                RemoveFromInspector(m_owner);
            }
        }

        T& Get() { return m_target; }
        const T& Get() const { return m_target; }

        // Fallback using your Glaze Parser for any GLM or complex type
        inline void Serialize(SerializedData& data) override {
            data.serialized_variables.insert_or_assign(m_id, Parser::ExportClassStr(m_target));
        }

        inline void Deserialize(SerializedData& data) override {
            auto it = data.serialized_variables.find(m_id);
            if (it != data.serialized_variables.end()) {
                Parser::PopulateClassStr(m_target, it->second);
            }

            if (m_on_init) {
                m_on_init(m_target);
            }
        }
    };

    // --- Explicit Specialization Declarations ---
    template<> void AutoSerializer<float>::Serialize(SerializedData& data);
    template<> void AutoSerializer<float>::Deserialize(SerializedData& data);
    template<> void AutoSerializer<int>::Serialize(SerializedData& data);
    template<> void AutoSerializer<int>::Deserialize(SerializedData& data);
    template<> void AutoSerializer<bool>::Serialize(SerializedData& data);
    template<> void AutoSerializer<bool>::Deserialize(SerializedData& data);
    template<> void AutoSerializer<std::string>::Serialize(SerializedData& data);
    template<> void AutoSerializer<std::string>::Deserialize(SerializedData& data);

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