#pragma once

#include "SerializedData.hpp"
#include "ISerializable.hpp"
#include "InstanceInitializer.hpp"

#include <string>
#include <unordered_map>
#include <functional>
#include <typeinfo>
#include <vector>
#include <memory>
#include <map>

namespace gbe {
    class TypeRegistry {
        typedef std::function<ISerializable*(SerializedData&)> TypeDeserializerFunction;
        typedef std::function<ISerializable* ()> TypeCreatorFunction;

    public:
        struct RegistryEntry {
            std::string name;
            TypeDeserializerFunction factory;
        };
        struct RegistryEntryEditor {
            std::string name;
            std::string category;
            TypeCreatorFunction factory;
        };

        inline static void RegisterInstanceInitializer(IInstanceInitializer* initializer) {
            static std::unordered_map<std::string, IInstanceInitializer*> initializers;
            initializers[initializer->GetTypeHash()] = initializer;
        }

        inline static std::vector<RegistryEntry>& GetEntries() {
            static std::vector<RegistryEntry> entries;
            return entries;
        }

        inline static std::vector<RegistryEntryEditor>& GetEditorEntries() {
            static std::vector<RegistryEntryEditor> editorEntries;
            return editorEntries;
        }

        static void Register(const std::string& name, TypeDeserializerFunction factory) {
            // Check if name already exists
            for (auto& entry : GetEntries()) {
                if (entry.name == name) {
                    entry.factory = factory; // Overwrite old factory
                    return;
                }
            }
            // If it's a unique name, add it
            GetEntries().push_back({ name, factory });
        }

        static void RegisterEditor(const std::string& name, const std::string& category, TypeCreatorFunction factory) {
            // Check if name already exists
            for (auto& entry : GetEditorEntries()) {
                if (entry.name == name) {
                    entry.category = category; // Update category if it changed
                    entry.factory = factory;   // Overwrite old factory
                    return;
                }
            }
            // If it's a unique name, add it
            GetEditorEntries().push_back({ name, category, factory });
        }

        static ISerializable* Instantiate(const std::string& name, SerializedData& data) {
            // Fixed: passed by reference (&) to prevent heavy string copies
            for (const auto& entry : GetEntries())
            {
                if (entry.name == name) {
                    return entry.factory(data);
                }
            }
            return nullptr; // Good practice to explicitly return nullptr if not found
        }
    };

// Registers an initializer
#define GBE_REGISTER_INITIALIZER(Type) \
    inline static Type Type##_instance; \
    static bool Type##_registered = []() { \
        gbe::TypeRegistry::RegisterInstanceInitializer(&Type##_instance); \
        return true; \
    }()

//Creates the object then passes it to the instance initializer
#define GBE_CREATE(Name, Type, Parameters) \
    Type* Name = nullptr; \
    { \
        auto newobj_ptr = std::make_unique<Type>Parameters; \
        Name = newobj_ptr.get(); \
        gbe::InstanceInitializer<Type>::Initialize(std::move(newobj_ptr));\
    }

    // Used to register an object type for runtime serialization/saving
#define GBE_REGISTER_SERIALIZED_TYPE(Type) \
    static bool Type##_registered = []() { \
        gbe::TypeRegistry::Register(typeid(Type).name(), [](gbe::SerializedData& data) { \
            GBE_CREATE(newobj, Type, (data)); \
            return newobj; \
        }); \
        return true; \
    }()

#define GBE_GENERATE_SERIALIZER_CONSTRUCTOR(Type, Base) \
    public: \
    inline Type(gbe::SerializedData& data) : Base(data) {} \
    protected:
}
