#include "ScriptParser.hpp"

#include "ScriptAnalyzer.hpp"

#include "AnitoScriptLexer.h"
#include "AnitoScriptParser.h"

#include <antlr4-runtime.h>

#include <cstdlib>
#include <optional>

namespace {

using Parser = anito_script::AnitoScriptParser;

class ErrorCollector final : public antlr4::BaseErrorListener {
public:
    ErrorCollector(const std::filesystem::path& path, std::vector<std::string>& errors)
        : m_path(path), m_errors(errors) {}

    void syntaxError(antlr4::Recognizer*, antlr4::Token*, size_t line, size_t column,
                     const std::string& message, std::exception_ptr) override {
        m_errors.push_back(m_path.string() + "(" + std::to_string(line) + ":" + std::to_string(column) + "): " + message);
    }

private:
    const std::filesystem::path& m_path;
    std::vector<std::string>& m_errors;
};

std::string Where(const std::filesystem::path& path, antlr4::ParserRuleContext* ctx) {
    return path.string() + "(" + std::to_string(ctx->getStart()->getLine()) + "): ";
}

std::string Unquote(const std::string& literal) {
    std::string out;
    for (size_t i = 1; i + 1 < literal.size(); ++i) {
        if (literal[i] == '\\' && i + 2 < literal.size()) {
            const char next = literal[++i];
            out += next == 'n' ? '\n' : next == 't' ? '\t' : next;
        } else {
            out += literal[i];
        }
    }
    return out;
}

// Constants are evaluated as float/int/bool/string/vec3, then coerced to the field type.
std::optional<ScriptValue> EvalConst(Parser::ExprContext* expr) {
    if (auto* primaryExpr = dynamic_cast<Parser::PrimaryExprContext*>(expr)) {
        antlr4::ParserRuleContext* primary = primaryExpr->primary();
        if (auto* lit = dynamic_cast<Parser::IntLiteralContext*>(primary)) {
            return ScriptValue{std::in_place_type<int>, std::atoi(lit->INT_LIT()->getText().c_str())};
        }
        if (auto* lit = dynamic_cast<Parser::FloatLiteralContext*>(primary)) {
            return ScriptValue{std::in_place_type<float>, std::strtof(lit->FLOAT_LIT()->getText().c_str(), nullptr)};
        }
        if (auto* lit = dynamic_cast<Parser::StringLiteralContext*>(primary)) {
            return ScriptValue{std::in_place_type<std::string>, Unquote(lit->STRING_LIT()->getText())};
        }
        if (dynamic_cast<Parser::TrueLiteralContext*>(primary)) return ScriptValue{std::in_place_type<bool>, true};
        if (dynamic_cast<Parser::FalseLiteralContext*>(primary)) return ScriptValue{std::in_place_type<bool>, false};
        if (auto* paren = dynamic_cast<Parser::ParenExprContext*>(primary)) return EvalConst(paren->expr());
        return std::nullopt;
    }

    if (auto* unary = dynamic_cast<Parser::UnaryExprContext*>(expr)) {
        auto inner = EvalConst(unary->expr());
        if (!inner) return std::nullopt;
        if (unary->op->getType() == Parser::BANG) {
            if (auto* b = std::get_if<bool>(&*inner)) return ScriptValue{std::in_place_type<bool>, !*b};
        } else if (auto* f = std::get_if<float>(&*inner)) {
            return ScriptValue{std::in_place_type<float>, -*f};
        } else if (auto* i = std::get_if<int>(&*inner)) {
            return ScriptValue{std::in_place_type<int>, -*i};
        }
        return std::nullopt;
    }

    if (auto* call = dynamic_cast<Parser::CallExprContext*>(expr)) {
        auto* callee = dynamic_cast<Parser::PrimaryExprContext*>(call->expr());
        auto* typeName = callee ? dynamic_cast<Parser::TypeNameContext*>(callee->primary()) : nullptr;
        if (!typeName || !typeName->primitiveType()->VEC3_T() || !call->argList()) return std::nullopt;

        const auto args = call->argList()->expr();
        if (args.size() != 3) return std::nullopt;

        glm::vec3 v(0.0f);
        for (size_t i = 0; i < 3; ++i) {
            auto component = EvalConst(args[i]);
            if (!component) return std::nullopt;
            if (auto* f = std::get_if<float>(&*component)) v[static_cast<int>(i)] = *f;
            else if (auto* n = std::get_if<int>(&*component)) v[static_cast<int>(i)] = static_cast<float>(*n);
            else return std::nullopt;
        }
        return ScriptValue{std::in_place_type<glm::vec3>, v};
    }
    return std::nullopt;
}

bool Coerce(ScriptFieldType type, const ScriptValue& in, ScriptValue& out) {
    switch (type) {
    case ScriptFieldType::Float:
        if (auto* f = std::get_if<float>(&in)) { out.emplace<float>(*f); return true; }
        if (auto* i = std::get_if<int>(&in)) { out.emplace<float>(static_cast<float>(*i)); return true; }
        return false;
    case ScriptFieldType::Int:
        if (auto* i = std::get_if<int>(&in)) { out.emplace<int>(*i); return true; }
        return false;
    case ScriptFieldType::Bool:
        if (auto* b = std::get_if<bool>(&in)) { out.emplace<bool>(*b); return true; }
        return false;
    case ScriptFieldType::String:
        if (auto* s = std::get_if<std::string>(&in)) { out.emplace<std::string>(*s); return true; }
        return false;
    case ScriptFieldType::Vec3:
        if (auto* v = std::get_if<glm::vec3>(&in)) { out.emplace<glm::vec3>(*v); return true; }
        return false;
    }
    return false;
}

ScriptValue ZeroValue(ScriptFieldType type) {
    switch (type) {
    case ScriptFieldType::Int: return ScriptValue{std::in_place_type<int>, 0};
    case ScriptFieldType::Bool: return ScriptValue{std::in_place_type<bool>, false};
    case ScriptFieldType::String: return ScriptValue{std::in_place_type<std::string>};
    case ScriptFieldType::Vec3: return ScriptValue{std::in_place_type<glm::vec3>, 0.0f};
    default: return ScriptValue{std::in_place_type<float>, 0.0f};
    }
}

bool FieldTypeFromName(const std::string& name, ScriptFieldType& out) {
    if (name == "float") out = ScriptFieldType::Float;
    else if (name == "int") out = ScriptFieldType::Int;
    else if (name == "bool") out = ScriptFieldType::Bool;
    else if (name == "string") out = ScriptFieldType::String;
    else if (name == "vec3") out = ScriptFieldType::Vec3;
    else return false;
    return true;
}

Parser::AnnotationContext* FindAnnotation(const std::vector<Parser::AnnotationContext*>& annotations, const std::string& name) {
    for (auto* annotation : annotations) {
        if (annotation->name && annotation->name->getText() == name) return annotation;
    }
    return nullptr;
}

void ReadField(Parser::FieldDeclContext* decl, const std::filesystem::path& path,
               ScriptDescriptor& desc, std::vector<std::string>& errors) {
    auto* fieldAnnotation = FindAnnotation(decl->annotation(), "field");
    if (!fieldAnnotation) return; // Not exposed to the editor

    ScriptField field;
    field.name = decl->name->getText();

    if (!FieldTypeFromName(decl->type()->getText(), field.type)) {
        errors.push_back(Where(path, decl) + "unsupported field type '" + decl->type()->getText() + "' for '" + field.name + "'");
        return;
    }

    const auto labels = fieldAnnotation->STRING_LIT();
    field.displayName = labels.empty() ? field.name : Unquote(labels.front()->getText());
    field.defaultValue = ZeroValue(field.type);

    if (decl->expr()) {
        auto value = EvalConst(decl->expr());
        if (!value || !Coerce(field.type, *value, field.defaultValue)) {
            errors.push_back(Where(path, decl) + "default for '" + field.name + "' must be a constant " + decl->type()->getText());
        }
    }
    desc.fields.push_back(std::move(field));
}

} // namespace

std::vector<ScriptDescriptor> ScriptParser::Parse(const std::string& source,
                                                  const std::filesystem::path& path,
                                                  std::vector<std::string>& errors) {
    antlr4::ANTLRInputStream input(source);
    anito_script::AnitoScriptLexer lexer(&input);
    ErrorCollector listener(path, errors);
    lexer.removeErrorListeners();
    lexer.addErrorListener(&listener);

    antlr4::CommonTokenStream tokens(&lexer);
    Parser parser(&tokens);
    parser.removeErrorListeners();
    parser.addErrorListener(&listener);

    // ANTLR recovers from syntax errors, so declarations before the error are still reported.
    Parser::ScriptContext* tree = parser.script();
    const bool syntaxClean = errors.empty();

    std::vector<ScriptDescriptor> result;
    for (auto* component : tree->componentDecl()) {
        if (!component->name) continue;

        ScriptDescriptor desc;
        desc.name = component->name->getText();
        desc.path = path;

        for (auto* member : component->member()) {
            if (auto* field = member->fieldDecl()) {
                if (field->name && field->type()) ReadField(field, path, desc, errors);
            } else if (auto* method = member->methodDecl()) {
                if (method->name && FindAnnotation(method->annotation(), "method")) {
                    desc.methods.push_back(method->name->getText());
                }
            } else if (auto* handler = member->eventHandler()) {
                if (handler->name) desc.events.push_back(handler->name->getText());
            }
        }
        result.push_back(std::move(desc));
    }

    // Recovered trees can be incomplete, so semantic checks only run on clean syntax.
    if (syntaxClean) {
        const auto code = AnalyzeScript(tree, path, errors);
        for (auto& desc : result) {
            const auto it = code.find(desc.name);
            if (it != code.end()) desc.cppSource = it->second;
        }
    }
    return result;
}
