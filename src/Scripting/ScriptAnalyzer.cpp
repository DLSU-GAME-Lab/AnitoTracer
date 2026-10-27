#include "ScriptAnalyzer.hpp"
#include "ScriptApiMetadata.hpp"

#include <antlr4-runtime.h>

#include <map>
#include <set>
#include <stdexcept>

namespace {

using P = anito_script::AnitoScriptParser;

// Script-level types. Handle types and statics are lowered to calls on the SDK classes in
// sdk/anito/Api.hpp by naming convention: property `foo` -> foo() / setFoo(v), method `bar` -> bar(...).
enum class T {
    Error, Void, Float, Int, Bool, String, Vec3, Quat,
    GameObject, Transform, Rigidbody, Collider, AudioSource, Script, Prefab, RaycastHit,
    // Static API classes: only usable as the base of a member access.
    SMathf, SVector3, SQuaternion, SRandom, STime, SInput, SPhysics, SScene, SCamera, SGui, SDebug
};

// `array<E>` is encoded as kArrayBase + E so it stays a plain comparable enum value.
constexpr int kArrayBase = 100;
bool IsArray(T t) { return static_cast<int>(t) >= kArrayBase; }
T ElementOf(T array) { return static_cast<T>(static_cast<int>(array) - kArrayBase); }
T ArrayOf(T element) { return static_cast<T>(kArrayBase + static_cast<int>(element)); }

bool IsStatic(T t) { return t >= T::SMathf && !IsArray(t); }
bool Storable(T t) { return t != T::Error && t != T::Void && !IsStatic(t); }

// Types that can cross the script-to-script boundary as a Variant.
bool Marshalable(T t) { return t == T::Float || t == T::Int || t == T::Bool || t == T::Vec3 || t == T::String || t == T::GameObject; }

const std::map<std::string, T> kStorableNames = {
    {"float", T::Float}, {"int", T::Int}, {"bool", T::Bool}, {"string", T::String}, {"vec3", T::Vec3},
    {"quat", T::Quat}, {"gameobject", T::GameObject}, {"transform", T::Transform}, {"rigidbody", T::Rigidbody},
    {"collider", T::Collider}, {"audiosource", T::AudioSource}, {"script", T::Script}, {"prefab", T::Prefab},
    {"raycasthit", T::RaycastHit}};

const std::map<std::string, T> kStaticNames = {
    {"Mathf", T::SMathf}, {"Vector3", T::SVector3}, {"Quaternion", T::SQuaternion}, {"Random", T::SRandom},
    {"Time", T::STime}, {"Input", T::SInput}, {"Physics", T::SPhysics}, {"Scene", T::SScene},
    {"Camera", T::SCamera}, {"Gui", T::SGui}, {"Debug", T::SDebug}};

const anito::script::TypeMetadata* FindRegisteredType(T t) {
    for (const auto& [name, type] : kStorableNames) {
        if (type != t) continue;
        const auto& registered = anito::script::RegisteredTypes();
        const auto it = registered.find(name);
        if (it != registered.end()) return &it->second;
    }
    return nullptr;
}

// Handle types that expose valid().
bool HasValid(T t) {
    if (const auto* type = FindRegisteredType(t)) return type->hasValid;
    return t == T::GameObject || t == T::Script;
}

std::string Name(T t) {
    switch (t) {
    case T::Void: return "void";
    case T::Error: return "<error>";
    default: break;
    }
    if (IsArray(t)) return "array<" + Name(ElementOf(t)) + ">";
    for (const auto& [name, type] : kStorableNames) if (type == t) return name;
    for (const auto& [name, type] : kStaticNames) if (type == t) return name;
    return "<error>";
}

std::string Cpp(T t) {
    if (IsArray(t)) return "anito::Array<" + Cpp(ElementOf(t)) + ">";
    if (const auto* type = FindRegisteredType(t)) return type->cppType;
    switch (t) {
    case T::Float: return "float";
    case T::Int: return "int";
    case T::Bool: return "bool";
    case T::String: return "std::string";
    case T::Vec3: return "anito::Vec3";
    case T::Quat: return "anito::Quat";
    case T::GameObject: return "anito::GameObject";
    case T::Script: return "anito::Script";
    case T::Prefab: return "anito::PrefabAsset";
    case T::RaycastHit: return "anito::RaycastHit";
    default: return "void";
    }
}

bool Numeric(T t) { return t == T::Float || t == T::Int; }
bool Convertible(T to, T from) { return to == from || (to == T::Float && from == T::Int); }
bool Equatable(T a, T b) {
    if (Numeric(a) && Numeric(b)) return true;
    if (a != b) return false;
    return a == T::Bool || a == T::String || a == T::Vec3 || a == T::GameObject;
}

T ApiType(const std::string& name) {
    if (name == "void") return T::Void;
    const auto it = kStorableNames.find(name);
    if (it == kStorableNames.end()) throw std::logic_error("unmapped script API type: " + name);
    return it->second;
}

// ---- API surface ------------------------------------------------------------------------------
// Component-backed APIs come from registered metadata; SDK-only APIs remain in the table below.
struct Member {
    std::string name;
    T type;
    bool settable = false;
};

struct Method {
    std::string name;
    T ret = T::Void;
    std::vector<T> params;
    bool variadic = false; // extra marshalable arguments after the listed ones
};

struct TypeInfo {
    std::string cpp;               // class qualifier for static types
    std::vector<Member> members;
    std::vector<Method> methods;
};

const TypeInfo* Info(T t) {
    using Md = Method;
    constexpr T F = T::Float, I = T::Int, B = T::Bool, S = T::String, V3 = T::Vec3, Q = T::Quat, G = T::GameObject,
                Vd = T::Void;
    if (IsArray(t)) {
        // Array<T> members depend on the element type, so each instantiation is built once per thread.
        static thread_local std::map<T, TypeInfo> arrays;
        auto it = arrays.find(t);
        if (it == arrays.end()) {
            const T e = ElementOf(t);
            TypeInfo info{"", {{"count", I}}, {Md{"add", Vd, {e}}, Md{"insert", Vd, {I, e}}, Md{"removeAt", Vd, {I}},
                                               Md{"clear", Vd, {}}, Md{"get", e, {I}}, Md{"contains", B, {e}},
                                               Md{"indexOf", I, {e}}}};
            if (HasValid(e)) info.methods.push_back(Md{"prune", Vd, {}});
            it = arrays.emplace(t, std::move(info)).first;
        }
        return &it->second;
    }
    if (FindRegisteredType(t)) {
        static const std::map<T, TypeInfo> registeredTypes = [] {
            std::map<T, TypeInfo> result;
            for (const auto& [scriptName, metadata] : anito::script::RegisteredTypes()) {
                const auto type = kStorableNames.find(scriptName);
                if (type == kStorableNames.end()) continue;

                TypeInfo info{metadata.cppType, {}, {}};
                for (const auto& member : metadata.members) {
                    info.members.push_back({member.name, ApiType(member.type), member.settable});
                }
                for (const auto& method : metadata.methods) {
                    std::vector<T> parameters;
                    parameters.reserve(method.parameters.size());
                    for (const auto& parameter : method.parameters) parameters.push_back(ApiType(parameter));
                    info.methods.push_back({method.name, ApiType(method.result), std::move(parameters)});
                }
                if (metadata.hasValid) info.methods.push_back({"valid", T::Bool, {}});

                // These are SDK conveniences backed by object IDs rather than component accessors.
                if (metadata.hasValid) info.members.push_back({"gameObject", T::GameObject, false});
                if (scriptName == "transform") {
                    info.members.insert(info.members.end(), {{"forward", T::Vec3}, {"right", T::Vec3}, {"up", T::Vec3},
                                                             {"parent", T::Transform}});
                    info.methods.insert(info.methods.end(), {{"rotate", T::Void, {T::Vec3, T::Float}},
                                                             {"translate", T::Void, {T::Vec3}},
                                                             {"lookAt", T::Void, {T::Vec3, T::Vec3}},
                                                             {"transformPoint", T::Vec3, {T::Vec3}},
                                                             {"transformDirection", T::Vec3, {T::Vec3}},
                                                             {"inverseTransformDirection", T::Vec3, {T::Vec3}}});
                } else if (scriptName == "rigidbody") {
                    info.members.insert(info.members.end(), {{"simulated", T::Bool}, {"rotation", T::Quat}});
                    info.methods.push_back({"lockRotation", T::Void, {T::Bool, T::Bool, T::Bool}});
                }
                result.emplace(type->second, std::move(info));
            }
            return result;
        }();
        const auto it = registeredTypes.find(t);
        return it != registeredTypes.end() ? &it->second : nullptr;
    }
    static const std::map<T, TypeInfo> table = {
        {T::Vec3, {"", {{"length", F}, {"lengthSq", F}, {"normalized", V3}}, {Md{"dot", F, {V3}}, Md{"cross", V3, {V3}}}}},
        {T::Quat, {"", {{"normalized", Q}, {"inverse", Q}}, {Md{"dot", F, {Q}}}}},

        // GameObject / component getters
        {T::GameObject,
         {"",
          {{"name", S, true}, {"transform", T::Transform}, {"rigidbody", T::Rigidbody}, {"collider", T::Collider},
           {"audioSource", T::AudioSource}, {"parent", G, true}, {"root", G}, {"childCount", I}, {"componentCount", I}},
          {Md{"valid", B, {}}, Md{"getScript", T::Script, {S}}, Md{"hasScript", B, {S}}, Md{"addScript", T::Script, {S}},
           Md{"componentName", S, {I}}, Md{"hasComponent", B, {S}}, Md{"addRigidbody", T::Rigidbody, {F}},
           Md{"addBoxCollider", T::Collider, {V3}}, Md{"addSphereCollider", T::Collider, {F}},
           Md{"addCapsuleCollider", T::Collider, {F, F}}, Md{"child", G, {I}}, Md{"findChild", G, {S}},
           Md{"destroy", Vd, {}}, Md{"sendMessage", Vd, {S}, true}}}},

        {T::RaycastHit, {"", {{"hit", B}, {"point", V3}, {"gameObject", G}}, {}}},

        // Scripts / assets / collections
        {T::Script,
         {"",
          {{"gameObject", G}},
          {Md{"valid", B, {}}, Md{"call", Vd, {S}, true}, Md{"callFloat", F, {S}, true}, Md{"callInt", I, {S}, true},
           Md{"callBool", B, {S}, true}, Md{"callVec3", V3, {S}, true}, Md{"callObject", G, {S}, true}}}},
        {T::Prefab, {"", {}, {Md{"empty", B, {}}}}},

        // Statics
        {T::SMathf,
         {"anito::Mathf",
          {{"pi", F}, {"deg2rad", F}, {"rad2deg", F}, {"epsilon", F}},
          {Md{"clamp", F, {F, F, F}}, Md{"clamp01", F, {F}}, Md{"lerp", F, {F, F, F}}, Md{"sign", F, {F}},
           Md{"sin", F, {F}}, Md{"cos", F, {F}}, Md{"tan", F, {F}}, Md{"asin", F, {F}}, Md{"acos", F, {F}},
           Md{"atan", F, {F}}, Md{"atan2", F, {F, F}}, Md{"sqrt", F, {F}}, Md{"cbrt", F, {F}}, Md{"pow", F, {F, F}},
           Md{"exp", F, {F}}, Md{"abs", F, {F}}, Md{"floor", F, {F}}, Md{"ceil", F, {F}}, Md{"round", F, {F}},
           Md{"min", F, {F, F}}, Md{"max", F, {F, F}}, Md{"moveTowards", F, {F, F, F}}}}},
        {T::SVector3,
         {"anito::Vector3",
          {{"zero", V3}, {"one", V3}, {"up", V3}, {"forward", V3}, {"right", V3}},
          {Md{"dot", F, {V3, V3}}, Md{"cross", V3, {V3, V3}}, Md{"length", F, {V3}}, Md{"distance", F, {V3, V3}},
           Md{"normalize", V3, {V3}}, Md{"lerp", V3, {V3, V3, F}}, Md{"scale", V3, {V3, V3}}}}},
        {T::SQuaternion,
         {"anito::Quaternion",
          {{"identity", Q}},
          {Md{"dot", F, {Q, Q}}, Md{"angleAxis", Q, {F, V3}}, Md{"euler", Q, {V3}}, Md{"lookRotation", Q, {V3, V3}},
           Md{"slerp", Q, {Q, Q, F}}}}},
        {T::SRandom,
         {"anito::Random",
          {},
          {Md{"value", F, {}}, Md{"range", F, {F, F}}, Md{"onUnitSphere", V3, {}}, Md{"insideUnitSphere", V3, {}},
           Md{"rotation", Q, {}}}}},
        {T::STime, {"anito::Time", {{"time", F}, {"deltaTime", F}, {"fixedDeltaTime", F}}, {}}},
        {T::SInput,
         {"anito::Input", {{"mouseDelta", V3}, {"mousePosition", V3}}, {Md{"getAction", B, {S}}}}},
        {T::SPhysics,
         {"anito::Physics", {{"gravity", V3, true}}, {Md{"raycast", T::RaycastHit, {V3, V3, F, G}}}}},
        {T::SScene,
         {"anito::Scene",
          {{"none", G}},
          {Md{"createEmpty", G, {S, G}}, Md{"createPrimitive", G, {S, G}}, Md{"instantiate", G, {T::Prefab, G}},
           Md{"find", G, {S}}, Md{"findWithScript", G, {S}}}}},
        {T::SCamera, {"anito::Camera", {{"main", G}}, {}}},
        {T::SGui,
         {"anito::Gui",
          {},
          {Md{"worldToScreen", V3, {V3}}, Md{"fillRect", Vd, {V3, V3, V3, F}}, Md{"drawRect", Vd, {V3, V3, V3, F, F}}}}},
        {T::SDebug, {"anito::Debug", {}, {Md{"log", Vd, {S}}}}},
    };
    const auto it = table.find(t);
    return it != table.end() ? &it->second : nullptr;
}

const Member* FindMember(T t, const std::string& name) {
    if (const TypeInfo* info = Info(t)) {
        for (const Member& m : info->members) if (m.name == name) return &m;
    }
    return nullptr;
}

const Method* FindMethod(T t, const std::string& name) {
    if (const TypeInfo* info = Info(t)) {
        for (const Method& m : info->methods) if (m.name == name) return &m;
    }
    return nullptr;
}

std::string SetterName(const std::string& property) {
    std::string out = "set" + property;
    out[3] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[3])));
    return out;
}

// Analysis carries a source type alongside its emitted C++ expression, avoiding a second AST pass.
struct V {
    T t = T::Error;
    std::string c;
};

struct MethodSig {
    T ret = T::Void;
    std::vector<T> params;
};

// Event handlers the runtime can dispatch: parameter types and the SDK virtual they map to.
struct HandlerInfo {
    std::vector<T> params;
    const char* cppName;
};
const std::map<std::string, HandlerInfo> kHandlers = {
    {"Start", {{}, "OnStart"}},
    {"Update", {{T::Float}, "OnUpdate"}},
    {"FixedUpdate", {{T::Float}, "OnFixedUpdate"}},
    {"GUI", {{T::Float}, "OnGUI"}},
    {"CollisionEnter", {{T::GameObject}, "OnCollisionEnter"}},
};

// Names that would collide with C++ or with the generated class's own members.
const std::set<std::string> kReserved = {
    "alignas", "alignof", "and", "asm", "auto", "bitand", "bitor", "case", "catch", "char", "class", "compl",
    "concept", "const", "constexpr", "const_cast", "decltype", "default", "delete", "do", "double", "dynamic_cast",
    "enum", "explicit", "export", "extern", "friend", "goto", "inline", "long", "mutable", "namespace", "new",
    "noexcept", "not", "nullptr", "operator", "or", "private", "protected", "public", "register", "short",
    "signed", "sizeof", "static", "struct", "switch", "template", "this", "throw", "try", "typedef", "typeid",
    "typename", "union", "unsigned", "using", "virtual", "volatile", "xor", "std", "anito", "user", "ctx",
    "m_ctx", "gameObject", "OnStart", "OnUpdate", "OnFixedUpdate", "OnGUI", "OnCollisionEnter", "Invoke", "Call",
    "method", "args", "argc", "ret", "owner"};

bool IsReservedName(const std::string& name) {
    if (kReserved.count(name) || kStorableNames.count(name) || kStaticNames.count(name)) return true;
    for (const auto& [scriptName, metadata] : anito::script::RegisteredTypes()) {
        if (scriptName == name || metadata.cppType == name) return true;
    }
    return false;
}

struct Lvalue {
    std::string pre, ref, post;
    std::string setter; // set when the target is a whole property: a plain `=` lowers to `setter(rhs)`
    std::string rhs;
    bool plain = false;
};

class Analyzer {
public:
    Analyzer(const std::filesystem::path& path, std::vector<std::string>& errors)
        : m_path(path), m_errors(errors), m_pathText(path.generic_string()) {}

    std::map<std::string, std::string> Run(P::ScriptContext* script) {
        const size_t before = m_errors.size();
        std::set<std::string> seen;
        std::map<std::string, std::string> result;
        // A semantic error invalidates the compilation unit, so callers never build a partial module.
        for (auto* component : script->componentDecl()) {
            CheckName(component, component->name->getText());
            if (!seen.insert(component->name->getText()).second) {
                Report(component, "duplicate component '" + component->name->getText() + "'");
            }
            result[component->name->getText()] = AnalyzeComponent(component);
        }
        if (m_errors.size() != before) result.clear();
        return result;
    }

private:
    void Report(antlr4::ParserRuleContext* ctx, const std::string& message) { ReportAt(ctx->getStart(), message); }

    void ReportAt(antlr4::Token* token, const std::string& message) {
        if (m_quiet > 0) return;
        m_errors.push_back(m_path.string() + "(" + std::to_string(token->getLine()) + ":" +
                           std::to_string(token->getCharPositionInLine()) + "): " + message);
    }

    void CheckName(antlr4::ParserRuleContext* ctx, const std::string& name) {
        if (IsReservedName(name) || name.front() == '_') Report(ctx, "'" + name + "' is a reserved name");
    }

    // ---- output ----

    void Emit(const std::string& line) { *m_out += std::string(m_indent * 4, ' ') + line + "\n"; }

    // Maps compiler errors in the generated C++ back to the .ascript source.
    void Directive(antlr4::ParserRuleContext* ctx) {
        *m_out += "#line " + std::to_string(ctx->getStart()->getLine()) + " \"" + m_pathText + "\"\n";
    }

    // ---- declarations ----

    T ResolveType(P::TypeContext* type, bool allowVoid) {
        if (type->elem) {
            if (type->ID()->getText() != "array") {
                Report(type, "unknown generic type '" + type->ID()->getText() + "'");
                return T::Error;
            }
            const T element = ResolveType(type->elem, false);
            if (element == T::Error) return T::Error;
            if (IsArray(element)) {
                Report(type, "arrays of arrays are not supported");
                return T::Error;
            }
            return ArrayOf(element);
        }
        const std::string text = type->getText();
        const auto it = kStorableNames.find(text);
        if (it != kStorableNames.end()) return it->second;
        if (text == "void" && allowVoid) return T::Void;
        Report(type, "unknown type '" + text + "'");
        return T::Error;
    }

    static bool IsExposed(P::FieldDeclContext* field) {
        for (auto* annotation : field->annotation()) {
            if (annotation->name && annotation->name->getText() == "field") return true;
        }
        return false;
    }

    static bool IsMethodExposed(P::MethodDeclContext* method) {
        for (auto* annotation : method->annotation()) {
            if (annotation->name && annotation->name->getText() == "method") return true;
        }
        return false;
    }

    std::string AnalyzeComponent(P::ComponentDeclContext* component) {
        m_fields.clear();
        m_methods.clear();
        m_refFields.clear();

        const auto ids = component->ID();
        for (size_t i = 1; i < ids.size(); ++i) {
            if (!kHandlers.count(ids[i]->getText())) {
                ReportAt(ids[i]->getSymbol(), "unknown trigger '" + ids[i]->getText() + "'");
            }
        }

        // Collect signatures before bodies so fields and methods can be referenced regardless of order.
        std::set<std::string> handlers;
        for (auto* member : component->member()) {
            if (auto* field = member->fieldDecl()) {
                const std::string name = field->name->getText();
                CheckName(field, name);
                const T type = ResolveType(field->type(), false);
                if (!m_fields.emplace(name, type).second) Report(field, "duplicate field '" + name + "'");
            } else if (auto* method = member->methodDecl()) {
                const std::string name = method->name->getText();
                CheckName(method, name);
                MethodSig sig;
                sig.ret = method->type() ? ResolveType(method->type(), true) : T::Void;
                if (method->params()) {
                    for (auto* param : method->params()->param()) {
                        if (!param->type()) {
                            Report(param, "parameter '" + param->name->getText() + "' needs a type");
                            sig.params.push_back(T::Error);
                        } else {
                            sig.params.push_back(ResolveType(param->type(), false));
                        }
                    }
                }
                if (m_fields.count(name) || !m_methods.emplace(name, sig).second) {
                    Report(method, "duplicate member '" + name + "'");
                }
            } else if (auto* handler = member->eventHandler()) {
                if (!handlers.insert(handler->name->getText()).second) {
                    Report(handler, "duplicate handler 'on " + handler->name->getText() + "'");
                }
            }
        }

        // Separate output buffers let class layout follow C++ declaration order requirements while
        // retaining source-order processing for diagnostics and method bodies.
        std::string members;   // handlers and methods
        std::string data;      // field declarations
        std::string init;      // constructor initialisers for exposed fields
        std::string invoke;    // @method dispatch cases
        std::string call;      // script-to-script dispatch cases (all callable methods)
        size_t exposedIndex = 0;
        m_indent = 1;

        for (auto* member : component->member()) {
            if (auto* field = member->fieldDecl()) {
                m_out = &data;
                EmitField(field, exposedIndex, init);
            } else if (auto* method = member->methodDecl()) {
                m_out = &members;
                EmitMethod(method, invoke, call);
            } else if (auto* handler = member->eventHandler()) {
                m_out = &members;
                EmitHandler(handler);
            }
        }

        const std::string cls = component->name->getText();
        std::string out;
        // The generated type is a thin SDK behaviour: editor fields are references, while private
        // state belongs to the native instance and is recreated when the module reloads.
        out += "class " + cls + " final : public anito::ScriptBehaviour {\n";
        out += "public:\n";
        out += "    explicit " + cls + "(const anito::ScriptContext& ctx)\n";
        out += "        : anito::ScriptBehaviour(ctx)" + init + " {}\n\n";
        out += members;
        out += "    bool Invoke(const char* method) override {\n        (void)method;\n" + invoke + "        return false;\n    }\n\n";
        out += "    bool Call(const char* method, const anito::Variant* args, uint32_t argc, anito::Variant* ret) override {\n";
        out += "        (void)method; (void)args; (void)argc; (void)ret;\n" + call + "        return false;\n    }\n\n";
        out += "private:\n" + data + "};\n";
        return out;
    }

    void EmitField(P::FieldDeclContext* field, size_t& exposedIndex, std::string& init) {
        const std::string name = field->name->getText();
        const T declared = m_fields[name];
        if (declared == T::Error) return;

        if (IsExposed(field)) {
            // Exposed fields live in host storage so the Inspector and serializer own the values;
            // declaration order must match the descriptor and module field table.
            const std::string slot = "ctx.fields[" + std::to_string(exposedIndex++) + "]";
            if (declared == T::Prefab || declared == T::Transform) {
                // Reference slots are resolved on every read so inspector edits and destroyed targets are seen.
                init += ",\n          _f_" + name + "(" + slot + ")";
                Emit("void* _f_" + name + ";");
                m_refFields.insert(name);
            } else {
                init += ",\n          " + name + "(*static_cast<" + Cpp(declared) + "*>(" + slot + "))";
                Emit(Cpp(declared) + "& " + name + ";");
            }
            return;
        }

        std::string value = "{}";
        if (field->expr()) {
            m_scopes.clear();
            const V v = Check(field->expr());
            if (v.t != T::Error && !Convertible(declared, v.t)) {
                Report(field, "cannot initialize " + Name(declared) + " field '" + name + "' with " + Name(v.t));
            }
            value = "= " + v.c;
        }
        Emit(Cpp(declared) + " " + name + " " + (value == "{}" ? "{}" : value) + ";");
    }

    void EmitMethod(P::MethodDeclContext* method, std::string& invoke, std::string& call) {
        const std::string name = method->name->getText();
        const MethodSig& sig = m_methods[name];
        std::vector<std::pair<std::string, T>> params;
        std::string list;
        if (method->params()) {
            size_t i = 0;
            for (auto* param : method->params()->param()) {
                const std::string pname = param->name->getText();
                CheckName(param, pname);
                params.emplace_back(pname, sig.params[i]);
                if (!list.empty()) list += ", ";
                list += Cpp(sig.params[i]) + " " + pname;
                ++i;
            }
        }
        if (IsMethodExposed(method) && params.empty()) {
            invoke += "        if (std::strcmp(method, \"" + name + "\") == 0) { " + name + "(); return true; }\n";
        }
        EmitCallCase(name, sig, call);
        EmitBody(method, method->block(), Cpp(sig.ret) + " " + name + "(" + list + ")", params, sig.ret);
    }

    // Methods whose signature fits in Variants are callable from other scripts via Script.call / sendMessage.
    static void EmitCallCase(const std::string& name, const MethodSig& sig, std::string& call) {
        if (sig.ret != T::Void && !Marshalable(sig.ret)) return;
        if (sig.ret == T::String) return; // Variant strings borrow memory; returning one would dangle
        std::string args;
        for (size_t i = 0; i < sig.params.size(); ++i) {
            if (!Marshalable(sig.params[i])) return;
            const std::string slot = "args[" + std::to_string(i) + "]";
            std::string value;
            switch (sig.params[i]) {
            case T::Float: value = slot + ".AsFloat()"; break;
            case T::Int: value = slot + ".AsInt()"; break;
            case T::Bool: value = slot + ".AsBool()"; break;
            case T::Vec3: value = slot + ".AsVec3()"; break;
            case T::String: value = "std::string(" + slot + ".AsString())"; break;
            default: value = "anito::GameObject{" + slot + ".AsObject()}"; break;
            }
            if (!args.empty()) args += ", ";
            args += value;
        }
        const std::string invocation = name + "(" + args + ")";
        std::string body;
        switch (sig.ret) {
        case T::Void: body = invocation + ";"; break;
        case T::GameObject: body = "if (ret) *ret = anito::Variant::OfObject(" + invocation + ".id);"; break;
        default: body = "if (ret) *ret = anito::Variant::Of(" + invocation + ");"; break;
        }
        call += "        if (std::strcmp(method, \"" + name + "\") == 0 && argc == " + std::to_string(sig.params.size()) +
                ") { " + body + " return true; }\n";
    }

    void EmitHandler(P::EventHandlerContext* handler) {
        std::vector<std::pair<std::string, T>> params;
        std::string list;
        std::string cppName = "OnUnknown";
        // Handler names/signatures are a fixed runtime contract, mapped to SDK virtual callbacks.
        const auto it = kHandlers.find(handler->name->getText());
        if (it == kHandlers.end()) {
            ReportAt(handler->name, "unknown event handler '" + handler->name->getText() + "'");
        } else {
            cppName = it->second.cppName;
            const auto declared = handler->params() ? handler->params()->param() : std::vector<P::ParamContext*>{};
            if (declared.size() != it->second.params.size()) {
                Report(handler, "'on " + it->first + "' takes " + std::to_string(it->second.params.size()) + " parameter(s)");
            } else {
                for (size_t i = 0; i < declared.size(); ++i) {
                    const std::string pname = declared[i]->name->getText();
                    CheckName(declared[i], pname);
                    if (declared[i]->type() && ResolveType(declared[i]->type(), false) != it->second.params[i]) {
                        Report(declared[i], "parameter '" + pname + "' must be " + Name(it->second.params[i]));
                    }
                    params.emplace_back(pname, it->second.params[i]);
                    if (!list.empty()) list += ", ";
                    list += Cpp(it->second.params[i]) + " " + pname;
                }
            }
        }
        EmitBody(handler, handler->block(), "void " + cppName + "(" + list + ") override", params, T::Void);
    }

    void EmitBody(antlr4::ParserRuleContext* owner, P::BlockContext* block, const std::string& signature,
                  const std::vector<std::pair<std::string, T>>& params, T ret) {
        m_ret = ret;
        m_loopDepth = 0;
        m_scopes.clear();
        m_scopes.emplace_back();
        for (const auto& [name, type] : params) {
            if (!m_scopes.back().emplace(name, type).second) Report(block, "duplicate parameter '" + name + "'");
        }

        Directive(owner);
        Emit(signature + " {");
        ++m_indent;
        for (const auto& [name, type] : params) Emit("(void)" + name + ";");
        for (auto* stmt : block->stmt()) CheckStmt(stmt);
        --m_indent;
        Emit("}");
        *m_out += "\n";
        m_scopes.pop_back();
    }

    // ---- scopes ----

    void Declare(const std::string& name, T type, antlr4::ParserRuleContext* ctx) {
        CheckName(ctx, name);
        if (!m_scopes.back().emplace(name, type).second) Report(ctx, "'" + name + "' is already declared in this scope");
    }

    bool Lookup(const std::string& name, T& out) const {
        for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
            const auto found = it->find(name);
            if (found != it->end()) { out = found->second; return true; }
        }
        const auto field = m_fields.find(name);
        if (field != m_fields.end()) { out = field->second; return true; }
        if (name == "owner") { out = T::GameObject; return true; }
        const auto stat = kStaticNames.find(name);
        if (stat != kStaticNames.end()) { out = stat->second; return true; }
        return false;
    }

    // ---- statements ----

    // Returns `Type name = init` for let statements and for-loop initialisers.
    std::string CheckLet(antlr4::ParserRuleContext* ctx, antlr4::Token* nameToken, P::TypeContext* type, P::ExprContext* init) {
        const V value = Check(init);
        T result = value.t;
        if (value.t == T::Void || IsStatic(value.t)) {
            Report(ctx, "cannot store a " + Name(value.t) + " value");
            result = T::Error;
        }
        if (type) {
            const T declared = ResolveType(type, false);
            if (declared != T::Error && result != T::Error && !Convertible(declared, result)) {
                Report(ctx, "cannot initialize " + Name(declared) + " '" + nameToken->getText() + "' with " + Name(result));
            }
            result = declared;
        }
        Declare(nameToken->getText(), result, ctx);
        return Cpp(result) + " " + nameToken->getText() + " = " + value.c;
    }

    std::string CheckCondition(P::ExprContext* expr) {
        const V v = Check(expr);
        if (v.t != T::Error && v.t != T::Bool) Report(expr, std::string("condition must be bool, got ") + Name(v.t));
        return v.c;
    }

    void EmitScoped(P::StmtContext* stmt) {
        m_scopes.emplace_back();
        ++m_indent;
        CheckStmt(stmt);
        --m_indent;
        m_scopes.pop_back();
    }

    // Each statement is validated as it is lowered, keeping diagnostics tied to original tokens.
    void CheckStmt(P::StmtContext* stmt) {
        if (auto* s = dynamic_cast<P::BlockStmtContext*>(stmt)) {
            Emit("{");
            m_scopes.emplace_back();
            ++m_indent;
            for (auto* inner : s->block()->stmt()) CheckStmt(inner);
            --m_indent;
            m_scopes.pop_back();
            Emit("}");
        } else if (auto* s = dynamic_cast<P::LetStmtContext*>(stmt)) {
            Directive(s);
            Emit(CheckLet(s, s->name, s->type(), s->expr()) + ";");
        } else if (auto* s = dynamic_cast<P::IfStmtContext*>(stmt)) {
            Directive(s);
            Emit("if (" + CheckCondition(s->expr()) + ") {");
            const auto branches = s->stmt();
            EmitScoped(branches[0]);
            if (branches.size() > 1) {
                Emit("} else {");
                EmitScoped(branches[1]);
            }
            Emit("}");
        } else if (auto* s = dynamic_cast<P::WhileStmtContext*>(stmt)) {
            Directive(s);
            Emit("while (" + CheckCondition(s->expr()) + ") {");
            ++m_loopDepth;
            EmitScoped(s->stmt());
            --m_loopDepth;
            Emit("}");
        } else if (auto* s = dynamic_cast<P::ForStmtContext*>(stmt)) {
            m_scopes.emplace_back();
            std::string init, cond, step;
            if (auto* forInit = s->forInit()) {
                init = forInit->assignment() ? ForAssignment(forInit->assignment())
                                             : CheckLet(forInit, forInit->name, forInit->type(), forInit->expr());
            }
            if (s->expr()) cond = CheckCondition(s->expr());
            if (s->assignment()) step = ForAssignment(s->assignment());
            Directive(s);
            Emit("for (" + init + "; " + cond + "; " + step + ") {");
            ++m_loopDepth;
            EmitScoped(s->stmt());
            --m_loopDepth;
            Emit("}");
            m_scopes.pop_back();
        } else if (auto* s = dynamic_cast<P::ReturnStmtContext*>(stmt)) {
            Directive(s);
            if (s->expr()) {
                const V v = Check(s->expr());
                if (m_ret == T::Void) Report(s, "a void function cannot return a value");
                else if (v.t != T::Error && !Convertible(m_ret, v.t)) Report(s, std::string("cannot return ") + Name(v.t) + " from a function returning " + Name(m_ret));
                Emit("return " + v.c + ";");
            } else {
                if (m_ret != T::Void && m_ret != T::Error) Report(s, std::string("must return a ") + Name(m_ret));
                Emit("return;");
            }
        } else if (auto* s = dynamic_cast<P::BreakStmtContext*>(stmt)) {
            if (m_loopDepth == 0) Report(s, "'break' outside of a loop");
            Directive(s);
            Emit("break;");
        } else if (auto* s = dynamic_cast<P::ContinueStmtContext*>(stmt)) {
            if (m_loopDepth == 0) Report(s, "'continue' outside of a loop");
            Directive(s);
            Emit("continue;");
        } else if (auto* s = dynamic_cast<P::AssignStmtContext*>(stmt)) {
            Directive(s);
            EmitAssignment(s->assignment());
        } else if (auto* s = dynamic_cast<P::ExprStmtContext*>(stmt)) {
            Directive(s);
            const V v = Check(s->expr());
            if (!v.c.empty()) Emit(v.c + ";");
        }
    }

    bool IsLvalue(P::ExprContext* expr) {
        if (auto* primary = dynamic_cast<P::PrimaryExprContext*>(expr)) {
            auto* id = dynamic_cast<P::IdentifierContext*>(primary->primary());
            if (!id) return false;
            const std::string name = id->ID()->getText();
            return name != "owner" && !kStaticNames.count(name) && !m_refFields.count(name);
        }
        if (auto* index = dynamic_cast<P::IndexExprContext*>(expr)) {
            // Array elements are assignable references, as long as the array itself is a variable.
            return IsLvalue(index->expr()[0]);
        }
        if (auto* member = dynamic_cast<P::MemberExprContext*>(expr)) {
            ++m_quiet;
            const T base = Check(member->expr()).t;
            --m_quiet;
            const std::string name = member->name->getText();
            if (base == T::Vec3) return (name == "x" || name == "y" || name == "z") && IsLvalue(member->expr());
            const Member* m = FindMember(base, name);
            return m && m->settable;
        }
        return false;
    }

    // Properties are assignable through setX(); compound and component writes stage a temporary.
    Lvalue ToLvalue(P::ExprContext* expr) {
        if (auto* primary = dynamic_cast<P::PrimaryExprContext*>(expr)) {
            Lvalue out;
            out.ref = dynamic_cast<P::IdentifierContext*>(primary->primary())->ID()->getText();
            return out;
        }
        if (auto* index = dynamic_cast<P::IndexExprContext*>(expr)) {
            Lvalue out;
            out.ref = Check(index).c;
            return out;
        }
        auto* member = dynamic_cast<P::MemberExprContext*>(expr);
        ++m_quiet;
        const V base = Check(member->expr());
        --m_quiet;
        const std::string name = member->name->getText();
        if (base.t == T::Vec3) {
            Lvalue inner = ToLvalue(member->expr());
            inner.ref += "." + name;
            inner.setter.clear();
            return inner;
        }
        const Member* m = FindMember(base.t, name);
        const std::string sep = IsStatic(base.t) ? "::" : ".";
        const std::string tmp = "_t" + std::to_string(m_tmp++);
        Lvalue out;
        out.setter = base.c + sep + SetterName(name);
        out.pre = Cpp(m->type) + " " + tmp + " = " + base.c + sep + name + "();";
        out.ref = tmp;
        out.post = out.setter + "(" + tmp + ");";
        return out;
    }

    // Returns `ref op rhs` and the staging statements, or false if the assignment is invalid.
    bool BuildAssignment(P::AssignmentContext* assignment, Lvalue& target, std::string& expression) {
        const auto sides = assignment->expr();
        const V left = Check(sides[0]);
        const V right = Check(sides[1]);
        if (!IsLvalue(sides[0])) {
            Report(assignment, "left side of assignment cannot be assigned to");
            return false;
        }
        if (left.t == T::Error || right.t == T::Error) return false;

        const size_t op = assignment->op->getType();
        std::string rhs = right.c;
        if (op == P::ASSIGN) {
            if (!Convertible(left.t, right.t)) {
                Report(assignment, "cannot assign " + Name(right.t) + " to " + Name(left.t));
                return false;
            }
            if (left.t == T::Float && right.t == T::Int) rhs = "static_cast<float>(" + rhs + ")";
        } else {
            const char* symbol = op == P::PLUS_ASSIGN ? "+" : op == P::MINUS_ASSIGN ? "-" : op == P::STAR_ASSIGN ? "*" : "/";
            const V result = Arith(symbol, left, right, assignment);
            if (result.t == T::Error) return false;
            if (!Convertible(left.t, result.t)) {
                Report(assignment, "cannot assign " + Name(result.t) + " to " + Name(left.t));
                return false;
            }
        }
        target = ToLvalue(sides[0]);
        target.plain = op == P::ASSIGN;
        target.rhs = rhs;
        if (op != P::ASSIGN && left.t == T::Quat) {
            const std::string symbol = assignment->op->getText().substr(0, 1);
            expression = target.ref + " = (" + target.ref + " " + symbol + " " + rhs + ")";
        } else {
            expression = target.ref + " " + assignment->op->getText() + " " + rhs;
        }
        return true;
    }

    void EmitAssignment(P::AssignmentContext* assignment) {
        Lvalue target;
        std::string expression;
        if (!BuildAssignment(assignment, target, expression)) return;
        if (target.plain && !target.setter.empty()) {
            Emit(target.setter + "(" + target.rhs + ");");
        } else if (target.pre.empty()) {
            Emit(expression + ";");
        } else {
            Emit("{ " + target.pre + " " + expression + "; " + target.post + " }");
        }
    }

    std::string ForAssignment(P::AssignmentContext* assignment) {
        Lvalue target;
        std::string expression;
        if (!BuildAssignment(assignment, target, expression)) return "";
        if (target.plain && !target.setter.empty()) return target.setter + "(" + target.rhs + ")";
        if (!target.pre.empty()) {
            Report(assignment, "assigning to a property component is not supported in a for header");
            return "";
        }
        return expression;
    }

    // ---- expressions ----

    V Arith(const std::string& op, const V& l, const V& r, antlr4::ParserRuleContext* ctx) {
        if (l.t == T::Error || r.t == T::Error) return {};
        const std::string plain = "(" + l.c + " " + op + " " + r.c + ")";
        if (Numeric(l.t) && Numeric(r.t)) {
            const T result = (l.t == T::Float || r.t == T::Float) ? T::Float : T::Int;
            if (op == "%" && result == T::Float) return {result, "std::fmod(" + l.c + ", " + r.c + ")"};
            return {result, plain};
        }
        if (l.t == T::Vec3 && r.t == T::Vec3 && (op == "+" || op == "-")) return {T::Vec3, plain};
        if (op == "*" && ((l.t == T::Vec3 && Numeric(r.t)) || (Numeric(l.t) && r.t == T::Vec3))) return {T::Vec3, plain};
        if (op == "/" && l.t == T::Vec3 && Numeric(r.t)) return {T::Vec3, plain};
        if (op == "+" && l.t == T::String && r.t == T::String) return {T::String, plain};
        if (op == "*" && l.t == T::Quat && r.t == T::Quat) return {T::Quat, plain};
        if (op == "*" && l.t == T::Quat && r.t == T::Vec3) return {T::Vec3, plain};
        Report(ctx, "operator '" + op + "' cannot be applied to " + Name(l.t) + " and " + Name(r.t));
        return {};
    }

    // Expression checking is also code generation: each successful result carries its C++ form.
    V Check(P::ExprContext* expr) {
        if (auto* e = dynamic_cast<P::PrimaryExprContext*>(expr)) return CheckPrimary(e->primary());
        if (auto* e = dynamic_cast<P::MemberExprContext*>(expr)) return CheckMember(e);
        if (auto* e = dynamic_cast<P::CallExprContext*>(expr)) return CheckCall(e);
        if (auto* e = dynamic_cast<P::IndexExprContext*>(expr)) {
            const auto s = e->expr();
            const V base = Check(s[0]);
            const V index = Check(s[1]);
            if (base.t == T::Error || index.t == T::Error) return {};
            if (!IsArray(base.t)) {
                Report(e, "only arrays can be indexed, got " + Name(base.t));
                return {};
            }
            if (index.t != T::Int) {
                Report(e, "array index must be int, got " + Name(index.t));
                return {};
            }
            return {ElementOf(base.t), base.c + "[" + index.c + "]"};
        }
        if (auto* e = dynamic_cast<P::UnaryExprContext*>(expr)) {
            const V v = Check(e->expr());
            if (v.t == T::Error) return {};
            if (e->op->getType() == P::BANG) {
                if (v.t == T::Bool) return {T::Bool, "(!" + v.c + ")"};
                Report(e, std::string("'!' needs bool, got ") + Name(v.t));
            } else {
                if (Numeric(v.t) || v.t == T::Vec3) return {v.t, "(-" + v.c + ")"};
                Report(e, std::string("unary '-' needs a number or vec3, got ") + Name(v.t));
            }
            return {};
        }
        if (auto* e = dynamic_cast<P::MulExprContext*>(expr)) {
            const auto s = e->expr();
            return Arith(e->op->getText(), Check(s[0]), Check(s[1]), e);
        }
        if (auto* e = dynamic_cast<P::AddExprContext*>(expr)) {
            const auto s = e->expr();
            return Arith(e->op->getText(), Check(s[0]), Check(s[1]), e);
        }
        if (auto* e = dynamic_cast<P::RelExprContext*>(expr)) {
            const auto s = e->expr();
            const V l = Check(s[0]);
            const V r = Check(s[1]);
            if (l.t != T::Error && r.t != T::Error && !(Numeric(l.t) && Numeric(r.t))) {
                Report(e, "operator '" + e->op->getText() + "' needs numbers, got " + Name(l.t) + " and " + Name(r.t));
            }
            return {T::Bool, "(" + l.c + " " + e->op->getText() + " " + r.c + ")"};
        }
        if (auto* e = dynamic_cast<P::EqExprContext*>(expr)) {
            const auto s = e->expr();
            const V l = Check(s[0]);
            const V r = Check(s[1]);
            if (l.t != T::Error && r.t != T::Error && !(Equatable(l.t, r.t))) {
                Report(e, std::string("cannot compare ") + Name(l.t) + " with " + Name(r.t));
            }
            return {T::Bool, "(" + l.c + " " + e->op->getText() + " " + r.c + ")"};
        }
        if (auto* e = dynamic_cast<P::AndExprContext*>(expr)) return CheckLogic(e->expr(), e, "&&");
        if (auto* e = dynamic_cast<P::OrExprContext*>(expr)) return CheckLogic(e->expr(), e, "||");
        if (auto* e = dynamic_cast<P::TernaryExprContext*>(expr)) {
            const auto s = e->expr();
            const std::string cond = CheckCondition(s[0]);
            const V a = Check(s[1]);
            const V b = Check(s[2]);
            if (a.t == T::Error || b.t == T::Error) return {};
            T result = a.t;
            if (a.t != b.t) {
                if (Numeric(a.t) && Numeric(b.t)) {
                    result = T::Float;
                } else {
                    Report(e, std::string("branches of '?:' have different types: ") + Name(a.t) + " and " + Name(b.t));
                    return {};
                }
            }
            return {result, "(" + cond + " ? " + a.c + " : " + b.c + ")"};
        }
        return {};
    }

    V CheckLogic(const std::vector<P::ExprContext*>& sides, antlr4::ParserRuleContext* ctx, const char* op) {
        const V l = Check(sides[0]);
        const V r = Check(sides[1]);
        if (l.t != T::Error && r.t != T::Error && (l.t != T::Bool || r.t != T::Bool)) {
            Report(ctx, std::string("operator '") + op + "' needs bool operands, got " + Name(l.t) + " and " + Name(r.t));
        }
        return {T::Bool, "(" + l.c + " " + op + " " + r.c + ")"};
    }

    V CheckPrimary(P::PrimaryContext* primary) {
        if (auto* lit = dynamic_cast<P::IntLiteralContext*>(primary)) return {T::Int, lit->INT_LIT()->getText()};
        if (auto* lit = dynamic_cast<P::FloatLiteralContext*>(primary)) return {T::Float, lit->FLOAT_LIT()->getText() + "f"};
        if (auto* lit = dynamic_cast<P::StringLiteralContext*>(primary)) {
            return {T::String, "std::string(" + lit->STRING_LIT()->getText() + ")"};
        }
        if (dynamic_cast<P::TrueLiteralContext*>(primary)) return {T::Bool, "true"};
        if (dynamic_cast<P::FalseLiteralContext*>(primary)) return {T::Bool, "false"};
        if (auto* paren = dynamic_cast<P::ParenExprContext*>(primary)) {
            V inner = Check(paren->expr());
            inner.c = "(" + inner.c + ")";
            return inner;
        }
        if (auto* id = dynamic_cast<P::IdentifierContext*>(primary)) {
            const std::string name = id->ID()->getText();
            T type = T::Error;
            if (Lookup(name, type)) {
                if (name == "owner") return {type, "gameObject"};
                if (const auto stat = kStaticNames.find(name); stat != kStaticNames.end() && !m_fields.count(name)) {
                    return {type, Info(type)->cpp};
                }
                if (m_refFields.count(name)) {
                    return {type, std::string(type == T::Prefab ? "anito::ResolvePrefabField(" : "anito::ResolveTransformField(") + "_f_" + name + ")"};
                }
                return {type, name};
            }
            if (m_methods.count(name)) {
                Report(primary, "'" + name + "' is a method; call it with ()");
            } else {
                Report(primary, "undefined name '" + name + "'");
            }
            return {};
        }
        Report(primary, "type name used as a value");
        return {};
    }

    V CheckMember(P::MemberExprContext* member) {
        const V base = Check(member->expr());
        if (base.t == T::Error) return {};
        const std::string name = member->name->getText();
        if (base.t == T::Vec3 && (name == "x" || name == "y" || name == "z")) return {T::Float, base.c + "." + name};
        if (const Member* m = FindMember(base.t, name)) {
            return {m->type, base.c + (IsStatic(base.t) ? "::" : ".") + name + "()"};
        }
        if (FindMethod(base.t, name)) Report(member, "'" + name + "' is a method; call it with ()");
        else Report(member, Name(base.t) + " has no member '" + name + "'");
        return {};
    }

    // Checks argument count and types; on success returns the C++ argument list.
    bool ExpectArgs(const std::string& callee, const std::vector<T>& params, const std::vector<V>& args,
                    antlr4::ParserRuleContext* ctx) {
        if (args.size() != params.size()) {
            Report(ctx, "'" + callee + "' takes " + std::to_string(params.size()) + " argument(s), got " + std::to_string(args.size()));
            return false;
        }
        bool ok = true;
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i].t == T::Error || params[i] == T::Error) { ok = false; continue; }
            if (!Convertible(params[i], args[i].t)) {
                Report(ctx, "argument " + std::to_string(i + 1) + " of '" + callee + "' must be " + Name(params[i]) + ", got " + Name(args[i].t));
                ok = false;
            }
        }
        return ok;
    }

    static std::string Join(const std::vector<V>& args, bool castToFloat) {
        std::string out;
        for (const V& a : args) {
            if (!out.empty()) out += ", ";
            out += castToFloat ? "static_cast<float>(" + a.c + ")" : a.c;
        }
        return out;
    }

    V CheckCall(P::CallExprContext* call) {
        std::vector<V> args;
        if (call->argList()) {
            for (auto* arg : call->argList()->expr()) args.push_back(Check(arg));
        }

        if (auto* primaryExpr = dynamic_cast<P::PrimaryExprContext*>(call->expr())) {
            if (auto* id = dynamic_cast<P::IdentifierContext*>(primaryExpr->primary())) {
                return CallNamed(id->ID()->getText(), args, call);
            }
            if (auto* type = dynamic_cast<P::TypeNameContext*>(primaryExpr->primary())) {
                return CallConstructor(type->primitiveType()->getText(), args, call);
            }
        }
        if (auto* member = dynamic_cast<P::MemberExprContext*>(call->expr())) {
            const V base = Check(member->expr());
            if (base.t == T::Error) return {};
            const std::string name = member->name->getText();
            const Method* m = FindMethod(base.t, name);
            if (!m) {
                Report(call, Name(base.t) + " has no method '" + name + "'");
                return {};
            }
            const size_t fixed = m->params.size();
            const bool countOk = m->variadic ? args.size() >= fixed : args.size() == fixed;
            if (!countOk) {
                Report(call, "'" + name + "' takes " + std::string(m->variadic ? "at least " : "") + std::to_string(fixed) +
                                 " argument(s), got " + std::to_string(args.size()));
                return {m->ret, ""};
            }
            bool ok = true;
            std::string list;
            for (size_t i = 0; i < args.size(); ++i) {
                const V& a = args[i];
                if (a.t == T::Error) { ok = false; continue; }
                std::string text = a.c;
                if (i < fixed) {
                    if (!Convertible(m->params[i], a.t)) {
                        Report(call, "argument " + std::to_string(i + 1) + " of '" + name + "' must be " + Name(m->params[i]) + ", got " + Name(a.t));
                        ok = false;
                    } else if (m->params[i] == T::Float && a.t == T::Int) {
                        text = "static_cast<float>(" + text + ")";
                    }
                } else if (!Marshalable(a.t)) {
                    Report(call, "argument " + std::to_string(i + 1) + " of '" + name + "' cannot be passed to a script");
                    ok = false;
                }
                if (!list.empty()) list += ", ";
                list += text;
            }
            if (!ok) return {m->ret, ""};
            return {m->ret, base.c + (IsStatic(base.t) ? "::" : ".") + name + "(" + list + ")"};
        }
        Report(call, "expression is not callable");
        return {};
    }

    V CallNamed(const std::string& name, const std::vector<V>& args, P::CallExprContext* call) {
        const auto method = m_methods.find(name);
        if (method != m_methods.end()) {
            const MethodSig& sig = method->second;
            ExpectArgs(name, sig.params, args, call);
            return {sig.ret, name + "(" + Join(args, false) + ")"};
        }

        static const std::map<std::string, std::string> unary = {
            {"sin", "std::sin"}, {"cos", "std::cos"}, {"tan", "std::tan"}, {"sqrt", "std::sqrt"}, {"abs", "std::fabs"}};
        const auto fn = unary.find(name);
        if (fn != unary.end()) {
            ExpectArgs(name, {T::Float}, args, call);
            return {T::Float, fn->second + "(" + Join(args, true) + ")"};
        }
        if (name == "min" || name == "max") {
            ExpectArgs(name, {T::Float, T::Float}, args, call);
            return {T::Float, std::string(name == "min" ? "std::fmin(" : "std::fmax(") + Join(args, true) + ")"};
        }
        if (name == "log") {
            ExpectArgs(name, {T::String}, args, call);
            return {T::Void, "anito::Debug::log(" + Join(args, false) + ")"};
        }

        T local = T::Error;
        if (Lookup(name, local)) Report(call, "'" + name + "' is not a function");
        else Report(call, "undefined function '" + name + "'");
        return {};
    }

    V CallConstructor(const std::string& type, const std::vector<V>& args, P::CallExprContext* call) {
        if (type == "vec3") {
            ExpectArgs("vec3", {T::Float, T::Float, T::Float}, args, call);
            std::string list;
            for (const V& a : args) list += (list.empty() ? "" : ", ") + ("static_cast<float>(" + a.c + ")");
            return {T::Vec3, "anito::Vec3{" + list + "}"};
        }
        if (type == "float" || type == "int") {
            const T result = type == "float" ? T::Float : T::Int;
            if (args.size() != 1) {
                Report(call, "'" + type + "' conversion takes 1 argument");
                return {result, ""};
            }
            if (args[0].t != T::Error && !Numeric(args[0].t)) {
                Report(call, "cannot convert " + std::string(Name(args[0].t)) + " to " + type);
            }
            return {result, "static_cast<" + type + ">(" + args[0].c + ")"};
        }
        Report(call, "type '" + type + "' cannot be constructed");
        return {};
    }

    const std::filesystem::path& m_path;
    std::vector<std::string>& m_errors;
    std::string m_pathText;
    std::string* m_out = nullptr;
    std::map<std::string, T> m_fields;
    std::map<std::string, MethodSig> m_methods;
    std::set<std::string> m_refFields;
    int m_tmp = 0;
    std::vector<std::map<std::string, T>> m_scopes;
    T m_ret = T::Void;
    int m_loopDepth = 0;
    int m_quiet = 0;
    int m_indent = 0;
};

} // namespace

std::map<std::string, std::string> AnalyzeScript(P::ScriptContext* script, const std::filesystem::path& path,
                                                 std::vector<std::string>& errors) {
    return Analyzer(path, errors).Run(script);
}
