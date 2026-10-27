#pragma once

#include <map>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace anito::script {

struct MemberMetadata {
    std::string name;
    std::string type;
    bool settable = false;
};

struct MethodMetadata {
    std::string name;
    std::string result;
    std::vector<std::string> parameters;
};

struct TypeMetadata {
    std::string scriptName;
    std::string cppType;
    bool hasValid = false;
    std::vector<MemberMetadata> members;
    std::vector<MethodMetadata> methods;
};

using TypeRegistry = std::map<std::string, TypeMetadata>;

inline TypeRegistry& GetTypeRegistry() {
    static TypeRegistry registry;
    return registry;
}

template <typename>
inline constexpr bool kUnsupportedScriptType = false;

template <typename T>
struct ScriptTypeMapping {
    static constexpr bool supported = false;
};

#define ANITO_SCRIPT_DETAIL_TYPE_MAPPING(CppType, ScriptName) \
    template <>                                               \
    struct ScriptTypeMapping<CppType> {                       \
        static constexpr bool supported = true;               \
        static constexpr const char* name = ScriptName;       \
    }

#define ANITO_SCRIPT_TYPE_MAPPING(CppType, ScriptName) \
    namespace anito::script {                          \
    ANITO_SCRIPT_DETAIL_TYPE_MAPPING(CppType, ScriptName); \
    }

ANITO_SCRIPT_DETAIL_TYPE_MAPPING(void, "void");
ANITO_SCRIPT_DETAIL_TYPE_MAPPING(float, "float");
ANITO_SCRIPT_DETAIL_TYPE_MAPPING(int, "int");
ANITO_SCRIPT_DETAIL_TYPE_MAPPING(bool, "bool");
ANITO_SCRIPT_DETAIL_TYPE_MAPPING(std::string, "string");
ANITO_SCRIPT_DETAIL_TYPE_MAPPING(glm::vec3, "vec3");
ANITO_SCRIPT_DETAIL_TYPE_MAPPING(glm::quat, "quat");

template <typename T>
constexpr const char* ScriptTypeName() {
    using Type = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr (ScriptTypeMapping<Type>::supported) {
        return ScriptTypeMapping<Type>::name;
    } else {
        static_assert(kUnsupportedScriptType<Type>, "Type is not supported by the script API");
        return "";
    }
}

template <typename>
struct MemberFunctionTraits;

template <typename Result, typename Class, typename... Parameters>
struct MemberFunctionTraits<Result (Class::*)(Parameters...)> {
    using Return = Result;
    using ParameterTypes = std::tuple<Parameters...>;

    static std::vector<std::string> ParameterNames() {
        return {ScriptTypeName<Parameters>()...};
    }
};

template <typename Result, typename Class, typename... Parameters>
struct MemberFunctionTraits<Result (Class::*)(Parameters...) const>
    : MemberFunctionTraits<Result (Class::*)(Parameters...)> {};

template <typename Result, typename Class, typename... Parameters>
struct MemberFunctionTraits<Result (Class::*)(Parameters...) noexcept>
    : MemberFunctionTraits<Result (Class::*)(Parameters...)> {};

template <typename Result, typename Class, typename... Parameters>
struct MemberFunctionTraits<Result (Class::*)(Parameters...) const noexcept>
    : MemberFunctionTraits<Result (Class::*)(Parameters...)> {};

inline bool SetPropertyWritable(const char* scriptType, const char* propertyName) {
    auto& members = GetTypeRegistry()[scriptType].members;
    for (auto& member : members) {
        if (member.name == propertyName) {
            member.settable = true;
            return true;
        }
    }
    return false;
}

template <typename Class>
bool RegisterType(const char* scriptName, const char* cppType, bool hasValid) {
    auto& type = GetTypeRegistry()[scriptName];
    type.scriptName = scriptName;
    type.cppType = cppType;
    type.hasValid = hasValid;
    return true;
}

template <typename Getter>
bool RegisterReadOnlyProperty(const char* scriptType, const char* propertyName) {
    using Traits = MemberFunctionTraits<Getter>;
    using PropertyType = typename Traits::Return;
    auto& members = GetTypeRegistry()[scriptType].members;
    for (auto& member : members) {
        if (member.name == propertyName) {
            member.type = ScriptTypeName<PropertyType>();
            member.settable = false;
            return true;
        }
    }
    members.push_back({propertyName, ScriptTypeName<PropertyType>(), false});
    return true;
}

template <typename Getter, typename Setter>
bool RegisterProperty(const char* scriptType, const char* propertyName) {
    using GetterTraits = MemberFunctionTraits<Getter>;
    using SetterTraits = MemberFunctionTraits<Setter>;
    using PropertyType = typename GetterTraits::Return;
    using SetterParameters = typename SetterTraits::ParameterTypes;
    static_assert(std::is_same_v<typename SetterTraits::Return, void>, "Script property setters must return void");
    static_assert(!std::is_same_v<PropertyType, void>, "Script property getters must return a value");
    static_assert(std::tuple_size_v<SetterParameters> == 1, "Script property setters must accept one value");
    static_assert(std::is_same_v<
                      std::remove_cv_t<std::remove_reference_t<PropertyType>>,
                      std::remove_cv_t<std::remove_reference_t<std::tuple_element_t<0, SetterParameters>>>>,
                  "Script property getter and setter types must match");
    return RegisterReadOnlyProperty<Getter>(scriptType, propertyName) &&
           SetPropertyWritable(scriptType, propertyName);
}

template <typename Method>
bool RegisterMethod(const char* scriptType, const char* methodName) {
    using Traits = MemberFunctionTraits<Method>;
    MethodMetadata metadata{methodName, ScriptTypeName<typename Traits::Return>(), Traits::ParameterNames()};
    auto& methods = GetTypeRegistry()[scriptType].methods;
    for (auto& method : methods) {
        if (method.name == methodName) {
            method = std::move(metadata);
            return true;
        }
    }
    methods.push_back(std::move(metadata));
    return true;
}

} // namespace anito::script

#define ANITO_SCRIPT_DETAIL_JOIN_IMPL(a, b) a##b
#define ANITO_SCRIPT_DETAIL_JOIN(a, b) ANITO_SCRIPT_DETAIL_JOIN_IMPL(a, b)

#define ANITO_SCRIPT_TYPE(ClassName, ScriptName, CppName, HasValid)                                      \
    namespace {                                                                                         \
    [[maybe_unused]] const bool ANITO_SCRIPT_DETAIL_JOIN(_anito_script_type_, __COUNTER__) =             \
        ::anito::script::RegisterType<ClassName>(ScriptName, CppName, HasValid);                        \
    }

#define ANITO_SCRIPT_PROPERTY_RO(ClassName, ScriptType, ScriptName, Getter)                              \
    namespace {                                                                                         \
    [[maybe_unused]] const bool ANITO_SCRIPT_DETAIL_JOIN(_anito_script_member_, __COUNTER__) =          \
        ::anito::script::RegisterReadOnlyProperty<decltype(&ClassName::Getter)>(ScriptType, ScriptName); \
    }

#define ANITO_SCRIPT_PROPERTY(ClassName, ScriptType, ScriptName, Getter, Setter)                         \
    namespace {                                                                                         \
    [[maybe_unused]] const bool ANITO_SCRIPT_DETAIL_JOIN(_anito_script_member_, __COUNTER__) =          \
        ::anito::script::RegisterProperty<decltype(&ClassName::Getter),                                  \
                                          decltype(&ClassName::Setter)>(ScriptType, ScriptName);          \
    }

#define ANITO_SCRIPT_METHOD(ClassName, ScriptType, ScriptName, MethodName)                               \
    namespace {                                                                                         \
    [[maybe_unused]] const bool ANITO_SCRIPT_DETAIL_JOIN(_anito_script_method_, __COUNTER__) =          \
        ::anito::script::RegisterMethod<decltype(&ClassName::MethodName)>(ScriptType, ScriptName);       \
    }
