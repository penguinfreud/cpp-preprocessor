#include "preprocessor.h"
#include <fstream>
#include <iostream>

namespace cpp {
    namespace {
        token_t truncateLine(token_t token) {
            if (!token)
                return token;
            const std::string &v = token->value();
            auto i = v.find('\r'), j = v.find('\n');
            unsigned long pos;
            if (i == std::string::npos)
                pos = j;
            else if (j == std::string::npos)
                pos = i;
            else if (i < j)
                pos = i;
            else
                pos = j;
            return std::make_shared<Token>(Token::WHITESPACE, v.substr(pos), token->pos(), true);
        }

        bool shouldIgnore(const std::vector<int> &ifStack) {
            if (ifStack.empty())
                return false;
            auto state = ifStack.back();
            return state == 2 || state == 3 || state == 5 || state == 6;
        }
    }

    token_t DirectiveParser::_next() {
        if (included) {
            auto token = included->next();
            if (token) {
                return token;
            } else {
                included.reset();
            }
        }
        token_t sharp;
        auto pos = getPos();
        if (lineStart && (sharp = input()->matchPunc('#'))) {
            input()->space(false);
            if (input()->matchId("if")) {
                return parseIf(false);
            } else if (input()->matchId("ifdef")) {
                return parseIf(true, false);
            } else if (input()->matchId("ifndef")) {
                return parseIf(true, true);
            } else if (input()->matchId("elif")) {
                return parseElif(pos);
            } else if (input()->matchId("else")) {
                return parseElse(pos);
            } else if (input()->matchId("endif")) {
                return parseEndif(pos);
            } else {
                if (shouldIgnore(ifStack))
                    return skipLine();
                if (input()->matchId("define")) {
                    input()->space(false);
                    return parseDefine();
                } else if (input()->matchId("undef")) {
                    input()->space(false);
                    return parseUndef();
                } else if (input()->matchId("include")) {
                    input()->space(false);
                    return parseInclude(sharp->pos());
                } else {
                    return skipLine();
                }
            }
        } else {
            lineStart = false;
            if (shouldIgnore(ifStack))
                return skipLine();
            auto token = input()->next();
            if (token && token->type() == Token::WHITESPACE && token->hasNewLine()) {
                lineStart = true;
            }
            return token;
        }
    }

    token_t DirectiveParser::skipLine() {
        while (auto token = input()->next()) {
            if (token->type() == Token::WHITESPACE && token->hasNewLine()) {
                lineStart = true;
                return truncateLine(token);
            }
        }
        return token_t();
    }

    std::deque<token_t> DirectiveParser::readLine(bool allowVAARGS) {
        std::deque<token_t> result;
        lineStart = false;
        while (auto token = input()->next()) {
            if (token->type() == Token::WHITESPACE && token->hasNewLine()) {
                input()->unget(token);
                lineStart = true;
                break;
            } else if (!allowVAARGS &&
                       token->type() == Token::IDENTIFIER &&
                       token->value() == "__VA_ARGS__") {
                throw ParsingException("Unexpected __VA_ARGS__", token->pos());
            } else {
                result.push_back(token);
            }
        }
        if (!lineStart && !result.empty() &&
            result.back()->type() == Token::WHITESPACE) {
            result.pop_back();
        }
        return result;
    }

    token_t DirectiveParser::parseDefine() {
        const auto &name = input()->expectId()->value();
        if (input()->matchPunc('(')) {
            input()->space(false);
            bool first = true;
            auto macro = std::make_shared<FunctionMacro>(name);
            while (!input()->finished()) {
                if (input()->matchPunc(')')) {
                    macro->setBody(readLine());
                    (*macroTable())[name] = macro;
                    return truncateLine(input()->expectNewLine());
                } else {
                    if (first) {
                        first = false;
                    } else {
                        input()->expectPunc(',');
                        input()->space(false);
                    }
                    if (input()->matchPunc("...")) {
                        macro->addParam("__VA_ARGS__");
                        input()->space(false);
                        input()->expectPunc(')');
                        macro->setBody(readLine(true));
                        (*macroTable())[name] = macro;
                        return truncateLine(input()->expectNewLine());
                    } else {
                        macro->addParam(input()->expectId()->value());
                    }
                }
            }
            throw ParsingException("Expected )", getPos());
        } else {
            auto token = input()->next();
            if (token && token->type() != Token::WHITESPACE) {
                throw ParsingException("Expected space", token->pos());
            }
            auto macro = std::make_shared<Macro>(name);
            (*macroTable())[name] = macro;
            if (token && !token->hasNewLine()) {
                macro->setBody(readLine());
                token = input()->expectNewLine();
            }
            return truncateLine(token);
        }
    }

    token_t DirectiveParser::parseUndef() {
        const auto &name = input()->expectId()->value();
        auto it = macroTable()->find(name);
        if (it != macroTable()->end()) {
            macroTable()->erase(it);
        }
        return truncateLine(input()->expectNewLine());
    }

    token_t DirectiveParser::parseIf(bool defined, bool neg) {
        input()->space(false);
        bool cond;
        if (defined) {
            const auto &name = input()->expectId()->value();
            cond = (bool) (*macroTable())[name] != neg;
        } else {
            cond = parseCondition(readLine(), macroTable(), stack());
        }
        if (shouldIgnore(ifStack))
            ifStack.push_back(3);
        else if (cond)
            ifStack.push_back(1);
        else
            ifStack.push_back(2);
        return truncateLine(input()->expectNewLine());
    }

    token_t DirectiveParser::parseElif(const PosInfo &pos) {
        input()->space(false);
        bool cond = parseCondition(readLine(), macroTable(), stack());

        auto state = 0;
        if (ifStack.empty())
            throw ParsingException("Unexpected #elif", pos);
        else
            state = ifStack.back();
        if (state == 1)
            ifStack.back() = 2;
        else if (state == 2 && cond)
            ifStack.back() = 1;
        else if (state > 3)
            throw ParsingException("Unexpected #elif", pos);
        return truncateLine(input()->expectNewLine());
    }

    token_t DirectiveParser::parseElse(const PosInfo &pos) {
        input()->space(false);

        auto state = 0;
        if (ifStack.empty())
            throw ParsingException("Unexpected #else", pos);
        else
            state = ifStack.back();
        if (state == 1)
            ifStack.back() = 5;
        else if (state == 2)
            ifStack.back() = 4;
        else if (state == 3)
            ifStack.back() = 6;
        else if (state > 3)
            throw ParsingException("Unexpected #else", pos);
        return truncateLine(input()->expectNewLine());
    }

    token_t DirectiveParser::parseEndif(const PosInfo &pos) {
        input()->space(false);

        if (ifStack.empty())
            throw ParsingException("Unexpected #endif", pos);
        ifStack.pop_back();

        return truncateLine(input()->expectNewLine());
    }

    token_t DirectiveParser::parseInclude(const PosInfo &pos) {
        auto token = input()->next();
        if (!token)
            throw ParsingException("Expected '\"' or '<'", getPos());
        if (token->type() == Token::STRING) {
            std::string path(token->value().substr(1, token->value().size() - 2));
            return include(path, pos, expectNewLine(), true);
        } else if (token->value()[0] == '<') {
            std::string path(token->value().substr(1));
            while (token = input()->next()) {
                if (token->type() == Token::WHITESPACE && token->hasNewLine())
                    throw ParsingException("Expected >", token->pos());
                const auto &v = token->value();
                auto i = v.find('>');
                if (i == std::string::npos) {
                    path += v;
                } else if (i != v.size() - 1) {
                    throw ParsingException(("Unexpected: " + v.substr(i + 1)).c_str(), token->pos() + (i + 1));
                } else {
                    path += v.substr(0, i);
                    return include(path, pos, expectNewLine(), false);
                }
            }
            throw ParsingException("Expected >", getPos());
        } else {
            throw ParsingException("Expected \" or <", token->pos());
        }
    }

    namespace {
        void resolve(const std::string &base, const std::string &path, std::string &result) {
            if (path[0] == '/') {
                result = path;
            } else {
                auto i = base.rfind('/');
                if (i == std::string::npos) {
                    result = path;
                } else {
                    result = base.substr(0, i + 1) + path;
                }
            }
        }
    }

    token_t DirectiveParser::include(const std::string &path, const PosInfo &pos, token_t space, bool isQuote) {
        if (recursionDepth >= MAX_INCLUDE_RECURSION) {
            std::cerr << "Reached max include recursion depth";
            return truncateLine(space);
        }
        if (isQuote) {
            std::string result;
            resolve(file(), path, result);
            auto input = std::make_shared<std::ifstream>();
            input->open(result);
            if (input->is_open()) {
                auto tokenizer = std::make_shared<Tokenizer>(input, result);
                auto dirParser = std::make_shared<DirectiveParser>(tokenizer, macroTable(), stack(), result, recursionDepth + 1);
                included = std::make_shared<MacroExpander>(dirParser, macroTable(), stack());
                return _next();
            } else {
                std::cerr << "Open file failed: " << result << std::endl;
            }
        }
        std::string v("#include ");
        v.push_back(isQuote? '"': '<');
        v += path;
        v.push_back(isQuote? '"': '>');
        v += space->value();
        return std::make_shared<Token>(Token::OTHER, v, pos);
    }
}
