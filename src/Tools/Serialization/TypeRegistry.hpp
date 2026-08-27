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
		typedef std::function<ISerializable* (SerializedData&)> TypeDeserializerFunction;
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


	/*
	* @brief Registers a type instance initializer with the global Game Engine (GBE) registry.
	*
	* Creates a static instance of the specified type and automatically executes a lambda
	* function during static initialization to register its address.
	*
	* @param Type The class or struct type to create an instance of and register.
	*/
#define GBE_REGISTER_INITIALIZER(Type) \
    inline static Type Type##_instance; \
    static bool Type##_registered = []() { \
        gbe::TypeRegistry::RegisterInstanceInitializer(&Type##_instance); \
        return true; \
    }()

	/*
	* @brief Instantiates a managed object and registers it with the category instance initializer.
	*
	* Allocates a unique pointer of the specified type using the provided arguments, assigns
	* the raw pointer to a local variable, and transfers ownership to the GBE framework.
	*
	* @param Name The variable name assigned to the resulting raw pointer.
	* @param Type The class or struct type to instantiate.
	* @param CategoryType The GBE subsystem category responsible for initialization management.
	* @param Parameters The constructor arguments enclosed in parentheses, e.g., (arg1, arg2).
	*/
#define GBE_CREATE(Name, Type, CategoryType, Parameters) \
    Type* Name = nullptr; \
    { \
        auto newobj_ptr = std::make_unique<Type>Parameters; \
        Name = newobj_ptr.get(); \
        gbe::InstanceInitializer<CategoryType>::Initialize(std::move(newobj_ptr));\
    }
	/*
	* @brief Registers a serializable C++ type with the engine registry for dynamic instantiation.
	*
	* Binds the RTTI type name to a creation factory lambda. When triggered, the factory reads
	* serialized data, constructs the object using GBE_CREATE, and returns the raw pointer.
	*
	* @param Type The serializable class or struct type to register.
	* @param "Base" The type that you have made a InstanceInitialize<T> for. Usually is the grandfather class that inherits off of ISerializable directly.
	*/
#define GBE_REGISTER_SERIALIZED_TYPE(Type, CategoryType) \
    static bool Type##_registered = []() { \
        gbe::TypeRegistry::Register(typeid(Type).name(), [](gbe::SerializedData& data) { \
            GBE_CREATE(newobj, Type, CategoryType, (data)); \
            return newobj; \
        }); \
        return true; \
    }()

	/// @brief Creates the required constructors of a default serializable
	/// @param "Type" Target type of the current class.
	/// @param "Base" Base type of the current class.
#define GBE_GENERATE_SERIALIZER_CONSTRUCTOR(Type, Base) \
    public: \
    inline Type(gbe::SerializedData& data) : Base(data) {GBE_Init();} \
    protected:

	/// @brief Creates the required constructors of a default serializable.
	/// @param "Type" Target type of the current class.
	/// @param "Base" Base type of the current class.
	/// @param "NameGetter" Lambda to get the name of this instance.
#define GBE_GENERATE_SERIALIZER_CONSTRUCTOR_W_NAME(Type, Base, NameGetter) \
    public: \
    inline Type(gbe::SerializedData& data) : Base(data) {GBE_Init();} \
	inline virtual std::string GetLabel() override {return NameGetter();} \
    protected:

}
