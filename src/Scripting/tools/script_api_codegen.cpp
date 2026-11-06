#include "ScriptApiMetadata.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using anito::script::MemberMetadata;
using anito::script::MethodMetadata;
using anito::script::TypeMetadata;

std::string Pascal(std::string_view name) {
    std::string result(name);
    if (!result.empty()) result.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(result.front())));
    return result;
}

std::string LastName(std::string_view qualifiedName) {
    const size_t separator = qualifiedName.rfind("::");
    return std::string(separator == std::string_view::npos ? qualifiedName : qualifiedName.substr(separator + 2));
}

std::string HandleName(const TypeMetadata& type) {
    return LastName(type.cppType);
}

std::string SdkType(std::string_view type) {
    if (type == "void") return "void";
    if (type == "float") return "float";
    if (type == "int") return "int";
    if (type == "bool") return "bool";
    if (type == "string") return "std::string";
    if (type == "vec3") return "Vec3";
    if (type == "quat") return "Quat";
    if (type == "raycasthit") return "RaycastHit";
    throw std::runtime_error("unsupported generated SDK type: " + std::string(type));
}

std::string AbiType(std::string_view type) {
    if (type == "string") return "const char*";
    if (type == "vec3") return "::anito::Vec3";
    if (type == "quat") return "::anito::Quat";
    if (type == "raycasthit") return "::anito::RaycastResult";
    if (type == "void" || type == "float" || type == "int" || type == "bool") return SdkType(type);
    throw std::runtime_error("unsupported generated ABI type: " + std::string(type));
}

std::string EngineArgument(std::string_view type, std::string_view argument) {
    if (type == "string") return "std::string(" + std::string(argument) + " ? " + std::string(argument) + " : \"\")";
    if (type == "vec3") return "detail::ToGlm(" + std::string(argument) + ")";
    if (type == "quat") return "detail::ToGlm(" + std::string(argument) + ")";
    return std::string(argument);
}

std::string AbiResult(std::string_view type, std::string_view value) {
    if (type == "string") return "detail::TransientString(" + std::string(value) + ")";
    if (type == "vec3") return "detail::FromGlm(" + std::string(value) + ")";
    if (type == "quat") return "detail::FromGlm(" + std::string(value) + ")";
    if (type == "raycasthit")
        return "::anito::RaycastResult{" + std::string(value) + ".hit ? 1u : 0u, " + std::string(value) +
               ".object, detail::FromGlm(" + std::string(value) + ".point)}";
    return std::string(value);
}

std::string DefaultValue(const MemberMetadata& member) {
    if (!member.defaultValue.empty()) return member.defaultValue;
    if (member.type == "string") return "\"\"";
    return "{}";
}

std::string ApiGetter(const MemberMetadata& member) {
    return "Get" + Pascal(member.name);
}

std::string ApiGetter(const TypeMetadata& type, const MemberMetadata& member) {
    return HandleName(type) + ApiGetter(member);
}

std::string ApiSetter(const MemberMetadata& member) {
    return "Set" + Pascal(member.name);
}

std::string ApiSetter(const TypeMetadata& type, const MemberMetadata& member) {
    return HandleName(type) + ApiSetter(member);
}

std::string ApiMethod(const TypeMetadata& type, const MethodMetadata& method) {
    return HandleName(type) + Pascal(method.name);
}

void RequireSimpleSignature(const TypeMetadata& type) {
    if (type.hostCategory.empty())
        throw std::runtime_error("component metadata is missing its host category: " + type.scriptName);
    if (type.header.empty() || type.engineType.empty())
        throw std::runtime_error("component metadata is missing its C++ type or source header");

    for (const auto& member : type.members) {
        (void)SdkType(member.type);
        if (member.getter.empty()) throw std::runtime_error("property is missing its C++ getter: " + member.name);
        if (member.settable && member.setter.empty())
            throw std::runtime_error("writable property is missing its C++ setter: " + member.name);
    }
    for (const auto& method : type.methods) {
        if (method.cppName.empty()) throw std::runtime_error("method is missing its C++ name: " + method.name);
        (void)SdkType(method.result);
        for (const auto& parameter : method.parameters) (void)SdkType(parameter);
    }
}

void EmitHostTable(const std::vector<const TypeMetadata*>& types, std::string_view category,
                   const std::filesystem::path& path) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot write " + path.string());

    out << "#pragma once\n\n"
           "#include \"anito/host/Common.hpp\"\n\n"
           "namespace anito {\n"
           "struct " << Pascal(category) << "API {\n";
    for (const TypeMetadata* type : types) {
        if (type->hasValid) out << "    bool (*Has" << HandleName(*type) << ")(ObjectId);\n";
        for (const auto& member : type->members) {
            out << "    " << AbiType(member.type) << " (*" << ApiGetter(*type, member) << ")(ObjectId);\n";
            if (member.settable)
                out << "    void (*" << ApiSetter(*type, member) << ")(ObjectId, " << AbiType(member.type) << ");\n";
        }
        for (const auto& method : type->methods) {
            out << "    " << AbiType(method.result) << " (*" << ApiMethod(*type, method) << ")(ObjectId";
            for (const auto& parameter : method.parameters) out << ", " << AbiType(parameter);
            out << ");\n";
        }
    }
    out << "};\n} // namespace anito\n";
}

void EmitSdkHandles(const std::vector<const TypeMetadata*>& types, std::string_view category,
                    const std::filesystem::path& path) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot write " + path.string());

    out << "#pragma once\n\n"
           "#include \"anito/api/Common.hpp\"\n";
    for (const TypeMetadata* type : types) {
        for (const auto& method : type->methods) {
            if (method.result == "raycasthit") out << "#include \"anito/api/RaycastHit.hpp\"\n";
        }
    }
    out << "\nnamespace anito::generated {\n";
    for (const TypeMetadata* type : types) {
        const std::string handleName = HandleName(*type);
        out << "class " << handleName << "Handle {\n"
               "public:\n"
               "    ObjectId id = 0;\n"
               "    explicit " << handleName << "Handle(ObjectId value = 0) : id(value) {}\n\n";
        if (type->hasValid)
            out << "    bool valid() const { return detail::H()." << category << ".Has" << handleName
                << "(id); }\n";
        out << "    inline GameObject gameObject() const;\n\n";

        for (const auto& member : type->members) {
            const auto sdkType = SdkType(member.type);
            out << "    " << sdkType << " " << member.name << "() const { ";
            if (member.type == "string") {
                out << "const char* value = detail::H()." << category << "." << ApiGetter(*type, member) << "(id); "
                    << "return value ? value : \"\"; }\n";
            } else {
                out << "return detail::H()." << category << "." << ApiGetter(*type, member) << "(id); }\n";
            }
            if (member.settable) {
                const std::string paramType = member.type == "string" ? "const std::string&" : sdkType;
                const std::string arg = member.type == "string" ? "value.c_str()" : "value";
                out << "    void set" << Pascal(member.name) << "(" << paramType
                    << " value) const { detail::H()." << category << "." << ApiSetter(*type, member) << "(id, "
                    << arg << "); }\n";
            }
        }

        for (const auto& method : type->methods) {
            out << "    " << SdkType(method.result) << " " << method.name << "(";
            for (size_t i = 0; i < method.parameters.size(); ++i) {
                if (i) out << ", ";
                const auto paramType = SdkType(method.parameters[i]);
                out << (method.parameters[i] == "string" ? "const std::string&" : paramType) << " arg" << i;
            }
            out << ") const { ";
            if (method.result == "string") out << "const char* value = ";
            else if (method.result == "raycasthit") out << "RaycastHit value{}; value.result = ";
            else if (method.result != "void") out << "return ";
            out << "detail::H()." << category << "." << ApiMethod(*type, method) << "(id";
            for (size_t i = 0; i < method.parameters.size(); ++i)
                out << ", " << (method.parameters[i] == "string" ? "arg" + std::to_string(i) + ".c_str()"
                                                                   : "arg" + std::to_string(i));
            out << ");";
            if (method.result == "string") out << " return value ? value : \"\";";
            if (method.result == "raycasthit") out << " return value;";
            out << " }\n";
        }

        out << "\n    bool operator==(const " << handleName << "Handle& other) const { return id == other.id; }\n"
               "    bool operator!=(const " << handleName << "Handle& other) const { return id != other.id; }\n"
               "};\n\n";
    }
    out << "} // namespace anito::generated\n";
}

void EmitBridge(const std::vector<const TypeMetadata*>& types, std::string_view category,
                const std::filesystem::path& path, const std::filesystem::path& componentRoot) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot write " + path.string());

    out << "#include \"ScriptHostAPI/Bindings/ComponentBindings.hpp\"\n"
           "#include \"ScriptHostAPI/Common.hpp\"\n"
           "#include \"anito/api/Components.hpp\"\n"
           "#include \"ScriptApiTypeMappings.hpp\"\n";
    out << "\nnamespace {\n"
           "template <typename HostApi>\n"
           "inline constexpr bool kAnitoScriptHasHostCategory_" << category
        << " = requires(HostApi& host) { host." << category << "; };\n"
           "static_assert(kAnitoScriptHasHostCategory_" << category
        << "<anito::HostAPI>, \"ANITO_SCRIPT_TYPE: category does not exist in HostAPI.\");\n"
           "} // namespace\n";

    std::set<std::string> headers;
    for (const TypeMetadata* type : types) {
        const std::filesystem::path headerPath(type->header);
        const std::filesystem::path relativeHeader =
            headerPath.is_absolute() ? headerPath.lexically_relative(componentRoot) : headerPath;
        if (relativeHeader.empty() || relativeHeader.is_absolute() || *relativeHeader.begin() == "..")
            throw std::runtime_error("component header is outside the configured source root: " + type->header);
        headers.insert(relativeHeader.generic_string());
    }
    for (const auto& header : headers) out << "#include \"" << header << "\"\n";
    out << "\nnamespace ScriptHost::Bindings::" << category << " {\n";

    for (const TypeMetadata* type : types) {
        const std::string handleName = HandleName(*type);
        if (type->hasValid) {
            out << "bool Has" << handleName << "(anito::ObjectId id) {\n"
                   "    return detail::GetComponent<" << type->engineType << ">(id) != nullptr;\n"
                   "}\n\n";
        }
        for (const auto& member : type->members) {
            const std::string abiType = AbiType(member.type);
            out << abiType << " " << ApiGetter(*type, member) << "(anito::ObjectId id) {\n"
                   "    auto* component = detail::GetComponent<" << type->engineType << ">(id);\n"
                   "    if (!component) return " << DefaultValue(member) << ";\n";
            if (member.type == "string") {
                out << "    auto value = component->" << member.getter << "();\n"
                       "    return detail::TransientString(value);\n";
            } else {
                out << "    return " << AbiResult(member.type, "component->" + member.getter + "()") << ";\n";
            }
            out << "}\n\n";
            if (member.settable) {
                out << "void " << ApiSetter(*type, member) << "(anito::ObjectId id, " << abiType << " value) {\n"
                       "    if (auto* component = detail::GetComponent<" << type->engineType << ">(id))\n"
                       "        component->" << member.setter << "(" << EngineArgument(member.type, "value")
                    << ");\n}\n\n";
            }
        }
        for (const auto& method : type->methods) {
            out << AbiType(method.result) << " " << ApiMethod(*type, method) << "(anito::ObjectId id";
            for (size_t i = 0; i < method.parameters.size(); ++i)
                out << ", " << AbiType(method.parameters[i]) << " arg" << i;
            out << ") {\n"
                   "    auto* component = detail::GetComponent<" << type->engineType << ">(id);\n";
            if (method.result == "void") {
                out << "    if (!component) return;\n"
                       "    component->" << method.cppName << "(";
            } else {
                out << "    if (!component) return {};\n"
                       "    auto value = component->" << method.cppName << "(";
            }
            for (size_t i = 0; i < method.parameters.size(); ++i) {
                if (i) out << ", ";
                out << EngineArgument(method.parameters[i], "arg" + std::to_string(i));
            }
            out << ");\n";
            if (method.result != "void")
                out << "    return " << AbiResult(method.result, "value") << ";\n";
            out << "}\n\n";
        }
    }

    out << "anito::" << Pascal(category) << "API GetAPI() {\n    return {";
    bool comma = false;
    const auto emitField = [&out, &comma](const std::string& field) {
        if (comma) out << ", ";
        out << field;
        comma = true;
    };
    for (const TypeMetadata* type : types) {
        if (type->hasValid) emitField("Has" + HandleName(*type));
        for (const auto& member : type->members) {
            emitField(ApiGetter(*type, member));
            if (member.settable) emitField(ApiSetter(*type, member));
        }
        for (const auto& method : type->methods) emitField(ApiMethod(*type, method));
    }
    out << "};\n}\n";

    for (const TypeMetadata* type : types) {
        const std::string handle = "anito::" + HandleName(*type);
        if (type->hasValid) out << "ANITO_SCRIPT_CONFORMS_VALID(" << handle << ");\n";
        for (const auto& member : type->members) {
            if (member.settable) {
                out << "ANITO_SCRIPT_CONFORMS_PROPERTY(" << type->engineType << ", " << member.getter << ", "
                    << member.setter << ", " << handle << ", " << member.name << ", set"
                    << Pascal(member.name) << ");\n";
            } else {
                out << "ANITO_SCRIPT_CONFORMS_READ_ONLY_PROPERTY(" << type->engineType << ", " << member.getter
                    << ", " << handle << ", " << member.name << ");\n";
            }
        }
        for (const auto& method : type->methods)
            out << "ANITO_SCRIPT_CONFORMS_METHOD(" << type->engineType << ", " << method.cppName << ", "
                << handle << ", " << method.name << ");\n";
    }
    out << "} // namespace ScriptHost::Bindings::" << category << "\n";
}

void EmitComponentBindings(const std::set<std::string>& categories, const std::filesystem::path& path) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot write " + path.string());

    out << "#pragma once\n\n"
           "#include \"ScriptHostAPI.hpp\"\n\n";
    for (const std::string& category : categories) {
        out << "namespace ScriptHost::Bindings::" << category << " {\n"
            << "anito::" << Pascal(category) << "API GetAPI();\n"
            << "} // namespace ScriptHost::Bindings::" << category << "\n\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 4)
            throw std::runtime_error(
                "usage: anito_scriptapigen <generated-sdk-root> <component-source-root> <category>...");
        const auto& types = anito::script::RegisteredTypes();
        std::map<std::string, std::vector<const TypeMetadata*>> categories;
        for (const auto& [scriptName, type] : types) {
            (void)scriptName;
            RequireSimpleSignature(type);
            categories[type.hostCategory].push_back(&type);
        }

        const std::filesystem::path root(argv[1]);
        const std::filesystem::path componentRoot(argv[2]);
        const auto bindings = root / "ScriptHostAPI" / "Bindings";
        const auto api = root / "anito" / "generated" / "api";
        const auto host = root / "anito" / "generated" / "host";
        const auto source = root / "generated" / "ScriptHostAPI";
        std::filesystem::create_directories(bindings);
        std::filesystem::create_directories(api);
        std::filesystem::create_directories(host);
        std::filesystem::create_directories(source);

        std::set<std::string> requestedCategories;
        for (int i = 3; i < argc; ++i) requestedCategories.insert(argv[i]);
        for (const auto& [category, categoryTypes] : categories) {
            if (!requestedCategories.contains(category))
                throw std::runtime_error("component '" + categoryTypes.front()->scriptName +
                                         "' declares host category '" + category +
                                         "', but no CMake output was declared for it; add the category to "
                                         "SCRIPT_API_CATEGORIES and wire its HostAPI table");
            (void)categoryTypes;
        }

        EmitComponentBindings(requestedCategories, bindings / "ComponentBindings.hpp");
        for (const std::string& category : requestedCategories) {
            const auto found = categories.find(category);
            if (found == categories.end())
                throw std::runtime_error("no annotated component uses host category: " + category);
            const std::string categoryName = Pascal(category);
            const auto& categoryTypes = found->second;
            EmitSdkHandles(categoryTypes, category, api / (categoryName + ".hpp"));
            EmitHostTable(categoryTypes, category, host / (categoryName + ".hpp"));
            EmitBridge(categoryTypes, category, source / (categoryName + ".cpp"), componentRoot);
        }
    } catch (const std::exception& error) {
        std::cerr << "script API generation failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
