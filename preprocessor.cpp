#include <iostream>
#include <fstream>
#include <sstream>
#include "preprocessor.h"

namespace cpp {
    void Tokenizer::spliceLine() {
        if (input.peek() == '\\') {
            input.ignore();
            int c = input.peek();
            if (c == '\r') {
                input.ignore();
                pos.pos += 2;
                newLine();
                if (input.peek() == '\n') {
                    input.ignore();
                    pos.pos++;
                }
            } else if (c == '\n') {
                input.ignore();
                pos.pos += 2;
                newLine();
            } else {
                input.unget();
                input.clear();
            }
        }
    }

    void Tokenizer::advance(bool raw) {
        int c = input.get();
        if (c == '\r') {
            hasReturn = true;
            newLine();
        } else if (c == '\n') {
            if (!hasReturn) {
                newLine();
                hasReturn = false;
            }
        } else {
            pos.col++;
            hasReturn = false;
        }
        pos.pos++;
        if (!raw)
            spliceLine();
    }

    bool Tokenizer::match(const std::string &s, bool output, bool raw) {
        auto p = input.tellg();
        auto buf = tokenBuffer.str();
        auto bufP = tokenBuffer.tellp();
        auto pi(pos);
        for (auto c: s) {
            if (!match(c, output, raw)) {
                input.clear();
                input.seekg(p);
                pos = pi;
                tokenBuffer.str(buf);
                tokenBuffer.seekp(bufP);
                return false;
            }
        }
        return true;
    }

    token_t Tokenizer::parseSpace() {
        bool hasNewLine = false;
        while (!input.eof()) {
            int c;
            if (match("/*", false)) {
                tokenBuffer << ' ';
                while (!match("*/", false)) {
                    if (input.eof()) {
                        throw "Unterminated comment";
                    }
                    advance();
                }
            } else if (match("//", false)) {
                tokenBuffer << ' ';
                while (true) {
                    if (input.eof())
                        break;
                    c = input.peek();
                    if (c == '\r' || c == '\n')
                        break;
                    advance();
                }
            } else if (match('\r')) {
                hasNewLine = true;
            } else if (match('\n')) {
                hasNewLine = true;
            } else if (match(' ') || match('\t')) {
            } else {
                break;
            }
        }
        auto str = tokenBuffer.str();
        if (str.empty()) {
            return token_t(nullptr);
        } else {
            return token_t(new Token(Token::WHITESPACE, tokenBuffer.str(), startPos, hasNewLine));
        }
    }

    token_t Tokenizer::parseId() {
        while (!input.eof()) {
            int c = input.peek();
            if (isalpha(c) || isdigit(c) || c == '_') {
                tokenBuffer << (char)c;
                advance();
            } else {
                break;
            }
        }
        return token_t(new Token(Token::IDENTIFIER, tokenBuffer.str(), startPos));
    }

    token_t Tokenizer::parseNumber() {
        auto p = input.tellg();
        int c = input.peek();
        if (c == '.') {
            advance();
            c = input.peek();
            if (isdigit(c)) {
                tokenBuffer << '.' << (char) c;
                advance();
            } else {
                input.clear();
                input.seekg(p);
                pos = startPos;
                return parsePunc();
            }
        } else if (isdigit(c)) {
            tokenBuffer << (char) c;
            advance();
        } else {
            throw "Expected digit";
        }
        while (!input.eof()) {
            if (match('E') || match('e')) {
                if (!match('+'))
                    match('-');
            } else if (match('\'')) {
                c = input.peek();
                if (isIdChar(c)) {
                    tokenBuffer << (char) c;
                    advance();
                } else {
                    throw "Unexpected " + (char) c;
                }
            } else if (isIdChar(c)) {
                tokenBuffer << (char) c;
                advance();
            } else {
                break;
            }
        }
        return token_t(new Token(Token::NUMBER, tokenBuffer.str(), startPos));
    }

    token_t Tokenizer::parsePunc() {
        for (int i = 0; i<PP_PUNCS_COUNT; i++) {
            if (match(PP_PUNCS[i]), false) {
                return token_t(new Token(Token::PUNC, PP_PUNCS[i], startPos));
            }
        }
        int c = input.peek();
        advance();
        return token_t(new Token(Token::PUNC, (char*)&c, startPos));
    }

    void Tokenizer::parseEscape() {
        int c;
        if (match('\'') || match('\"') || match('?') || match('\\') ||
                match('a') || match('b') || match('f') || match('n') ||
                match('r') || match('t') || match('v')) {
        } else if (match('x') || match('u')) {
            hex();
            hex();
            hex();
            hex();
        } else if (match('U')) {
            hex();
            hex();
            hex();
            hex();
            hex();
            hex();
            hex();
            hex();
        } else {
            c = input.peek();
            if (c >= '0' && c <= '7') {
                oct();
                oct();
                oct();
            } else {
                throw "Unexpected " + (char) c;
            }
        }
    }

    token_t Tokenizer::parseCharSequence(char quote, Token::token_type type) {
        if (!match(quote))
            throw "Expected " + quote;
        while (!input.eof()) {
            if (match('\\')) {
                parseEscape();
            } else if (match(quote)) {
                return token_t(new Token(type, tokenBuffer.str(), startPos));
            } else if (match('\r', false) || match('\n', false)) {
                throw "Unterminated string";
            } else {
                tokenBuffer << (char) input.peek();
                advance();
            }
        }
        throw "Unterminated string";
    }
    
    token_t Tokenizer::parseRawString() {
        int c;
        std::stringstream ss;
        if (!match('\"', true, true))
            throw "Expected \"";
        while (!input.eof()) {
            if (match('(', true, true)) {
                auto indicator = ")" + ss.str() + "\"";
                while (!input.eof()) {
                    if (match(indicator, true, true)) {
                        return token_t(new Token(Token::STRING, tokenBuffer.str(), startPos));
                    } else {
                        tokenBuffer << (char) input.peek();
                        advance(true);
                    }
                }
            } else if ((c = match(' ', false, true) ||
                    match(')', false, true) ||
                    match('\\', false, true) ||
                    match('\t', false, true) ||
                    match('\f', false, true) ||
                    match('\r', false, true) ||
                    match('\n', false, true))) {
                throw "Unexpected " + escape(c);
            } else {
                c = input.peek();
                tokenBuffer << (char) c;
                ss << (char) c;
                advance(true);
            }
        }
        throw "Unterminated raw string";
    }

    token_t Tokenizer::_next() {
        int c = input.peek();
        if (input.eof())
            return token_t(nullptr);

        startToken();
        auto space = parseSpace();
        if (space)
            return space;

        auto p = input.tellg();
        if (input.peek() == '.' || isdigit(input.peek())) {
            return parseNumber();
        } else if (match('u') || match('U') || match('L') || match('R')) {
            bool needString = false, isRaw = false;
            if (c == 'R') {
                isRaw = true;
            } else {
                if (c == 'u' && match('8')) {
                    needString = true;
                }
                if (match('R')) {
                    isRaw = true;
                }
            }
            c = input.peek();
            if (c == '"') {
                return isRaw? parseRawString(): parseCharSequence((char) c, Token::STRING);
            } else if (c == '\'') {
                if (isRaw || needString)
                    throw "Expected \"";
                return parseCharSequence((char) c, Token::CHARACTER);
            } else {
                pos = startPos;
                input.clear();
                input.seekg(p);
                tokenBuffer.str("");
                return parseId();
            }
        } else if (c == '"') {
            return parseCharSequence('"', Token::STRING);
        } else if (c == '\'') {
            return parseCharSequence('\'', Token::CHARACTER);
        } else if (isalpha(c) || c == '_') {
            return parseId();
        }
        return parsePunc();
    }

    token_t MacroExpander::_next() {
        if (expander) {
            auto token = expander->next();
            if (token) {
                return token;
            } else {
                expander = nullptr;
            }
        }
        auto token = input->next();
        if (token && token->type() == Token::IDENTIFIER) {
            return expandMacro(token);
        }
        return token;
    }

    token_t MacroExpander::expandMacro(token_t name) {
        if (name->value() == "__VA_ARGS__")
            throw "Unexpected __VA_ARGS__";
        if (stack->hasName(name->value())) {
            auto macro = (*macroTable)[name->value()];
            if (macro) {
                if (macro->isFunctionLike()) {
                    return expandFunctionMacro(name, *std::dynamic_pointer_cast<FunctionMacro>(macro));
                } else {
                    return expandObjectMacro(*macro);
                }
            }
        }
        return name;
    }

    token_t MacroExpander::expandObjectMacro(const Macro &macro) {
        if (!macro.empty()) {
            auto stream = new TokenStream();
            stream->setBuffer(macro.body());
            expander = new MacroExpander(stream, macroTable, new MacroStack(macro.name(), stack));
        }
        return _next();
    }

    token_t MacroExpander::expandFunctionMacro(token_t name, const FunctionMacro &macro) {
        auto token = input->space();
        if (input->matchPunc("(")) {
            input->space();
            std::vector<std::shared_ptr<std::deque<token_t>>> args;
            std::shared_ptr<std::deque<token_t>> curArg(new std::deque);
            int depth = 0;
            while (!input->finished()) {
                if ((token = input->matchPunc(")"))) {
                    if (depth == 0) {
                        args.push_back(scanArg(curArg));

                        auto stream = new TokenStream();
                        stream->setBuffer(*subBody(macro, args));
                        expander = new MacroExpander(stream, macroTable, new MacroStack(macro.name(), stack));
                        return _next();
                    } else {
                        depth--;
                        curArg->push_back(token);
                    }
                } else if ((token = input->matchPunc("("))) {
                    depth++;
                    curArg->push_back(token);
                } else if (depth == 0 && input->matchPunc(",")) {
                    input->space();
                    args.push_back(scanArg(curArg));
                } else {
                    curArg->push_back(input->next());
                }
            }
        } else {
            if (token)
                input->unget(token);
            return name;
        }
    }

    std::shared_ptr<std::deque<token_t>> MacroExpander::scanArg(std::shared_ptr<std::deque<token_t>> curArg) {

    }

    std::shared_ptr<std::deque<token_t>> MacroExpander::subBody(const FunctionMacro &macro,
                                                                const std::vector<std::shared_ptr<std::deque<token_t>>> &args) {
        
    }
}

inline void assert(bool cond) {
    if (!cond)
        throw "Assertion failed";
}

int main(int argc, char **argv) {
    /*std::ifstream ifs;
    ifs.open("/home/wsy/courses/OOP/lab3/lab3.h");*/
    using namespace cpp;
    std::stringstream ss;
    ss.str("a/*foo\n*/b //\n");
    try {
        {
            Tokenizer tokenizer(ss, "file");
            auto tok = tokenizer.next();
            assert(tok->type() == Token::IDENTIFIER);
            assert(tok->value() == "a");
            tok = tokenizer.next();
            assert(tok->type() == Token::WHITESPACE);
            assert(tok->value() == " ");
            assert(!tok->hasNewLine());
            tok = tokenizer.next();
            assert(tok->type() == Token::IDENTIFIER);
            assert(tok->value() == "b");
            tok = tokenizer.next();
            assert(tok->type() == Token::WHITESPACE);
            assert(tok->value() == "  \n");
            assert(tok->hasNewLine());
            assert(!tokenizer.next());
        }
        ss.clear();
        ss.str("''u'cd'U'\\000''\\n'U\"aaaa\\\"\"u8R\"/*(\nfoo)/*\"");
        {
            Tokenizer tokenizer(ss, "file");
            auto token = tokenizer.next();
            assert(token->type() == Token::CHARACTER);
            assert(token->value() == "''");
            token = tokenizer.next();
            assert(token->type() == Token::CHARACTER);
            assert(token->value() == "u'cd'");
            token = tokenizer.next();
            assert(token->type() == Token::CHARACTER);
            assert(token->value() == "U'\\000'");
            token = tokenizer.next();
            assert(token->type() == Token::CHARACTER);
            assert(token->value() == "'\\n'");
            token = tokenizer.next();
            assert(token->type() == Token::STRING);
            assert(token->value() == "U\"aaaa\\\"\"");
            token = tokenizer.next();
            assert(token->type() == Token::STRING);
            assert(token->value() == "u8R\"/*(\nfoo)/*\"");
            token = tokenizer.next();
            assert(!token);
        }
    } catch (char const *e) {
        std::cout << e << std::endl;
    }
}