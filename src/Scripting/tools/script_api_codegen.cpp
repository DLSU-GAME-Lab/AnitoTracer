#include "ScriptApiMetadata.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
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

std::string SdkType(std::string_view type) {
    if (type == "void") return "void";
    if (type == "float") return "float";
    if (type == "int") return "int";
    if (type == "bool") return "bool";
    if (type == "string") return "std::string";
    if (type == "vec3") return "Vec3";
    if (type == "quat") return "Quat";
    throw std::runtime_error("unsupported generated SDK type: " + std::string(type));
}

std::string AbiType(std::string_view type) {
    if (type == "string") return "const char*";
    return SdkType(type);
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

std::string ApiSetter(const MemberMetadata& member) {
    return "Set" + Pascal(member.name);
}

void RequireSimpleSignature(const TypeMetadata& type) {
    if (type.scriptName != "audiosource" || type.hostCategory != "audio")
        throw std::runtime_error("this generator version only emits the AudioSource component API");
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

std::string HandleName(const TypeMetadata& type) {
    return LastName(type.cppType);
}

void EmitHostTable(const TypeMetadata& type, const std::filesystem::path& path) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot write " + path.string());

    out << "#pragma once\n\n"
           "#include \"anito/host/Common.hpp\"\n\n"
           "namespace anito {\n"
           "struct AudioAPI {\n";
    if (type.hasValid) out << "    bool (*Has" << HandleName(type) << ")(ObjectId);\n";
    for (const auto& member : type.members) {
        out << "    " << AbiType(member.type) << " (*" << ApiGetter(member) << ")(ObjectId);\n";
        if (member.settable)
            out << "    void (*" << ApiSetter(member) << ")(ObjectId, " << AbiType(member.type) << ");\n";
    }
    for (const auto& method : type.methods) {
        out << "    " << AbiType(method.result) << " (*" << Pascal(method.name) << ")(ObjectId";
        for (const auto& parameter : method.parameters) out << ", " << AbiType(parameter);
        out << ");\n";
    }
    out << "};\n} // namespace anito\n";
}

void EmitSdkHandle(const TypeMetadata& type, const std::filesystem::path& path) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot write " + path.string());

    out << "#pragma once\n\n"
           "#include \"anito/api/Common.hpp\"\n\n"
           "namespace anito {\n"
           "class " << HandleName(type) << " {\n"
           "public:\n"
           "    ObjectId id = 0;\n\n";
    if (type.hasValid)
        out << "    bool valid() const { return detail::H().audio.Has" << HandleName(type) << "(id); }\n";
    out << "    inline GameObject gameObject() const;\n\n";

    for (const auto& member : type.members) {
        const auto sdkType = SdkType(member.type);
        out << "    " << sdkType << " " << member.name << "() const { ";
        if (member.type == "string") {
            out << "const char* value = detail::H().audio." << ApiGetter(member) << "(id); "
                << "return value ? value : \"\"; }\n";
        } else {
            out << "return detail::H().audio." << ApiGetter(member) << "(id); }\n";
        }
        if (member.settable) {
            const std::string paramType = member.type == "string" ? "const std::string&" : sdkType;
            const std::string arg = member.type == "string" ? "value.c_str()" : "value";
            out << "    void set" << Pascal(member.name) << "(" << paramType
                << " value) const { detail::H().audio." << ApiSetter(member) << "(id, " << arg << "); }\n";
        }
    }

    for (const auto& method : type.methods) {
        out << "    " << SdkType(method.result) << " " << method.name << "(";
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            if (i) out << ", ";
            const auto paramType = SdkType(method.parameters[i]);
            out << (method.parameters[i] == "string" ? "const std::string&" : paramType) << " arg" << i;
        }
        out << ") const { ";
        if (method.result != "void") {
            if (method.result == "string") {
                out << "const char* value = ";
            } else {
                out << "return ";
            }
        }
        out << "detail::H().audio." << Pascal(method.name) << "(id";
        for (size_t i = 0; i < method.parameters.size(); ++i)
            out << ", " << (method.parameters[i] == "string" ? "arg" + std::to_string(i) + ".c_str()"
                                                               : "arg" + std::to_string(i));
        out << ");";
        if (method.result == "string") out << " return value ? value : \"\";";
        out << " }\n";
    }

    out << "\n    bool operator==(const " << HandleName(type) << "& other) const { return id == other.id; }\n"
           "    bool operator!=(const " << HandleName(type) << "& other) const { return id != other.id; }\n"
           "};\n} // namespace anito\n";
}

void EmitBridge(const TypeMetadata& type, const std::filesystem::path& path,
                const std::filesystem::path& componentRoot) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot write " + path.string());

    const std::filesystem::path headerPath(type.header);
    const std::filesystem::path relativeHeader =
        headerPath.is_absolute() ? headerPath.lexically_relative(componentRoot) : headerPath;
    if (relativeHeader.empty() || relativeHeader.is_absolute() ||
        (!relativeHeader.empty() && *relativeHeader.begin() == ".."))
        throw std::runtime_error("component header is outside the configured source root: " + type.header);
    const std::string componentHeader = relativeHeader.generic_string();
    out << "#include \"ScriptHostAPI/Bindings/Audio.hpp\"\n"
           "#include \"ScriptHostAPI/Common.hpp\"\n"
           "#include \"anito/api/AudioSource.hpp\"\n"
           "#include \"" << componentHeader << "\"\n\n"
           "namespace ScriptHost::Bindings::audio {\n";

    if (type.hasValid) {
        out << "bool Has" << HandleName(type) << "(anito::ObjectId id) {\n"
               "    return detail::GetComponent<" << type.engineType << ">(id) != nullptr;\n"
               "}\n\n";
    }

    for (const auto& member : type.members) {
        const std::string abiType = AbiType(member.type);
        out << abiType << " " << ApiGetter(member) << "(anito::ObjectId id) {\n"
               "    auto* component = detail::GetComponent<" << type.engineType << ">(id);\n"
               "    if (!component) return " << DefaultValue(member) << ";\n";
        if (member.type == "string") {
            out << "    auto value = component->" << member.getter << "();\n"
                   "    return detail::TransientString(value);\n";
        } else {
            out << "    return " << AbiResult(member.type, "component->" + member.getter + "()") << ";\n";
        }
        out << "}\n\n";
        if (member.settable) {
            out << "void " << ApiSetter(member) << "(anito::ObjectId id, " << abiType << " value) {\n"
                   "    if (auto* component = detail::GetComponent<" << type.engineType << ">(id))\n"
                   "        component->" << member.setter << "(" << EngineArgument(member.type, "value") << ");\n"
                   "}\n\n";
        }
    }

    for (const auto& method : type.methods) {
        out << AbiType(method.result) << " " << Pascal(method.name) << "(anito::ObjectId id";
        for (size_t i = 0; i < method.parameters.size(); ++i)
            out << ", " << AbiType(method.parameters[i]) << " arg" << i;
        out << ") {\n"
               "    auto* component = detail::GetComponent<" << type.engineType << ">(id);\n";
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

    out << "anito::AudioAPI GetAPI() {\n"
           "    return {";
    bool comma = false;
    const auto emitField = [&out, &comma](const std::string& field) {
        if (comma) out << ", ";
        out << field;
        comma = true;
    };
    if (type.hasValid) emitField("Has" + HandleName(type));
    for (const auto& member : type.members) {
        emitField(ApiGetter(member));
        if (member.settable) emitField(ApiSetter(member));
    }
    for (const auto& method : type.methods) emitField(Pascal(method.name));
    out << "};\n}\n";

    out << "ANITO_SCRIPT_CONFORMS_VALID(anito::" << HandleName(type) << ");\n";
    for (const auto& member : type.members) {
        if (member.settable) {
            out << "ANITO_SCRIPT_CONFORMS_PROPERTY(" << type.engineType << ", " << member.getter << ", "
                << member.setter << ", anito::" << HandleName(type) << ", " << member.name << ", set"
                << Pascal(member.name) << ");\n";
        } else {
            out << "ANITO_SCRIPT_CONFORMS_READ_ONLY_PROPERTY(" << type.engineType << ", " << member.getter
                << ", anito::" << HandleName(type) << ", " << member.name << ");\n";
        }
    }
    for (const auto& method : type.methods)
        out << "ANITO_SCRIPT_CONFORMS_METHOD(" << type.engineType << ", " << method.cppName << ", anito::"
            << HandleName(type) << ", " << method.name << ");\n";
    out << "} // namespace ScriptHost::Bindings::audio\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 3)
            throw std::runtime_error("usage: anito_scriptapigen <generated-sdk-root> <component-source-root>");
        const auto& types = anito::script::RegisteredTypes();
        const auto found = types.find("audiosource");
        if (found == types.end()) throw std::runtime_error("no annotated audiosource type was registered");
        const TypeMetadata& audio = found->second;
        RequireSimpleSignature(audio);

        const std::filesystem::path root(argv[1]);
        const std::filesystem::path componentRoot(argv[2]);
        const auto api = root / "anito" / "generated" / "api";
        const auto host = root / "anito" / "generated" / "host";
        const auto source = root / "generated" / "ScriptHostAPI";
        std::filesystem::create_directories(api);
        std::filesystem::create_directories(host);
        std::filesystem::create_directories(source);
        EmitSdkHandle(audio, api / "AudioSource.hpp");
        EmitHostTable(audio, host / "Audio.hpp");
        EmitBridge(audio, source / "Audio.cpp", componentRoot);
    } catch (const std::exception& error) {
        std::cerr << "script API generation failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
