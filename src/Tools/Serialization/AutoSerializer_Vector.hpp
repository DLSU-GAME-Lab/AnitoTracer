#pragma once

#include "AutoSerializer.hpp"
#include "TypeRegistry.hpp"

#include <vector>
#include <memory>
#include <string>
#include <typeinfo>
#include <type_traits>

namespace gbe {

    template <typename T>
    class AutoSerializer<std::vector<std::unique_ptr<T>>> : public AutoSerializerBase<std::vector<std::unique_ptr<T>>> {
    public:
        // Automatically inherits constructors, destructor, Get(), and DrawInspector()!
        using AutoSerializerBase<std::vector<std::unique_ptr<T>>>::AutoSerializerBase;

        void Serialize(SerializedData& data) override {
            data.serialized_variables.insert_or_assign(this->m_id + ".count", std::to_string(this->m_target.size()));

            for (size_t i = 0; i < this->m_target.size(); ++i) {
                if (!this->m_target[i]) continue;

                SerializedData child_data = this->m_target[i]->Serialize();
                std::string type_name = typeid(*this->m_target[i]).name();
                child_data.serialized_variables.insert_or_assign("__type", type_name);

                std::string prefix = this->m_id + "[" + std::to_string(i) + "].";
                for (const auto& [key, value] : child_data.serialized_variables) {
                    data.serialized_variables.insert_or_assign(prefix + key, value);
                }
            }
        }

        void Deserialize(SerializedData& data) override {
            auto count_it = data.serialized_variables.find(this->m_id + ".count");
            if (count_it == data.serialized_variables.end()) return;

            size_t count = std::stoul(count_it->second);

            this->m_target.clear();
            this->m_target.reserve(count);

            for (size_t i = 0; i < count; ++i) {
                std::string prefix = this->m_id + "[" + std::to_string(i) + "].";
                SerializedData child_data;

                for (const auto& [key, value] : data.serialized_variables) {
                    if (key.rfind(prefix, 0) == 0) {
                        std::string child_key = key.substr(prefix.length());
                        child_data.serialized_variables.insert_or_assign(child_key, value);
                    }
                }

                auto type_it = child_data.serialized_variables.find("__type");
                if (type_it == child_data.serialized_variables.end()) continue;

                ISerializable* raw_obj = TypeRegistry::Instantiate(type_it->second, child_data);
                if (raw_obj) {
                    raw_obj->Deserialize(child_data);

                    if constexpr (std::is_same_v<T, ISerializable>) {
                        this->m_target.emplace_back(raw_obj);
                    }
                    else {
                        T* typed_obj = dynamic_cast<T*>(raw_obj);
                        if (typed_obj) {
                            this->m_target.emplace_back(typed_obj);
                        }
                        else {
                            delete raw_obj;
                        }
                    }
                }
            }

            if (this->m_on_init) {
                this->m_on_init(this->m_target);
            }
        }
    };

}