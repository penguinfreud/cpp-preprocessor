#include <sstream>
#include "preprocessor.h"

namespace cpp {
    char PP_PUNCS[PP_PUNCS_COUNT][4] = {
        "->*", "%:%", "...", ">>=", "<<=", "##", "<:", ":>",
        "<%", "%>", "%:", "::", ".*", "+=", "-=", "*=", "/=",
        "%=", "^=", "&=", "|=", "<<", ">>", "==", "!=", "<=",
        ">=", "&&", "||", "++", "--", "->", "{", "}", "[",
        "]", "#", "(", ")", ";", ":", "?", ".", "+", "-",
        "*", "/", "%", "^", "&", "|", "~", "!", "=", "<", ">", ","
    };

    void Tokenizer::spliceLine() {
        auto _pos(pos);
        if (input->peek() == '\\') {
            advanceRaw();
            int c = input->peek();
            if (c == '\r') {
                advanceRaw();
                pos.newLine();
                if (input->peek() == '\n') {
                    input->ignore();
                    advanceRaw();
                }
            } else if (c == '\n') {
                advanceRaw();
                pos.newLine();
            } else {
                pos = _pos;
                input->unget();
                input->clear();
            }
        }
    }

    void Tokenizer::advanceRaw() {
        int c = input->get();
        if (c == '\r') {
            hasReturn = true;
            pos.newLine();
        } else if (c == '\n') {
            if (!hasReturn) {
                pos.newLine();
            } else {
                hasReturn = false;
            }
        } else {
            pos.col++;
            hasReturn = false;
        }
        pos.pos++;
    }

    bool Tokenizer::match(const std::string &s, bool output, bool raw) {
        auto p = input->tellg();
        auto _pos(pos);
        for (auto c: s) {
            if (raw? !matchRaw(c, false): !match(c, false)) {
                input->clear();
                input->seekg(p);
                pos = _pos;
                return false;
            }
        }
        if (output)
            tokenBuffer << s;
        return true;
    }

    token_t Tokenizer::parseSpace() {
        bool hasNewLine = false;
        while (!input->eof()) {
            int c;
            if (match("/*", false)) {
                tokenBuffer << ' ';
                while (!match("*/", false)) {
                    if (input->eof()) {
                        throw ParsingException("Unterminated comment", pos);
                    }
                    advance();
                }
            } else if (match("//", false)) {
                tokenBuffer << ' ';
                while (true) {
                    if (input->eof())
                        break;
                    c = input->peek();
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
            return token_t();
        } else {
            return std::make_shared<Token>(Token::WHITESPACE, tokenBuffer.str(), startPos, hasNewLine);
        }
    }

    token_t Tokenizer::parseId() {
        while (!input->eof()) {
            int c = input->peek();
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
        auto p = input->tellg();
        int c = input->peek();
        if (c == '.') {
            advance();
            c = input->peek();
            if (isdigit(c)) {
                tokenBuffer << '.' << (char) c;
                advance();
            } else {
                input->clear();
                input->seekg(p);
                pos = startPos;
                return parsePunc();
            }
        } else if (isdigit(c)) {
            tokenBuffer << (char) c;
            advance();
        } else {
            throw ParsingException("Expected digit", pos);
        }
        while (!input->eof()) {
            if (match('E') || match('e')) {
                if (!match('+'))
                    match('-');
            } else if (match('\'')) {
                c = input->peek();
                if (isIdChar(c)) {
                    tokenBuffer << (char) c;
                    advance();
                } else {
                    unexpected((char) c, pos);
                }
            } else if (isIdChar(input->peek())) {
                tokenBuffer << (char) input->peek();
                advance();
            } else {
                break;
            }
        }
        return std::make_shared<Token>(Token::NUMBER, tokenBuffer.str(), startPos);
    }

    token_t Tokenizer::parsePunc() {
        for (int i = 0; i<PP_PUNCS_COUNT; i++) {
            if (match(PP_PUNCS[i], false)) {
                return std::make_shared<Token>(Token::PUNC, PP_PUNCS[i], startPos);
            }
        }
        std::string s;
        s.push_back((char) input->peek());
        advance();
        return std::make_shared<Token>(Token::PUNC, s, startPos);
    }

    void Tokenizer::hex() {
        int c = input->peek();
        if (isHexDigit(c)) {
            advance();
            tokenBuffer << (char) c;
        } else {
            throw ParsingException("Expected hexadecimal digit", pos);
        }
    }

    void Tokenizer::parseEscape() {
        int c;
        if (match('\'') || match('\"') || match('?') || match('\\') ||
            match('a') || match('b') || match('f') || match('n') ||
            match('r') || match('t') || match('v')) {
        } else if (match('u')) {
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
        } else if (match('x')) {
            c = input->peek();
            if (isHexDigit(c)) {
                do {
                    advance();
                    tokenBuffer << (char) c;
                } while (isHexDigit(c));
            } else {
                unexpected((char) c, pos);
            }
        } else {
            c = input->peek();
            if (isOctDigit(c)) {
                oct();
                oct();
                oct();
            } else {
                unexpected((char) c, pos);
            }
        }
    }

    token_t Tokenizer::parseCharSequence(char quote, Token::token_type type) {
        if (!match(quote))
            throw ParsingException("Expected " + quote, pos);
        while (!input->eof()) {
            if (match('\\')) {
                parseEscape();
            } else if (match(quote)) {
                return std::make_shared<Token>(type, tokenBuffer.str(), startPos);
            } else if (match('\r', false) || match('\n', false)) {
                throw ParsingException("Unterminated string", pos);
            } else {
                tokenBuffer << (char) input->peek();
                advance();
            }
        }
        throw ParsingException("Unterminated string", pos);
    }

    token_t Tokenizer::parseRawString() {
        int c;
        std::stringstream ss;
        if (!matchRaw('\"', true))
            throw ParsingException("Expected \"", pos);
        while (!input->eof()) {
            if (matchRaw('(', true)) {
                auto indicator = ")" + ss.str() + "\"";
                while (!input->eof()) {
                    if (match(indicator, true, true)) {
                        return std::make_shared<Token>(Token::STRING, tokenBuffer.str(), startPos);
                    } else {
                        tokenBuffer << (char) input->peek();
                        advanceRaw();
                    }
                }
            } else if ((c = matchRaw(' ', false) ||
                            matchRaw(')', false) ||
                            matchRaw('\\', false) ||
                            matchRaw('\t', false) ||
                            matchRaw('\f', false) ||
                            matchRaw('\r', false) ||
                            matchRaw('\n', false))) {
                unexpected((char) c, pos);
            } else {
                c = input->peek();
                tokenBuffer << (char) c;
                ss << (char) c;
                advanceRaw();
            }
        }
        throw ParsingException("Unterminated raw string", pos);
    }

    token_t Tokenizer::_next() {
        int c = input->peek();
        if (input->eof())
            return token_t();

        startToken();
        auto space = parseSpace();
        if (space)
            return space;

        auto p = input->tellg();
        if (input->peek() == '.' || isdigit(input->peek())) {
            return parseNumber();
        } else if (match('u') || match('U') || match('L') || match('R')) {
            bool needString = false, isRaw;
            if (c == 'R') {
                isRaw = true;
            } else {
                needString = c == 'u' && match('8');
                isRaw = match('R') > 0;
            }
            c = input->peek();
            if (c == '"') {
                return isRaw? parseRawString(): parseCharSequence((char) c, Token::STRING);
            } else if (c == '\'') {
                if (isRaw || needString)
                    throw ParsingException("Expected \"", pos);
                return parseCharSequence((char) c, Token::CHARACTER);
            } else {
                pos = startPos;
                input->clear();
                input->seekg(p);
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
}