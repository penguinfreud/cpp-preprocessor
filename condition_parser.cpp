#include "preprocessor.h"

namespace cpp {
    namespace {
        inline void checkFp(char c, const PosInfo &pos) {
            if (c == '.')
                throw ParsingException("Floating point number is not allowed", pos);
        }

        MacroValue parseInt(const std::string &v, const PosInfo &pos) {
            unsigned long x = 0;
            int p = 0;
            auto l = v.size();
            char c = v[p];
            checkFp(c, pos + p);
            if (c == '0') {
                p++;
                if (p < l) {
                    c = v[p];
                    checkFp(c, pos + p);
                    if (c == 'x' || c == 'X') {
                        p++;
                        while (p < l) {
                            c = v[p];
                            if (c == 'U' || c == 'L')
                                break;
                            if (!isHexDigit(c))
                                unexpected(c, pos + p);
                            x = (x << 4) + hexDigit(c);
                            p++;
                        }
                    } else if (c == 'b' || c == 'B') {
                        p++;
                        while (p < l) {
                            c = v[p];
                            if (c == 'U' || c == 'L')
                                break;
                            if (c != '0' && c != '1')
                                unexpected(c, pos + p);
                            int d = c - '0';
                            x = (x << 1) + d;
                            p++;
                        }
                    } else {
                        while (p < l) {
                            c = v[p];
                            if (c == 'U' || c == 'L')
                                break;
                            if (c == '\'') {
                                p++;
                                if (p >= l)
                                    throw ParsingException("Expected digit", pos + p);
                                c = v[p];
                            }
                            if (!isOctDigit(c))
                                unexpected(c, pos + p);
                            int d = c - '0';
                            x = (x << 3) + d;
                            p++;
                        }
                    }
                }
            } else if (c >= '0' && c <= '9') {
                x += c - '0';
                p++;
                while (p < l) {
                    c = v[p];
                    checkFp(c, pos + p);
                    if (c == 'U' || c == 'L')
                        break;
                    if (c < '0' || c > '9')
                        unexpected(c, pos + p);
                    int d = c - '0';
                    x = x * 10 + d;
                    p++;
                }
            } else {
                unexpected(c, pos + p);
            }
            return MacroValue(x);
        }

        MacroValue parseCharacter(const std::string &v, const PosInfo &pos) {
            unsigned long x = 0;
            unsigned long p = 0;
            auto l = v.size();
            char c = v[p];
            int bits = 1;
            if (c == 'u') {
                bits = 2;
                p++;
            } else if (c == 'U' || c == 'L') {
                bits = 4;
                p++;
            }
            p++;
            while (p < l) {
                c = v[p];
                int d = 0;
                if (c == '\'') {
                    break;
                } else if (c == '\\') {
                    p++;
                    c = v[p];
                    if (c == 'a') {
                        d = '\a';
                    } else if (c == 'b') {
                        d = '\b';
                    } else if (c == 'f') {
                        d = '\f';
                    } else if (c == 'n') {
                        d = '\n';
                    } else if (c == 'r') {
                        d = '\r';
                    } else if (c == 't') {
                        d = '\t';
                    } else if (c == 'v') {
                        d = '\v';
                    } else if (c == 'u') {
                        if (bits < 2)
                            throw ParsingException("\\u escape not allowed", pos + p);
                        p++;
                        d = hexDigit(v[p]) << 12 |
                            hexDigit(v[p+1]) << 8 |
                            hexDigit(v[p+2]) << 4 |
                            hexDigit(v[p+3]);
                        p += 4;
                    } else if (c == 'U') {
                        if (bits < 4)
                            throw ParsingException("\\U escape not allowed", pos + p);
                        p++;
                        d = hexDigit(v[p]) << 28 |
                            hexDigit(v[p+1]) << 24 |
                            hexDigit(v[p+2]) << 20 |
                            hexDigit(v[p+3]) << 16 |
                            hexDigit(v[p+4]) << 12 |
                            hexDigit(v[p+5]) << 8 |
                            hexDigit(v[p+6]) << 4 |
                            hexDigit(v[p+7]);
                        p += 8;
                    } else if (c == 'x') {
                        d = 0;
                        int b = 0;
                        while (p < l) {
                            c = v[p];
                            if (!isHexDigit(c))
                                break;
                            d = d << 4 | hexDigit(c);
                            b += 2;
                            if (b > bits)
                                throw ParsingException("Invalid escape", pos + p);
                            p++;
                        }
                    } else if (isOctDigit(c)) {
                        d = c - '0';
                        p++;
                        if (p < l) {
                            c = v[p];
                            if (isOctDigit(c)) {
                                d = (d << 3) | (c - '0');
                                p++;
                                if (p < l) {
                                    c = v[p];
                                    if (isOctDigit(c)) {
                                        d = (d << 3) | (c - '0');
                                        p++;
                                    }
                                }
                            }
                        }
                    } else {
                        unexpected(c, pos + p);
                    }
                } else {
                    d = c;
                }
                x = (x << bits) | d;
            }
            return MacroValue(x);
        }
    }

    MacroValue ConditionParser::parsePrimary() {
        auto token = next();
        if (token) {
            if (token->type() == Token::NUMBER) {
                return parseInt(token->value(), token->pos());
            } else if (token->type() == Token::CHARACTER) {
                return parseCharacter(token->value(), token->pos());
            } else if (token->type() == Token::IDENTIFIER) {
                if (token->value() == "true") {
                    return MacroValue(1UL);
                } else if (token->value() == "defined") {
                    token = __next(false);
                    bool paren = false;
                    if (token && token->type() == Token::WHITESPACE && !token->hasNewLine()) {
                        token = __next(false);
                    }
                    if (token && token->type() == Token::PUNC &&
                        token->value() == "(") {
                        paren = true;
                        token = __next(false);
                        if (token->type() == Token::WHITESPACE && !token->hasNewLine()) {
                            token = __next(false);
                        }
                    }
                    if (!token || token->type() != Token::IDENTIFIER)
                        throw ParsingException("Expected identifier", token->pos());
                    long v;
                    if ((*macroTable())[token->value()])
                        v = 1;
                    else
                        v = 0;
                    if (paren) {
                        token = __next(false);
                        if (token && token->type() == Token::WHITESPACE && !token->hasNewLine()) {
                            token = __next(false);
                        }
                        if (!token || token->type() != Token::PUNC ||
                            token->value() != ")") {
                            throw ParsingException("Expected )", token? token->pos() : getPos());
                        }
                    }
                    return MacroValue(v);
                } else {
                    return MacroValue(0UL);
                }
            }
        }
        throw ParsingException(("Unexpected " + token->value()).c_str(), token? token->pos(): getPos());
    }

    MacroValue ConditionParser::parseUnary() {
        while (matchPunc('+')) {}
        if (matchPunc('-'))
            return -parseUnary();
        else if (matchPunc('~'))
            return ~parseUnary();
        else if (matchPunc('!') || matchId("not"))
            return !parseUnary();
        else
            return parsePrimary();
    }

    MacroValue ConditionParser::parseMultiply() {
        auto v = parseUnary();
        while (!finished()) {
            if (matchPunc('*'))
                v = v * parseUnary();
            else if (matchPunc('/')) {
                const auto &p = getPos();
                auto w = parseUnary();
                if (w.isUnsigned) {
                    if (w.v.ul == 0)
                        throw ParsingException("Divide by zero", p);
                } else {
                    if (w.v.l == 0)
                        throw ParsingException("Divide by zero", p);
                }
                v = v / parseUnary();
            } else if (matchPunc('%'))
                v = v % parseUnary();
            else
                break;
        }
        return v;
    }

    MacroValue ConditionParser::parseAdd() {
        auto v = parseMultiply();
        while (!finished()) {
            if (matchPunc('+'))
                v = v + parseMultiply();
            else if (matchPunc('-'))
                v = v - parseMultiply();
            else
                break;
        }
        return v;
    }

    MacroValue ConditionParser::parseShift() {
        auto v = parseAdd();
        while (!finished()) {
            if (matchPunc("<<"))
                v = v << parseAdd();
            else if (matchPunc(">>"))
                v = v >> parseAdd();
            else
                break;
        }
        return v;
    }

    MacroValue ConditionParser::parseRelation() {
        auto v = parseShift();
        while (!finished()) {
            if (matchPunc('<'))
                v = v < parseShift();
            else if (matchPunc("<="))
                v = v <= parseShift();
            else if (matchPunc('>'))
                v = v > parseShift();
            else if (matchPunc(">="))
                v = v > parseShift();
            else
                break;
        }
        return v;
    }

    MacroValue ConditionParser::parseEquality() {
        auto v = parseRelation();
        while (!finished()) {
            if (matchPunc("==") || matchId("eq"))
                v = v == parseRelation();
            else if (matchPunc("!=") || matchId("not_eq"))
                v = v != parseRelation();
            else
                break;
        }
        return v;
    }

    MacroValue ConditionParser::parseBitwiseAnd() {
        auto v = parseEquality();
        while (matchPunc('&') || matchId("bitand"))
            v = v & parseEquality();
        return v;
    }

    MacroValue ConditionParser::parseXor() {
        auto v = parseBitwiseAnd();
        while (matchPunc('^') || matchId("xor"))
            v = v ^ parseBitwiseAnd();
        return v;
    }

    MacroValue ConditionParser::parseBitwiseOr() {
        auto v = parseXor();
        while (matchPunc('|') || matchId("bitor"))
            v = v | parseXor();
        return v;
    }

    MacroValue ConditionParser::parseAnd() {
        auto v = parseBitwiseOr();
        while (matchPunc("&&") || matchId("and"))
            v = v && parseBitwiseOr();
        return v;
    }

    MacroValue ConditionParser::parseOr() {
        auto v = parseAnd();
        while (matchPunc("||") || matchId("or"))
            v = v || parseAnd();
        return v;
    }

    MacroValue ConditionParser::parseConditional() {
        auto cond = parseOr();
        if (matchPunc('?')) {
            auto seq = parseConditional();
            expectPunc(':');
            auto alt = parseConditional();
            return cond? seq: alt;
        } else {
            return cond;
        }
    }

    MacroValue ConditionParser::parse() {
        auto v = parseConditional();
        while (matchPunc(',')) {
            v = parseConditional();
        }
        return v;
    }
}
