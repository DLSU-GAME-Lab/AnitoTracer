#pragma once

#include <unordered_map>
#include <string>
#include <functional>
#include <typeindex>
#include <vector>

#include "ISerializable.hpp" // Assuming this is where ISerializable is defined

namespace gbe {

    class MethodRegistry {
    public:
        // Type-erased function wrapper that takes the base ISerializable*
        using InvokerFunc = std::function<void(ISerializable*)>;

        static MethodRegistry& GetInstance() {
            static MethodRegistry instance;
            return instance;
        }

        // Register a method associated with a specific class type
        void Register(std::type_index type, const std::string& methodName, InvokerFunc func) {
            m_Registry[type][methodName] = std::move(func);
            m_MethodNames[type].push_back(methodName); // Keep a flat list for ImGui dropdowns
        }

        // Invoke the method by string name safely
        void Invoke(ISerializable* target, const std::string& methodName) {
            if (!target) return;

            auto type = std::type_index(typeid(*target));
            auto it = m_Registry.find(type);
            if (it != m_Registry.end() && it->second.count(methodName)) {
                it->second[methodName](target);
            }
        }

        // Retrieve available methods for the Inspector dropdown
        const std::vector<std::string>& GetAvailableMethods(std::type_index type) {
            static std::vector<std::string> empty;
            auto it = m_MethodNames.find(type);
            return it != m_MethodNames.end() ? it->second : empty;
        }

    private:
        MethodRegistry() = default;
        ~MethodRegistry() = default;

        std::unordered_map<std::type_index, std::unordered_map<std::string, InvokerFunc>> m_Registry;
        std::unordered_map<std::type_index, std::vector<std::string>> m_MethodNames;
    };


#define GBE_REGISTER_METHOD(ClassName, MethodName) \
    inline static struct MethodRegisterer_##ClassName_##MethodName { \
        MethodRegisterer_##ClassName_##MethodName() { \
            gbe::MethodRegistry::GetInstance().Register( \
                std::type_index(typeid(ClassName)), \
                #MethodName, \
                [](gbe::ISerializable* instance) { \
                    if (auto* typedInst = dynamic_cast<ClassName*>(instance)) { \
                        typedInst->MethodName(); \
                    } \
                } \
            ); \
        } \
    } _method_reg_##ClassName_##MethodName;

} // namespace gbe