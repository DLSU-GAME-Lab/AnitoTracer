#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace Diligent::ScriptLanguage
{
    enum class TokenKind
    {
        Plain,
        Keyword,
        Type,
        Number,
        String,
        Comment,
        Annotation,
        Operator
    };

    struct SyntaxToken
    {
        std::size_t begin = 0;
        std::size_t end = 0;
        TokenKind kind = TokenKind::Plain;
    };

    struct Completion
    {
        std::size_t replaceBegin = 0;
        std::string prefix;
        std::vector<std::string> words;

        const std::string* Selected() const { return words.empty() ? nullptr : &words.front(); }
    };

    std::vector<SyntaxToken> Highlight(const std::string& source);
    Completion Complete(const std::string& source, std::size_t cursor,
                        const std::vector<std::string>& projectWords);
}
