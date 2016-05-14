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

    void Tokenizer::advance() {
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
        spliceLine();
    }

    bool Tokenizer::match(const std::string &s, bool output) {
        pos_t p = input.tellg();
        auto pi(pos);
        for (auto c: s) {
            if (!match(c)) {
                input.clear();
                input.seekg(p);
                pos = pi;
                return false;
            }
        }
        if (output)
            buffer << s;
        return true;
    }

    token_t Tokenizer::parseSpace() {
        bool hasNewLine = false;
        while (!input.eof()) {
            int c;
            if (match("/*", false)) {
                buffer << ' ';
                while (!match("*/", false)) {
                    if (input.eof()) {
                        throw "Unterminated comment";
                    }
                    advance();
                }
            } else if (match("//", false)) {
                buffer << ' ';
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
                match('\n');
            } else if (match('\n')) {
                hasNewLine = true;
            } else if (match(' ') || match('\t')) {
            } else {
                break;
            }
        }
        const auto &str = buffer.str();
        if (str.empty()) {
            return token_t();
        } else {
            return token_t(new Token(Token::WHITESPACE, buffer.str(), startPos, hasNewLine));
        }
    }

    token_t Tokenizer::parseId() {
        while (!input.eof()) {
            int c = input.peek();
            if (isalpha(c) || isdigit(c) || c == '_') {
                buffer << (char)c;
                advance();
            } else {
                break;
            }
        }
        return token_t(new Token(Token::IDENTIFIER, buffer.str(), startPos));
    }

    token_t Tokenizer::parseNumber() {
        pos_t p = input.tellg();
        int c = input.peek();
        if (c == '.') {
            buffer << (char) c;
            advance();
            c = input.peek();
            if (isdigit(c)) {
                buffer << (char) c;
                advance();
            } else {
                input.clear();
                input.seekg(p);
                pos = startPos;
                return parsePunc();
            }
        } else if (isdigit(c)) {
            buffer << (char) c;
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
                    buffer << (char) c;
                    advance();
                } else {
                    throw "Unexpected " + (char) c;
                }
            } else if (isIdChar(c)) {
                buffer << (char) c;
                advance();
            } else {
                break;
            }
        }
        return token_t(new Token(Token::NUMBER, buffer.str(), startPos));
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
        } else if ((c = match('x') || match('u'))) {
            hex();
            hex();
            hex();
            hex();
        } else if ((c = match('U'))) {
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
                return token_t(new Token(type, buffer.str(), startPos));
            } else if (match('\r', false) || match('\n', false)) {
                throw "Unterminated string";
            } else {
                buffer << (char) input.peek();
                advance();
            }
        }
        throw "Unterminated string";
    }
    
    token_t Tokenizer::parseRawString() {
        
    }
}

int main(int argc, char **argv) {
    /*std::ifstream ifs;
    ifs.open("/home/wsy/courses/OOP/lab3/lab3.h");*/
    using namespace cpp;
    std::stringstream ss;
    ss.str("a");
    Tokenizer tokenizer(ss, "hello");
    std::cout << tokenizer.match("abc") << std::endl;
    std::cout << ss.peek() << std::endl;
}