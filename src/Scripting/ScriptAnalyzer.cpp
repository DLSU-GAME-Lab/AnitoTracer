#include "ScriptAnalyzer.hpp"

#include <antlr4-runtime.h>

#include <map>
#include <set>

namespace {

using P = anito_script::AnitoScriptParser;

enum class T { Error, Void, Float, Int, Bool, String, Vec3, Object, Transform };

const char* Name(T t) {
    switch (t) {
    case T::Void: return "void";
    case T::Float: return "float";
    case T::Int: return "int";
    case T::Bool: return "bool";
    case T::String: return "string";
    case T::Vec3: return "vec3";
    case T::Object: return "object";
    case T::Transform: return "transform";
    default: return "<error>";
    }
}

const char* Cpp(T t) {
    switch (t) {
    case T::Float: return "float";
    case T::Int: return "int";
    case T::Bool: return "bool";
    case T::String: return "std::string";
    case T::Vec3: return "anito::Vec3";
    default: return "void";
    }
}

bool Numeric(T t) { return t == T::Float || t == T::Int; }
bool Convertible(T to, T from) { return to == from || (to == T::Float && from == T::Int); }

// A checked expression: its type and the C++ that computes it.
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
};

// Names that would collide with C++ or with the generated class's own members.
const std::set<std::string> kReserved = {
    "alignas", "alignof", "and", "asm", "auto", "bitand", "bitor", "case", "catch", "char", "class", "compl",
    "concept", "const", "constexpr", "const_cast", "decltype", "default", "delete", "do", "double", "dynamic_cast",
    "enum", "explicit", "export", "extern", "friend", "goto", "inline", "long", "mutable", "namespace", "new",
    "noexcept", "not", "nullptr", "operator", "or", "private", "protected", "public", "register", "short",
    "signed", "sizeof", "static", "struct", "switch", "template", "this", "throw", "try", "typedef", "typeid",
    "typename", "union", "unsigned", "using", "virtual", "volatile", "xor", "std", "anito", "ctx",
    "m_ctx", "Log", "GetPosition", "SetPosition", "Rotate", "OnStart", "OnUpdate", "Invoke", "method"};

struct Lvalue {
    std::string pre, ref, post;
};

class Analyzer {
public:
    Analyzer(const std::filesystem::path& path, std::vector<std::string>& errors)
        : m_path(path), m_errors(errors), m_pathText(path.generic_string()) {}

    std::map<std::string, std::string> Run(P::ScriptContext* script) {
        const size_t before = m_errors.size();
        std::set<std::string> seen;
        std::map<std::string, std::string> result;
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
        if (kReserved.count(name) || name.front() == '_') Report(ctx, "'" + name + "' is a reserved name");
    }

    // ---- output ----

    void Emit(const std::string& line) { *m_out += std::string(m_indent * 4, ' ') + line + "\n"; }

    // Maps compiler errors in the generated C++ back to the .ascript source.
    void Directive(antlr4::ParserRuleContext* ctx) {
        *m_out += "#line " + std::to_string(ctx->getStart()->getLine()) + " \"" + m_pathText + "\"\n";
    }

    // ---- declarations ----

    T ResolveType(P::TypeContext* type, bool allowVoid) {
        const std::string text = type->getText();
        if (text == "float") return T::Float;
        if (text == "int") return T::Int;
        if (text == "bool") return T::Bool;
        if (text == "string") return T::String;
        if (text == "vec3") return T::Vec3;
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

        const auto ids = component->ID();
        for (size_t i = 1; i < ids.size(); ++i) {
            if (!kHandlers.count(ids[i]->getText())) {
                ReportAt(ids[i]->getSymbol(), "unknown trigger '" + ids[i]->getText() + "'");
            }
        }

        // Declarations first so members can refer to each other regardless of order.
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

        std::string members;   // handlers and methods
        std::string data;      // field declarations
        std::string init;      // constructor initialisers for exposed fields
        std::string invoke;    // @method dispatch cases
        size_t exposedIndex = 0;
        m_indent = 1;

        for (auto* member : component->member()) {
            if (auto* field = member->fieldDecl()) {
                m_out = &data;
                EmitField(field, exposedIndex, init);
            } else if (auto* method = member->methodDecl()) {
                m_out = &members;
                EmitMethod(method, invoke);
            } else if (auto* handler = member->eventHandler()) {
                m_out = &members;
                EmitHandler(handler);
            }
        }

        const std::string cls = component->name->getText();
        std::string out;
        out += "class " + cls + " final : public anito::ScriptBehaviour {\n";
        out += "public:\n";
        out += "    explicit " + cls + "(const anito::ScriptContext& ctx)\n";
        out += "        : anito::ScriptBehaviour(ctx)" + init + " {}\n\n";
        out += members;
        out += "    bool Invoke(const char* method) override {\n" + invoke + "        return false;\n    }\n\n";
        out += "private:\n" + data + "};\n";
        return out;
    }

    void EmitField(P::FieldDeclContext* field, size_t& exposedIndex, std::string& init) {
        const std::string name = field->name->getText();
        const T declared = m_fields[name];
        if (declared == T::Error) return;

        if (IsExposed(field)) {
            // Exposed fields live in host storage so the Inspector and serializer own the values.
            init += ",\n          " + name + "(*static_cast<" + Cpp(declared) + "*>(ctx.fields[" + std::to_string(exposedIndex++) + "]))";
            Emit(std::string(Cpp(declared)) + "& " + name + ";");
            return;
        }

        std::string value = "{}";
        if (field->expr()) {
            m_scopes.clear();
            const V v = Check(field->expr());
            if (v.t != T::Error && !Convertible(declared, v.t)) {
                Report(field, "cannot initialize " + std::string(Name(declared)) + " field '" + name + "' with " + Name(v.t));
            }
            value = "= " + v.c;
        }
        Emit(std::string(Cpp(declared)) + " " + name + " " + (value == "{}" ? "{}" : value) + ";");
    }

    void EmitMethod(P::MethodDeclContext* method, std::string& invoke) {
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
                list += std::string(Cpp(sig.params[i])) + " " + pname;
                ++i;
            }
        }
        if (IsMethodExposed(method) && params.empty()) {
            invoke += "        if (std::strcmp(method, \"" + name + "\") == 0) { " + name + "(); return true; }\n";
        }
        EmitBody(method, method->block(), std::string(Cpp(sig.ret)) + " " + name + "(" + list + ")", params, sig.ret);
    }

    void EmitHandler(P::EventHandlerContext* handler) {
        std::vector<std::pair<std::string, T>> params;
        std::string list;
        std::string cppName = "OnUnknown";
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
                    list += std::string(Cpp(it->second.params[i])) + " " + pname;
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
        if (name == "owner") { out = T::Object; return true; }
        return false;
    }

    // ---- statements ----

    // Returns `Type name = init` for let statements and for-loop initialisers.
    std::string CheckLet(antlr4::ParserRuleContext* ctx, antlr4::Token* nameToken, P::TypeContext* type, P::ExprContext* init) {
        const V value = Check(init);
        T result = value.t;
        if (value.t == T::Void || value.t == T::Object || value.t == T::Transform) {
            Report(ctx, std::string("cannot store a ") + Name(value.t) + " value");
            result = T::Error;
        }
        if (type) {
            const T declared = ResolveType(type, false);
            if (declared != T::Error && result != T::Error && !Convertible(declared, result)) {
                Report(ctx, "cannot initialize " + std::string(Name(declared)) + " '" + nameToken->getText() + "' with " + Name(result));
            }
            result = declared;
        }
        Declare(nameToken->getText(), result, ctx);
        return std::string(Cpp(result)) + " " + nameToken->getText() + " = " + value.c;
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
            return id && id->ID()->getText() != "owner";
        }
        if (auto* member = dynamic_cast<P::MemberExprContext*>(expr)) {
            ++m_quiet;
            const T base = Check(member->expr()).t;
            --m_quiet;
            if (base == T::Vec3) return IsLvalue(member->expr());
            return base == T::Transform && member->name->getText() == "position";
        }
        return false;
    }

    // Assignable expressions that go through host calls (position) are staged in a temporary.
    Lvalue ToLvalue(P::ExprContext* expr) {
        if (auto* primary = dynamic_cast<P::PrimaryExprContext*>(expr)) {
            return {"", dynamic_cast<P::IdentifierContext*>(primary->primary())->ID()->getText(), ""};
        }
        auto* member = dynamic_cast<P::MemberExprContext*>(expr);
        ++m_quiet;
        const T base = Check(member->expr()).t;
        --m_quiet;
        if (base == T::Vec3) {
            Lvalue inner = ToLvalue(member->expr());
            inner.ref += "." + member->name->getText();
            return inner;
        }
        return {"anito::Vec3 _pos = GetPosition();", "_pos", "SetPosition(_pos);"};
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
        if (op == P::ASSIGN) {
            if (!Convertible(left.t, right.t)) {
                Report(assignment, std::string("cannot assign ") + Name(right.t) + " to " + Name(left.t));
                return false;
            }
        } else {
            const char* symbol = op == P::PLUS_ASSIGN ? "+" : op == P::MINUS_ASSIGN ? "-" : op == P::STAR_ASSIGN ? "*" : "/";
            const V result = Arith(symbol, left, right, assignment);
            if (result.t == T::Error) return false;
            if (!Convertible(left.t, result.t)) {
                Report(assignment, std::string("cannot assign ") + Name(result.t) + " to " + Name(left.t));
                return false;
            }
        }
        target = ToLvalue(sides[0]);
        expression = target.ref + " " + assignment->op->getText() + " " + right.c;
        return true;
    }

    void EmitAssignment(P::AssignmentContext* assignment) {
        Lvalue target;
        std::string expression;
        if (!BuildAssignment(assignment, target, expression)) return;
        if (target.pre.empty()) {
            Emit(expression + ";");
        } else {
            Emit("{ " + target.pre + " " + expression + "; " + target.post + " }");
        }
    }

    std::string ForAssignment(P::AssignmentContext* assignment) {
        Lvalue target;
        std::string expression;
        if (!BuildAssignment(assignment, target, expression)) return "";
        if (!target.pre.empty()) {
            Report(assignment, "assigning to a transform property is not supported in a for header");
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
        Report(ctx, "operator '" + op + "' cannot be applied to " + Name(l.t) + " and " + Name(r.t));
        return {};
    }

    V Check(P::ExprContext* expr) {
        if (auto* e = dynamic_cast<P::PrimaryExprContext*>(expr)) return CheckPrimary(e->primary());
        if (auto* e = dynamic_cast<P::MemberExprContext*>(expr)) return CheckMember(e);
        if (auto* e = dynamic_cast<P::CallExprContext*>(expr)) return CheckCall(e);
        if (dynamic_cast<P::IndexExprContext*>(expr)) {
            Report(expr, "indexing is not supported");
            return {};
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
            if (l.t != T::Error && r.t != T::Error && !(l.t == r.t || (Numeric(l.t) && Numeric(r.t)))) {
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
            if (Lookup(name, type)) return {type, name == "owner" && type == T::Object ? "" : name};
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
        if (base.t == T::Object && name == "transform") return {T::Transform, ""};
        if (base.t == T::Transform && name == "position") return {T::Vec3, "GetPosition()"};
        if (base.t == T::Vec3 && (name == "x" || name == "y" || name == "z")) return {T::Float, base.c + "." + name};
        Report(member, std::string(Name(base.t)) + " has no member '" + name + "'");
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
            if (base.t == T::Transform && name == "rotate") {
                if (!ExpectArgs("rotate", {T::Vec3, T::Float}, args, call)) return {T::Void, ""};
                return {T::Void, "Rotate(" + Join(args, false) + ")"};
            }
            Report(call, std::string(Name(base.t)) + " has no method '" + name + "'");
            return {};
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
            return {T::Void, "Log((" + Join(args, false) + ").c_str())"};
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
