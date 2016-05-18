#include "preprocessor.h"

namespace cpp {
    token_t MacroExpander::_next() {
        return __next();
    }

    token_t MacroExpander::__next(bool enableMacro) {
        if (expander) {
            auto token = expander->next();
            if (token) {
                return token;
            } else {
                expander.reset();
            }
        }
        auto token = input()->next();
        if (enableMacro && token &&
            token->type() == Token::IDENTIFIER) {
            return expandMacro(token);
        }
        return token;
    }

    token_t MacroExpander::expandMacro(token_t name) {
        if (name->value() == "__VA_ARGS__")
            throw ParsingException("Unexpected __VA_ARGS__", name->pos());
        if ((!stack() || !(stack()->hasName(name->value()))) && macroTable()) {
            auto macro = (*macroTable())[name->value()];
            if (macro) {
                if (macro->isFunctionLike()) {
                    return expandFunctionMacro(name, *static_cast<FunctionMacro*>(macro.get()));
                } else {
                    return expandObjectMacro(*macro);
                }
            }
        }
        return name;
    }

    namespace {
        void appendToken(std::deque<token_t> &list, token_t token, bool &ws) {
            if (token->type() == Token::WHITESPACE) {
                ws = true;
            } else {
                if (ws) {
                    if (!list.empty())
                        list.push_back(std::make_shared<Token>(Token::WHITESPACE, " ", token->pos()));
                    ws = false;
                }
                list.push_back(token);
            }
        }

        inline void appendTokens(std::deque<token_t> &list, std::shared_ptr<std::deque<token_t>> append, bool &ws) {
            for (auto token: *append) {
                appendToken(list, token, ws);
            }
        }

        inline int findParam(const std::deque<std::string> &params, const std::string &name) {
            for (int i = 0; i<params.size(); i++) {
                if (params[i] == name)
                    return i;
            }
            return -1;
        }
    }

    token_t MacroExpander::expandObjectMacro(const Macro &macro) {
        if (!macro.empty()) {
            std::deque<token_t> result;
            bool ws = false;
            appendTokens(result, std::make_shared<std::deque<token_t>>(macro.body()), ws);
            auto stream = std::make_shared<TokenStream>(result);
            expander = std::make_shared<MacroExpander>(stream, macroTable(), std::make_shared<MacroStack>(macro.name(), stack()));
        }
        return _next();
    }

    token_t MacroExpander::expandFunctionMacro(token_t name, const FunctionMacro &macro) {
        auto token = input()->space();
        if (input()->matchPunc('(')) {
            input()->space();
            std::vector<std::shared_ptr<std::deque<token_t>>> args;
            auto curArg = std::make_shared<std::deque<token_t>>();
            int depth = 0;
            while (!input()->finished()) {
                if ((token = input()->matchPunc(')'))) {
                    if (depth == 0) {
                        args.push_back(scanArg(curArg));

                        auto stream = std::make_shared<TokenStream>(subBody(macro, args));
                        expander = std::make_shared<MacroExpander>(stream, macroTable(), std::make_shared<MacroStack>(macro.name(), stack()));
                        return _next();
                    } else {
                        depth--;
                        curArg->push_back(token);
                    }
                } else if ((token = input()->matchPunc('('))) {
                    depth++;
                    curArg->push_back(token);
                } else if (depth == 0 && input()->matchPunc(',')) {
                    input()->space();
                    args.push_back(scanArg(curArg));
                    curArg = std::make_shared<std::deque<token_t>>();
                } else {
                    token = input()->next();
                    if (!token)
                        break;
                    curArg->push_back(token);
                }
            }
            throw ParsingException("Expected )", getPos());
        } else {
            if (token)
                input()->unget(token);
            return name;
        }
    }

    std::shared_ptr<std::deque<token_t>> MacroExpander::scanArg(std::shared_ptr<std::deque<token_t>> arg) {
        auto expander = makeExpander(*arg);
        std::shared_ptr<std::deque<token_t>> newArg = std::make_shared<std::deque<token_t>>();
        while (auto token = expander->next()) {
            newArg->push_back(token);
        }
        return newArg;
    }

    std::deque<token_t>
    MacroExpander::subBody(const FunctionMacro &macro,
                           const std::vector<std::shared_ptr<std::deque<token_t>>> &args) {
        const auto &params = macro.params();
        auto l = params.size();

        bool hasVAARGS = l > 0 && params[l-1] == "__VA_ARGS__";
        if (l == 0 && args.size() == 1 &&
                    ((args[0]->size() == 1 && (*args[0])[0]->type() == Token::WHITESPACE) ||
                    args[0]->size() == 0)) {
        } else if (hasVAARGS? args.size() < l-1: args.size() != l) {
            throw ParsingException("Too few args", getPos());
        }

        bool ws = false;
        std::deque<token_t> result;

        for (auto token: macro.body()) {
            if (token->type() == Token::IDENTIFIER) {
                if (token->value() == "__VA_ARGS__") {
                    if (!hasVAARGS)
                        throw ParsingException("Unexpected __VA_ARGS__", token->pos());
                    auto comma = std::make_shared<Token>(Token::PUNC, ",", PosInfo(token->pos()));
                    for (auto i = l-1; i<args.size(); i++) {
                        if (i > l-1) {
                            appendToken(result, comma, ws);
                            ws = true;
                        }
                        appendTokens(result, args[i], ws);
                    }
                } else {
                    int i = findParam(params, token->value());
                    if (i >= 0) {
                        appendTokens(result, args[i], ws);
                    } else {
                        appendToken(result, token, ws);
                    }
                }
            } else {
                appendToken(result, token, ws);
            }
        }
        return result;
    }
}