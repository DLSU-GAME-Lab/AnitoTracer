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
#include <utility>
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

struct HostEntry {
    enum class Kind { Log, Component, Static };
    Kind kind;
    std::string field;
    std::string type;
    std::string binding;
    std::string header;
};

bool IsIdentifier(std::string_view value) {
    static const std::set<std::string_view> keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break",
        "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept",
        "const", "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return",
        "co_yield", "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum",
        "explicit", "export", "extern", "false", "float", "for", "friend", "goto", "if", "inline", "int",
        "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or",
        "or_eq", "private", "protected", "public", "register", "reinterpret_cast", "requires", "return",
        "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct", "switch",
        "template", "this", "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
        "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
    };
    if (value.empty() || !((value.front() >= 'a' && value.front() <= 'z') ||
                           (value.front() >= 'A' && value.front() <= 'Z'))) return false;
    for (char c : value) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_')) return false;
    }
    return !keywords.contains(value);
}

std::string Lowercase(std::string_view value) {
    std::string result(value);
    for (char& c : result) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return result;
}

std::vector<HostEntry> ParseHostLayout(int argc, char** argv) {
    std::vector<HostEntry> entries;
    std::set<std::string> fields;
    std::set<std::string> types = {"HostAPI"};
    std::set<std::string> componentHeaders;
    std::set<std::string> staticHeaders = {"common", "hostapi", "module"};
    std::set<std::string> componentBindings;
    std::set<std::string> staticBindings;
    bool hasLog = false;
    for (int i = 3; i < argc; ++i) {
        const std::string entry(argv[i]);
        std::vector<std::string> parts;
        size_t start = 0;
        for (size_t end = entry.find(':'); end != std::string::npos; end = entry.find(':', start)) {
            parts.push_back(entry.substr(start, end - start));
            start = end + 1;
        }
        parts.push_back(entry.substr(start));

        HostEntry parsed;
        if (parts.size() == 1 && parts[0] == "log") {
            if (hasLog) throw std::runtime_error("duplicate log entry in HostAPI layout");
            hasLog = true;
            parsed = {HostEntry::Kind::Log, "Log", "", "", ""};
        } else if (parts.size() == 2 && parts[0] == "component") {
            const std::string& category = parts[1];
            if (!IsIdentifier(category) || category.front() < 'a' || category.front() > 'z' ||
                category.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_") != std::string::npos)
                throw std::runtime_error("invalid component category in HostAPI layout: " + category);
            parsed = {HostEntry::Kind::Component, category, Pascal(category) + "API", category, Pascal(category)};
            if (!componentHeaders.insert(Lowercase(parsed.header)).second)
                throw std::runtime_error("duplicate component header in HostAPI layout: " + parsed.header);
            componentBindings.insert(parsed.binding);
            if (Lowercase(parsed.header) == "hostincludes")
                throw std::runtime_error("component host header conflicts with generated HostIncludes.hpp");
        } else if (parts.size() == 5 && parts[0] == "static") {
            for (size_t part = 1; part < parts.size(); ++part)
                if (!IsIdentifier(parts[part]))
                    throw std::runtime_error("invalid static HostAPI layout entry: " + entry);
            parsed = {HostEntry::Kind::Static, parts[1], parts[2], parts[3], parts[4]};
            staticHeaders.insert(Lowercase(parsed.header));
            staticBindings.insert(parsed.binding);
        } else {
            throw std::runtime_error("invalid HostAPI layout entry: " + entry);
        }
        if (parsed.field == "HostAPI")
            throw std::runtime_error("HostAPI field conflicts with its enclosing type");
        if (!fields.insert(parsed.field).second)
            throw std::runtime_error("duplicate HostAPI field: " + parsed.field);
        if (!parsed.type.empty() && !types.insert(parsed.type).second)
            throw std::runtime_error("duplicate HostAPI table type: " + parsed.type);
        entries.push_back(std::move(parsed));
    }
    if (!hasLog) throw std::runtime_error("HostAPI layout is missing log");
    for (const auto& header : componentHeaders)
        if (staticHeaders.contains(header))
            throw std::runtime_error("component host header conflicts with handwritten header: " + header);
    for (const auto& binding : componentBindings)
        if (staticBindings.contains(binding))
            throw std::runtime_error("component binding conflicts with handwritten binding: " + binding);
    return entries;
}

void EmitHostLayout(const std::vector<HostEntry>& entries, const std::filesystem::path& root) {
    const auto host = root / "anito" / "generated" / "host";
    const auto wrappers = root / "anito" / "host";
    const auto initializers = root / "generated" / "ScriptHostAPI" / "HostInitializers.inc";
    std::filesystem::create_directories(wrappers);
    std::ofstream includes(host / "HostIncludes.hpp");
    std::ofstream fields(host / "HostFields.inc");
    std::ofstream init(initializers);
    if (!includes || !fields || !init) throw std::runtime_error("cannot write generated HostAPI layout");
    includes << "#pragma once\n\n";
    std::set<std::string> includedHeaders;
    for (const auto& entry : entries) {
        if (entry.kind == HostEntry::Kind::Log) {
            fields << "    void (*Log)(const char* message);\n";
            init << "    ScriptHost::Bindings::Log,\n";
            continue;
        }
        if (includedHeaders.insert(entry.header).second)
            includes << "#include \"anito/host/" << entry.header << ".hpp\"\n";
        fields << "    " << entry.type << " " << entry.field << ";\n";
        init << "    ScriptHost::Bindings::" << entry.binding << "::GetAPI(),\n";
        if (entry.kind == HostEntry::Kind::Component) {
            const auto path = wrappers / (entry.header + ".hpp");
            std::ofstream out(path);
            if (!out) throw std::runtime_error("cannot write " + path.string());
            out << "#pragma once\n\n#include \"anito/generated/host/" << entry.header << ".hpp\"\n";
            out.close();
            if (!out) throw std::runtime_error("cannot write " + path.string());
        }
    }
    includes.close();
    fields.close();
    init.close();
    if (!includes || !fields || !init) throw std::runtime_error("cannot write generated HostAPI layout");
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 4)
            throw std::runtime_error(
                "usage: anito_scriptapigen <generated-sdk-root> <component-source-root> <layout-entry>...");
        const auto layout = ParseHostLayout(argc, argv);
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
        for (const auto& entry : layout)
            if (entry.kind == HostEntry::Kind::Component) requestedCategories.insert(entry.field);
        for (const auto& [category, categoryTypes] : categories) {
            if (!requestedCategories.contains(category))
                throw std::runtime_error("component '" + categoryTypes.front()->scriptName +
                                         "' declares host category '" + category +
                                         "', but no CMake output was declared for it; add the category to "
                                         "SCRIPT_HOST_API_LAYOUT");
            (void)categoryTypes;
        }

        for (const std::string& category : requestedCategories) {
            const auto found = categories.find(category);
            if (found == categories.end())
                throw std::runtime_error("no annotated component uses host category: " + category);
        }
        EmitHostLayout(layout, root);
        EmitComponentBindings(requestedCategories, bindings / "ComponentBindings.hpp");
        for (const std::string& category : requestedCategories) {
            const auto found = categories.find(category);
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
