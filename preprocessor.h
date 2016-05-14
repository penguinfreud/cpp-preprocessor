#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <istream>
#include <vector>
#include <deque>
#include <memory>

namespace cpp {
/*
 * C++ Preprocessor Grammar
 *
 *
 * basic source character set
 *      a b c d e f g h i j k l m n o p q r s t u v w x y z
 *      A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
 *      0 1 2 3 4 5 6 7 8 9
 *      _ { } [ ] # ( ) < > % : ; . ? * + - / ^ & | ∼ ! = , \ " ’
 *
 * hex-quad:
 *      hexadecimal-digit hexadecimal-digit hexadecimal-digit hexadecimal-digit
 * universal-character-name:
 *      \u hex-quad
 *      \U hex-quad hex-quad
 * preprocessing-token:
 *      header-name
 *      identifier
 *      pp-number
 *      character-literal
 *      user-defined-character-literal
 *      string-literal
 *      user-defined-string-literal
 *      preprocessing-op-or-punc
 *      each non--white-space character that cannot be one of the above
 * header-name:
 *      < h-char-sequence >
 *      " q-char-sequence "
 * h-char-sequence:
 *      h-char
 *      h-char-sequence h-char
 * h-char:
 *      any member of the source character set except new-line and >
 * q-char-sequence:
 *      q-char
 *      q-char-sequence q-char
 * q-char:
 *      any member of the source character set except new-line and "
 * pp-number:
 *      digit
 *      . digit
 *      pp-number digit
 *      pp-number ' digit
 *      pp number ' nondigit
 *      pp-number identifier-nondigit
 *      pp-number e sign
 *      pp-number E sign
 *      pp-number .
 * sign: one of
 *      + -
 * identifier:
 *      identifier-nondigit
 *      identifier identifier-nondigit
 *      identifier digit
 * identifier-nondigit:
 *      nondigit
 *      universal-character-name
 *      other implementation-defined characters
 * nondigit: one of
 *      a b c d e f g h i j k l m
 *      n o p q r s t u v w x y z
 *      A B C D E F G H I J K L M
 *      N O P Q R S T U V W X Y Z _
 * digit: one of
 *      0 1 2 3 4 5 6 7 8 9
 * preprocessing-op-or-punc: one of
 *      {       }       [       ]       #       ##      (       )
 *      <:      :>      <%      %>      %:      %:%     ;       :       ...
 *      new     delete  ?       ::      .       .*
 *      +       -       *       /       %       ^       &       |       ~
 *      !       =       <       >       +=      -=      *=      /=      %=
 *      ^=      &=      |=      <<      >>      >>=     <<=     ==      !=
 *      <=      >=      &&      ||      ++      --      ,       ->*      ->
 *      and     and_eq  bitand  bitor   compl   not     not_eq
 *      or      or_eq   xor     xor_eq
 * octal-digit: one of
 *      0 1 2 3 4 5 6 7
 * hexadecimal-digit: one of
 *      0 1 2 3 4 5 6 7 8 9
 *      a b c d e f
 *      A B C D E F
 * character-literal:
 *      ' c-char-sequence '
 *      u' c-char-sequence '
 *      U' c-char-sequence '
 *      L' c-char-sequence '
 * c-char-sequence:
 *      c-char
 *      c-char-sequence c-char
 * c-char:
 *      any member of the source character set except
 *          the single-quote ', backslash \, or new-line character
 *      escape-sequence
 *      universal-character-name
 * escape-sequence:
 *      simple-escape-sequence
 *      octal-escape-sequence
 *      hexadecimal-escape-sequence
 * simple-escape-sequence: one of
 *      \'  \"" \?  \\
 *      \a  \b  \f  \n  \r  \t  \v
 * octal-escape-sequence:
 *      \ octal-digit
 *      \ octal-digit octal-digit
 *      \ octal-digit octal-digit octal-digit
 * hexadecimal-escape-sequence:
 *      \x hexadecimal-digit hexadecimal-digit hexadecimal-digit hexadecimal-digit
 * string-literal:
 *      encoding-prefix[opt]" s-char-sequence[opt]"
 *      encoding-prefix[opt]R raw-string
 * encoding-prefix
 *      u8
 *      u
 *      U
 *      L
 * s-char-sequence:
 *      s-char
 *      s-char-sequence s-char
 * s-char
 *      any member of the source character set except
 *          the double-quote ", backslash \, or new-line character
 *      escape-sequence
 *      universal-character-name
 * raw-string
 *      " d-char-sequence[opt]( r-char-sequence[opt] d-char-sequence[opt]"
 * r-char-sequence:
 *      r-char
 *      r-char-sequence r-char
 * r-char:
 *      any member of the source character set, except
 *          a right parenthesis ) followed by the initial d-char-sequence
 *          (which may be empty) followed by a double quote "
 * d-char-sequence
 *      d-char
 *      d-char-sequence d-char
 * d-char:
 *      any member of the basic source character set except:
 *          space, the left parenthesis (, the right parenthesis ), the backslash \,
 *          and the control characters representing horizontal tab,
 *          vertical tab, form feed, and newline.
 * user-defined-string-literal:
 *      string-literal ud-suffix
 * user-defined-character-literal:
 *      character-literal ud-suffix
 * ud-suffix:
 *      identifier
 *
 *
 * preprocessing-file:
 *      group[opt]
 * group:
 *      group-part
 *      group group-part
 * group-part:
 *      if-section
 *      control-line
 *      text-line
 *      # non-directive
 * if-section:
 *      if-group elif-groups[opt] else-group [opt] endif-line
 * if-group:
 *      # if        constant-expression new-line group[opt]
 *      # ifdef     identifier new-line group[opt]
 *      # ifndef    identifier new-line group[opt]
 * elif-groups:
 *      elif-group
 *      elif-groups elif-group
 * elif-group:
 *      # elif      constant-expression new-line group[opt]
 * else-group:
 *      # else      new-line group[opt]
 * endif-line:
 *      # endif     new-line
 * control-line:
 *      # include   pp-tokens new-line
 *      # define    identifier replacement-list new-line
 *      # define    identifier lparen identifier-list[opt]) replacement-list new-line
 *      # define    identifier lparen ... ) replacement-list new-line
 *      # define    identifier lparen identifier-list, ... ) replacement-list new-line
 *      # undef     identifier new-line
 * text-line:
 *      pp-tokens[opt] new-line
 * non-directive:
 *      pp-tokens new-line
 * lparen:
 *      a ( character not immediately preceded by white-space
 * identifier-list:
 *      identifier
 *      identifier-list, identifier
 * replacement-list:
 *      pp-tokens[opt]
 * pp-tokens:
 *      preprocessing-token
 *      pp-tokens preprocessing-token
 * new-line:
 *      the new-line character
 * */


    typedef std::istream::streampos pos_t;

    const int PP_PUNCS_COUNT = 57;
    char PP_PUNCS[PP_PUNCS_COUNT][4] = {
            "->*", "%:%", "...", ">>=", "<<=", "##", "<:", ":>",
            "<%", "%>", "%:", "::", ".*", "+=", "-=", "*=", "/=",
            "%=", "^=", "&=", "|=", "<<", ">>", "==", "!=", "<=",
            ">=", "&&", "||", "++", "--", "->", "{", "}", "[",
            "]", "#", "(", ")", ";", ":", "?", ".", "+", "-",
            "*", "/", "%", "^", "&", "|", "~", "!", "=", "<", ">", ","
    };

    class PosInfo {
    public:
        PosInfo(const std::string &f): file(f), line(1), col(0), pos(0) {}
        std::string file;
        int line, col;
        unsigned long pos;
    };

    class Token {
    public:
        enum token_type {
            WHITESPACE,
            IDENTIFIER,
            NUMBER,
            CHARACTER,
            STRING,
            PUNC
        };

        Token(token_type t, const std::string &v, const PosInfo &posInfo, bool nl = false):
                _type(t), _value(v), _file(posInfo.file), _line(posInfo.line), _col(posInfo.col), _pos(posInfo.pos), _hasNewLine(nl) { }

        token_type type() {
            return _type;
        }

        const std::string &value() {
            return _value;
        }

        bool hasNewLine() {
            return _hasNewLine;
        }

        const std::string &file() {
            return _file;
        }

        int line() {
            return _line;
        }

        int col() {
            return _col;
        }

        const pos_t &pos() {
            return _pos;
        }

    private:
        token_type _type;
        std::string _file, _value;
        pos_t _pos;
        int _line, _col;
        bool _hasNewLine;
    };

    typedef std::shared_ptr<Token> token_t;

    class Macro {
    public:
        Macro(const std::string &n):
                _name(n), _body() { }

        const std::string &name() {
            return _name;
        }

        void addToken(const std::shared_ptr<Token> token) {
            _body.push_back(token);
        }
    private:
        bool isFunctionLike;
        std::string _name;
        std::deque<std::shared_ptr<Token>> _body;
    };

    class FunctionMacro: public Macro {
        FunctionMacro(const std::string &n):
                Macro(n), _params() { }

        void addParam(const std::string param) {
            _params.push_back(param);
        }
    private:
        std::deque<std::string> _params;
    };

    class TokenStream {
    public:
        TokenStream():
                buffer() {}

        bool finished() {
            return buffer.empty() && _finished();
        }

        virtual bool _finished() {
            return true;
        }

        token_t next() {
            if (buffer.empty()) {
                return _next();
            } else {
                auto token = buffer[0];
                buffer.pop_front();
                return token;
            }
        }

        virtual token_t _next() {
            return token_t();
        }

        void unget(token_t token) {
            buffer.push_front(token);
        }

        token_t space(bool allowNewLine) {
            auto token = next();
            if (!token)
                return token;
            else if (token->type() != Token::WHITESPACE ||
                        (!allowNewLine && token->hasNewLine())) {
                unget(token);
                return token_t();
            } else {
                return token;
            }
        }

        token_t expectNewLine() {
            auto token = next();
            if (token && (token->type() != Token::WHITESPACE || !token->hasNewLine()))
                throw "Expected new line";
            return token;
        }

        token_t match(Token::token_type type, const std::string &v) {
            auto token = next();
            if (token && token->type() == type && token->value() == v) {
                return token;
            } else {
                unget(token);
                return token_t();
            }
        }

        token_t expect(Token::token_type type, const std::string &v) {
            auto token = next();
            if (token && token->type() == type && token->value() == v) {
                return token;
            } else {
                throw "Expected " + v;
            }
        }

        token_t matchPunc(const std::string &v) {
            return match(Token::PUNC, v);
        }

        token_t expectPunc(const std::string &v) {
            return expect(Token::PUNC, v);
        }

        token_t matchId(const std::string &v) {
            return match(Token::IDENTIFIER, v);
        }

        token_t expectId(const std::string &v) {
            return expect(Token::IDENTIFIER, v);
        }

        void print(std::ostream os) {
            auto prev = token_t();
            while (auto token = next()) {
                if (prev) {
                    if (prev->type() == Token::NUMBER ||
                            prev->type() == Token::IDENTIFIER ||
                            prev-> type() == Token::PUNC) {
                        os << ' ';
                    }
                }
                os << token->value();
            }
        }
    private:
        std::deque<token_t> buffer;
    };

    class Tokenizer: public TokenStream {
    public:
        Tokenizer(std::istream &i, const std::string &f): input(i), buffer(), pos(f), startPos(f), hasReturn(false) {}

        virtual bool _finished() {
            return input.eof();
        }

        void spliceLine();

        void advance();

        int match(int c, bool output = true) {
            if (input.peek() == c) {
                advance();
                if (output)
                    buffer << (char) c;
                return c;
            } else {
                return 0;
            }
        }

        bool match(const std::string &s, bool output = true);

        token_t parseSpace();
        token_t parseId();
        token_t parseNumber();
        token_t parsePunc();
        void parseEscape();
        token_t parseCharSequence(char quote, Token::token_type type);
        token_t parseRawString();
    private:
        void startToken() {
            startPos = pos;
            buffer.str("");
        }

        void newLine() {
            pos.line++;
            pos.col = 0;
        }

        bool isIdChar(int c) {
            return isdigit(c) || isalpha(c) || c == '_';
        }

        void hex() {
            int c = input.peek();
            if (c >= '0' && c <= '9' ||
                    c >= 'A' && c <= 'F' ||
                    c >= 'a' && c <= 'f') {
                advance();
                buffer << (char) c;
            } else {
                throw "Expected hexadecimal digit";
            }
        }

        void oct() {
            int c = input.peek();
            if (c >= '0' && c <= '7') {
                advance();
                buffer << (char) c;
            }
        }

        std::istream &input;
        std::stringstream buffer;
        PosInfo pos;
        PosInfo startPos;
        bool hasReturn;
    };
}

#endif