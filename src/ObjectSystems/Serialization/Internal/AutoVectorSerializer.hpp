#pragma once

#include "AutoSerializer.hpp"
#include "TypeRegistry.hpp"

#include <vector>
#include <memory>
#include <string>
#include <typeinfo>
#include <type_traits>

namespace gbe {

    // ==========================================
    // SPECIALIZATION: Direct Reference Vector Serializer
    // ==========================================
    template <typename T>
    class AutoSerializer<std::vector<std::unique_ptr<T>>> : public IAutoSerializer {
    private:
        std::vector<std::unique_ptr<T>>& m_target;
        ISerializable* m_owner;

    public:
        // Binds directly to the container reference — zero copies or vector reassignments
        AutoSerializer(ISerializable* owner,
            std::string id,
            std::string display_name,
            std::vector<std::unique_ptr<T>>& target)
            : m_owner(owner), m_target(target)
        {
            m_id = id;
            m_display_name = display_name;

            if (m_owner) {
                m_owner->RegisterProperty(this);
            }
        }

        // Strictly disable copy/assignment for the serializer itself
        AutoSerializer(const AutoSerializer&) = delete;
        AutoSerializer& operator=(const AutoSerializer&) = delete;

        ~AutoSerializer() override {
            if (m_owner) {
                RemoveFromInspector(m_owner);
            }
        }

        // ==========================================
        // SERIALIZE
        // ==========================================
        void Serialize(SerializedData& data) override {
            // Write element count
            data.serialized_variables.insert_or_assign(m_id + ".count", std::to_string(m_target.size()));

            for (size_t i = 0; i < m_target.size(); ++i) {
                if (!m_target[i]) continue;

                // 1. Get serialized data from child object
                SerializedData child_data = m_target[i]->Serialize();

                // 2. Embed concrete runtime type string for TypeRegistry lookup
                std::string type_name = typeid(*m_target[i]).name();
                child_data.serialized_variables.insert_or_assign("__type", type_name);

                // 3. Prefix child keys: "m_components[0].m_name"
                std::string prefix = m_id + "[" + std::to_string(i) + "].";
                for (const auto& [key, value] : child_data.serialized_variables) {
                    data.serialized_variables.insert_or_assign(prefix + key, value);
                }
            }
        }

        // ==========================================
        // DESERIALIZE
        // ==========================================
        void Deserialize(SerializedData& data) override {
            auto count_it = data.serialized_variables.find(m_id + ".count");
            if (count_it == data.serialized_variables.end()) return;

            size_t count = std::stoul(count_it->second);

            // Clear vector in-place without triggering copy operations
            m_target.clear();
            m_target.reserve(count);

            for (size_t i = 0; i < count; ++i) {
                std::string prefix = m_id + "[" + std::to_string(i) + "].";
                SerializedData child_data;

                // Extract all key-value pairs belonging to index i
                for (const auto& [key, value] : data.serialized_variables) {
                    if (key.rfind(prefix, 0) == 0) { // starts_with prefix
                        std::string child_key = key.substr(prefix.length());
                        child_data.serialized_variables.insert_or_assign(child_key, value);
                    }
                }

                // Get stored type name
                auto type_it = child_data.serialized_variables.find("__type");
                if (type_it == child_data.serialized_variables.end()) continue;

                // Instantiate instance using TypeRegistry
                ISerializable* raw_obj = TypeRegistry::Instantiate(type_it->second, child_data);
                if (raw_obj) {
                    // Load/Deserialize state directly into the newly created instance
                    raw_obj->Deserialize(child_data);

                    // Transfer ownership strictly via std::unique_ptr move
                    if constexpr (std::is_same_v<T, ISerializable>) {
                        m_target.emplace_back(raw_obj);
                    }
                    else {
                        T* typed_obj = dynamic_cast<T*>(raw_obj);
                        if (typed_obj) {
                            m_target.emplace_back(typed_obj);
                        }
                        else {
                            delete raw_obj; // Cleanup fallback if type cast fails
                        }
                    }
                }
            }
        }
    };

}