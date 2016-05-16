#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
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
            return std::make_shared<Token>(Token::WHITESPACE, tokenBuffer.str(), startPos, hasNewLine);
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
        return std::make_shared<Token>(Token::IDENTIFIER, tokenBuffer.str(), startPos);
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
        return std::make_shared<Token>(Token::NUMBER, tokenBuffer.str(), startPos);
    }

    token_t Tokenizer::parsePunc() {
        for (int i = 0; i<PP_PUNCS_COUNT; i++) {
            if (match(PP_PUNCS[i]), false) {
                return std::make_shared<Token>(Token::PUNC, PP_PUNCS[i], startPos);
            }
        }
        int c = input.peek();
        advance();
        return std::make_shared<Token>(Token::PUNC, (char*)&c, startPos);
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
                return std::make_shared<Token>(type, tokenBuffer.str(), startPos);
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
                        return std::make_shared<Token>(Token::STRING, tokenBuffer.str(), startPos);
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
        if ((!stack || !(stack->hasName(name->value()))) && macroTable) {
            auto macro = (*macroTable)[name->value()];
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

    token_t MacroExpander::expandObjectMacro(const Macro &macro) {
        if (!macro.empty()) {
            auto stream = std::make_shared<TokenStream>(macro.body());
            expander = std::make_shared<MacroExpander>(stream, macroTable, std::make_shared<MacroStack>(macro.name(), stack));
        }
        return _next();
    }

    token_t MacroExpander::expandFunctionMacro(token_t name, const FunctionMacro &macro) {
        auto token = input->space();
        if (input->matchPunc('(')) {
            input->space();
            std::vector<std::shared_ptr<std::deque<token_t>>> args;
            auto curArg = std::make_shared<std::deque<token_t>>();
            int depth = 0;
            while (!input->finished()) {
                if ((token = input->matchPunc(')'))) {
                    if (depth == 0) {
                        args.push_back(scanArg(curArg));

                        auto stream = std::make_shared<TokenStream>(subBody(macro, args));
                        expander = std::make_shared<MacroExpander>(stream, macroTable, std::make_shared<MacroStack>(macro.name(), stack));
                        return _next();
                    } else {
                        depth--;
                        curArg->push_back(token);
                    }
                } else if ((token = input->matchPunc('('))) {
                    depth++;
                    curArg->push_back(token);
                } else if (depth == 0 && input->matchPunc(',')) {
                    input->space();
                    args.push_back(scanArg(curArg));
                    curArg = std::make_shared<std::deque<token_t>>();
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

    std::shared_ptr<std::deque<token_t>> MacroExpander::scanArg(std::shared_ptr<std::deque<token_t>> arg) {
        auto stream = std::make_shared<TokenStream>(*arg);
        MacroExpander expander(stream, macroTable, stack);
        std::shared_ptr<std::deque<token_t>> newArg;
        while (auto token = expander.next()) {
            newArg->push_back(token);
        }
        return newArg;
    }

    namespace {
        void appendToken(std::deque<token_t> &list, token_t token, bool &ws) {
            if (token->type() == Token::WHITESPACE) {
                ws = true;
            } else {
                if (ws) {
                    list.push_back(std::make_shared<Token>(Token::WHITESPACE, " ", token->pos()));
                }
                list.push_back(token);
            }
        }

        void appendTokens(std::deque<token_t> &list, std::shared_ptr<std::deque<token_t>> append, bool &ws) {
            for (auto token: *append) {
                appendToken(list, token, ws);
            }
        }

        int findParam(const std::deque<std::string> &params, const std::string &name) {
            for (int i = 0; i<params.size(); i++) {
                if (params[i] == name)
                    return i;
            }
            return -1;
        }
    }

    std::deque<token_t>
        MacroExpander::subBody(const FunctionMacro &macro,
                               const std::vector<std::shared_ptr<std::deque<token_t>>> &args) {
        const auto &params = macro.params();
        auto l = params.size();

        bool hasVAARGS = params[l-1] == "__VA_ARGS__";
        if (hasVAARGS? args.size() < l-1: args.size() != l) {
            throw "Too few args";
        }

        bool ws = false;
        std::deque<token_t> result;

        for (auto token: macro.body()) {
            if (token->type() == Token::IDENTIFIER) {
                if (token->value() == "__VA_ARGS__") {
                    if (!hasVAARGS)
                        throw "Unexpected __VA_ARGS__";
                    for (auto i = l-1; i<args.size(); i++) {
                        if (i > l-1) {
                            auto comma = std::make_shared<Token>(Token::PUNC, ",", PosInfo(token->pos()));
                            appendToken(result, comma, ws);
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

    namespace {
        token_t truncateLine(token_t token) {
            const std::string &v = token->value();
            unsigned long pos;
            for (auto i = v.size()-1; i>=0; --i) {
                if (v[i] == '\n') {
                    if (i > 0 && v[i-1] == '\r') {
                        pos = i-1;
                    } else {
                        pos = i;
                    }
                    break;
                } else if (v[i] == '\r') {
                    pos = i;
                    break;
                }
            }
            return std::make_shared<Token>(Token::WHITESPACE, v.substr(pos), token->pos());
        }
    }

    token_t DirectiveParser::_next() {
        if (lineStart && input->matchPunc('#')) {
            lineStart = false;
            input->space(false);
            if (input->matchId("define")) {
                input->space(false);
                return parseDefine();
            } else if (input->matchId("undef")) {
                input->space(false);
                return parseUndef();
            } else if (input->matchId("if")) {
                return parseIf(false);
            } else if (input->matchId("ifdef")) {
                return parseIf(true, false);
            } else if (input->matchId("ifndef")) {
                return parseIf(true, true);
            } else if (input->matchId("elif")) {

            } else if (input->matchId("else")) {

            } else if (input->matchId("endif")) {

            } else {
                skipLine();
            }
        }
    }

    token_t DirectiveParser::skipLine() {
        while (auto token = input->next()) {
            if (token->type() == Token::WHITESPACE && token->hasNewLine()) {
                return truncateLine(token);
            }
        }
        return token_t(nullptr);
    }

    token_t DirectiveParser::parseDefine() { }
    token_t DirectiveParser::parseUndef() { }
    token_t DirectiveParser::parseIf(bool defined, bool neg) { }
}

inline void assert(bool cond) {
    if (!cond)
        throw "Assertion failed";
}

void testTokenizer() {
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

void testMacroExpansion() {
    using namespace cpp;
    std::stringstream ss;
    ss.str("foo");
    macro_table_t table = std::make_shared<std::map<std::string, std::shared_ptr<Macro>>>();
    auto macro = std::make_shared<Macro>("foo");
    PosInfo pi("anon");
    macro->addToken(std::make_shared<Token>(Token::NUMBER, "2", pi));
    (*table)["foo"] = macro;
    auto expander = (new Tokenizer(ss, "anon"))->expandMacro(table);
    auto token = expander->next();
    std::cout << token->type() << std::endl;
    std::cout << token->value() << std::endl;
}

class A {
public:
    ~A() {
        std::cout << "destructor" << std::endl;
    }
};

int main(int argc, char **argv) {
    /*std::ifstream ifs;
    ifs.open("/home/wsy/courses/OOP/lab3/lab3.h");*/
    //testTokenizer();
    //testMacroExpansion();
    A *pa = new A();
    auto spa1 = new std::shared_ptr<A>(pa);
    auto spa2 = new std::shared_ptr<A>(pa);
    delete spa1;
    delete spa2;
}
