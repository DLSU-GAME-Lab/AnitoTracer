#include "ScriptLanguage.hpp"

#include "AnitoScriptLexer.h"
#include "ScriptApiMetadata.hpp"

#include <antlr4-runtime.h>

#include <algorithm>
#include <cctype>
#include <set>
#include <string_view>

namespace Diligent::ScriptLanguage
{
    namespace
    {
        bool IsIdentifierStart(char c)
        {
            return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
        }

        bool IsIdentifierCharacter(char c)
        {
            return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
        }

        bool StartsWithIgnoreCase(std::string_view word, std::string_view prefix)
        {
            return word.size() >= prefix.size() &&
                std::equal(prefix.begin(), prefix.end(), word.begin(), [](char left, char right) {
                    return std::tolower(static_cast<unsigned char>(left)) ==
                        std::tolower(static_cast<unsigned char>(right));
                });
        }

        bool IsKeyword(int type)
        {
            using Lexer = anito_script::AnitoScriptLexer;
            return type >= Lexer::COMPONENT && type <= Lexer::FALSE;
        }

        bool IsTypeKeyword(int type)
        {
            using Lexer = anito_script::AnitoScriptLexer;
            return type >= Lexer::FLOAT_T && type <= Lexer::VOID_T;
        }

        std::vector<std::string> GrammarLiterals()
        {
            using Lexer = anito_script::AnitoScriptLexer;
            std::vector<std::string> result;
            antlr4::ANTLRInputStream input("");
            Lexer lexer(&input);
            const auto& vocabulary = lexer.getVocabulary();
            for (int token = Lexer::COMPONENT; token <= Lexer::VOID_T; ++token)
            {
                const std::string literal(vocabulary.getLiteralName(token));
                if (literal.size() < 3 || literal.front() != '\'' || literal.back() != '\'') continue;
                const std::string word = literal.substr(1, literal.size() - 2);
                if (!word.empty() && IsIdentifierStart(word.front())) result.push_back(word);
            }
            return result;
        }

        std::vector<std::string> EngineWords()
        {
            static const std::vector<std::string> words = {
                "array", "gameobject", "script", "prefab", "raycasthit", "quat",
                "Mathf", "Vector3", "Quaternion", "Random", "Time", "Input", "Scene", "Gui", "Debug",
                "owner", "sin", "cos", "tan", "sqrt", "abs", "min", "max", "log",
                "Start", "Update", "FixedUpdate", "GUI", "CollisionEnter", "field", "method",
                "valid", "gameObject", "name", "transform", "rigidbody", "collider", "audioSource", "camera",
                "parent", "root", "childCount", "componentCount", "length", "lengthSq", "normalized",
                "dot", "cross", "count", "add", "insert", "removeAt", "clear", "get", "contains", "indexOf",
                "prune", "position", "rotation", "localPosition", "localRotation", "localScale",
                "eulerAngles", "forward", "right", "up", "rotate", "translate", "lookAt",
                "transformPoint", "transformDirection", "inverseTransformDirection",
                "getScript", "hasScript", "addScript", "componentName", "hasComponent", "addRigidbody",
                "addBoxCollider", "addSphereCollider", "addCapsuleCollider", "child", "findChild", "destroy",
                "sendMessage", "call", "callFloat", "callInt", "callBool", "callVec3", "callObject",
                "pi", "deg2rad", "rad2deg", "epsilon", "clamp", "clamp01", "lerp", "sign", "asin",
                "acos", "atan", "atan2", "cbrt", "pow", "exp", "floor", "ceil", "round", "moveTowards",
                "zero", "one", "distance", "normalize", "scale", "identity", "angleAxis", "euler",
                "lookRotation", "slerp", "value", "range", "onUnitSphere", "insideUnitSphere",
                "time", "deltaTime", "fixedDeltaTime", "mouseDelta", "mousePosition", "getAction",
                "none", "createEmpty", "createPrimitive", "instantiate", "find", "findWithScript",
                "mainCamera", "worldToScreen", "fillRect", "drawRect"
            };
            return words;
        }

        std::string_view PrefixAt(const std::string& source, std::size_t cursor)
        {
            cursor = std::min(cursor, source.size());
            std::size_t begin = cursor;
            while (begin > 0 && IsIdentifierCharacter(source[begin - 1])) --begin;
            return { source.data() + begin, cursor - begin };
        }

        std::vector<std::string> RecognizedIdentifiers(const std::string& source)
        {
            antlr4::ANTLRInputStream input(source);
            anito_script::AnitoScriptLexer lexer(&input);
            std::vector<std::string> result;
            for (const auto& token : lexer.getAllTokens())
                if (token->getType() == anito_script::AnitoScriptLexer::ID)
                    result.push_back(token->getText());
            return result;
        }

        int ContextScore(const std::string& word, const std::string& previous)
        {
            static const std::set<std::string> events = {
                "Start", "Update", "FixedUpdate", "GUI", "CollisionEnter"
            };
            static const std::set<std::string> annotations = { "field", "method" };
            static const std::set<std::string> types = {
                "float", "int", "bool", "string", "vec3", "void", "quat", "gameobject",
                "transform", "rigidbody", "collider", "audiosource", "camera", "script",
                "prefab", "raycasthit", "array"
            };
            if (previous == ".") return 30;
            if (previous == "on" && events.count(word)) return 40;
            if (previous == "@" && annotations.count(word)) return 40;
            if (previous == ":" && types.count(word)) return 35;
            if (previous == "component" || previous == "fn" || previous == "let") return 10;
            return 0;
        }

        std::string PreviousSignificantToken(const std::string& source, std::size_t begin)
        {
            antlr4::ANTLRInputStream input(source.substr(0, begin));
            anito_script::AnitoScriptLexer lexer(&input);
            std::string previous;
            for (const auto& token : lexer.getAllTokens())
                if (token->getChannel() == antlr4::Token::DEFAULT_CHANNEL)
                    previous = token->getText();
            return previous;
        }
    }

    std::vector<SyntaxToken> Highlight(const std::string& source)
    {
        antlr4::ANTLRInputStream input(source);
        anito_script::AnitoScriptLexer lexer(&input);
        std::vector<SyntaxToken> result;
        bool annotationPending = false;

        for (const auto& token : lexer.getAllTokens())
        {
            using Lexer = anito_script::AnitoScriptLexer;
            const std::size_t begin = static_cast<std::size_t>(
                std::max<std::size_t>(0, token->getStartIndex()));
            const std::size_t end = token->getStopIndex() + 1;
            if (begin >= end || end > source.size()) continue;

            TokenKind kind = TokenKind::Plain;
            const int type = static_cast<int>(token->getType());
            if (IsTypeKeyword(type)) kind = TokenKind::Type;
            else if (IsKeyword(type)) kind = TokenKind::Keyword;
            else if (type == Lexer::INT_LIT || type == Lexer::FLOAT_LIT) kind = TokenKind::Number;
            else if (type == Lexer::STRING_LIT) kind = TokenKind::String;
            else if (type == Lexer::LINE_COMMENT || type == Lexer::BLOCK_COMMENT) kind = TokenKind::Comment;
            else if (type >= Lexer::PLUS_ASSIGN && type <= Lexer::RBRACK) kind = TokenKind::Operator;

            if (annotationPending && token->getChannel() == antlr4::Token::DEFAULT_CHANNEL)
            {
                kind = TokenKind::Annotation;
                annotationPending = false;
            }
            if (type == Lexer::AT) annotationPending = true;
            result.push_back({ begin, end, kind });
            if (type == Lexer::AT) result.back().kind = TokenKind::Annotation;
        }
        return result;
    }

    Completion Complete(const std::string& source, std::size_t cursor,
                        const std::vector<std::string>& projectWords)
    {
        cursor = std::min(cursor, source.size());
        const std::string_view prefix = PrefixAt(source, cursor);
        Completion result{ cursor - prefix.size(), std::string(prefix), {} };

        std::set<std::string> words;
        for (const std::string& word : GrammarLiterals()) words.insert(word);
        for (const std::string& word : EngineWords()) words.insert(word);
        for (const std::string& word : RecognizedIdentifiers(source)) words.insert(word);
        for (const std::string& word : projectWords) words.insert(word);
        for (const auto& [name, metadata] : anito::script::RegisteredTypes())
        {
            words.insert(name);
            for (const auto& member : metadata.members) words.insert(member.name);
            for (const auto& method : metadata.methods) words.insert(method.name);
        }

        const std::string previous = PreviousSignificantToken(source, result.replaceBegin);
        if (prefix.empty() && previous != "." && previous != "@")
            return result;
        std::vector<std::pair<int, std::string>> ranked;
        for (const std::string& word : words)
        {
            if (word.size() <= prefix.size() ||
                !StartsWithIgnoreCase(word, prefix))
                continue;
            ranked.emplace_back(ContextScore(word, previous), word);
        }

        std::sort(ranked.begin(), ranked.end(), [](const auto& left, const auto& right) {
            if (left.first != right.first) return left.first > right.first;
            return left.second < right.second;
        });
        for (const auto& [score, word] : ranked) result.words.push_back(word);
        return result;
    }
}
