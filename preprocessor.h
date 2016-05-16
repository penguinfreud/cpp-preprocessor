#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <istream>
#include <vector>
#include <deque>
#include <memory>
#include <map>

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
                _type(t), _value(v), _pos(posInfo), _hasNewLine(nl) { }

        token_type type() const {
            return _type;
        }

        const std::string &value() const {
            return _value;
        }

        bool hasNewLine() const {
            return _hasNewLine;
        }

        const PosInfo &pos() const {
            return _pos;
        }

    private:
        token_type _type;
        const PosInfo _pos;
        std::string _value;
        bool _hasNewLine;
    };

    typedef std::shared_ptr<Token> token_t;

    class Macro {
    public:
        Macro(const std::string &n, bool f = false):
                _name(n), _body(), _isFunctionLike(f) { }

        const std::string &name() const {
            return _name;
        }

        bool isFunctionLike() const {
            return _isFunctionLike;
        }

        bool empty() const {
            return _body.empty();
        }

        const std::deque<token_t> &body() const {
            return _body;
        }

        void addToken(const std::shared_ptr<Token> token) {
            _body.push_back(token);
        }
    private:
        bool _isFunctionLike;
        std::string _name;
        std::deque<std::shared_ptr<Token>> _body;
    };

    class FunctionMacro: public Macro {
    public:
        FunctionMacro(const std::string &n):
                Macro(n, true), _params() { }

        const std::deque<std::string> &params() const {
            return _params;
        }

        void addParam(const std::string param) {
            _params.push_back(param);
        }
    private:
        std::deque<std::string> _params;
    };

    typedef std::shared_ptr<std::map<std::string, std::shared_ptr<Macro>>> macro_table_t;

    class MacroExpander;

    class TokenStream {
    public:
        TokenStream():
                buffer() {}
        TokenStream(const std::deque<token_t> buf):
                buffer(buf) {}

        bool finished() const {
            return buffer.empty() && _finished();
        }

        virtual bool _finished() const {
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
            return token_t(nullptr);
        }

        void unget(token_t token) {
            buffer.push_front(token);
        }

        token_t space(bool allowNewLine = true) {
            auto token = next();
            if (!token)
                return token;
            else if (token->type() != Token::WHITESPACE ||
                        (!allowNewLine && token->hasNewLine())) {
                unget(token);
                return token_t(nullptr);
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
                return token_t(nullptr);
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

        token_t matchPunc(char c) {
            auto token = next();
            if (token && token->type() == Token::PUNC) {
                const std::string &v = token->value();
                if (v.size() == 1 && v[0] == c) {
                    return token;
                }
            }
            unget(token);
            return token_t(nullptr);
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
            auto prev = token_t(nullptr);
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

        std::shared_ptr<MacroExpander> expandMacro(macro_table_t table);
    private:
        std::deque<token_t> buffer;
    };

    class Tokenizer: public TokenStream {
    public:
        Tokenizer(std::istream &i, const std::string &f): input(i), tokenBuffer(), pos(f), startPos(f), hasReturn(false) {}

        virtual bool _finished() const {
            return input.eof();
        }

        void spliceLine();

        void advance(bool raw = false);

        int match(int c, bool output = true, bool raw = false) {
            if (input.peek() == c) {
                advance(raw);
                if (output)
                    tokenBuffer << (char) c;
                return c;
            } else {
                return 0;
            }
        }

        bool match(const std::string &s, bool output = true, bool raw = false);

        token_t parseSpace();
        token_t parseId();
        token_t parseNumber();
        token_t parsePunc();
        void parseEscape();
        token_t parseCharSequence(char quote, Token::token_type type);
        token_t parseRawString();

        virtual token_t _next();
    private:
        void startToken() {
            startPos = pos;
            tokenBuffer.str("");
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
                tokenBuffer << (char) c;
            } else {
                throw "Expected hexadecimal digit";
            }
        }

        void oct() {
            int c = input.peek();
            if (c >= '0' && c <= '7') {
                advance();
                tokenBuffer << (char) c;
            }
        }

        std::string escape(int c) {
            return c == '\t'? "\\t":
                   c == '\f'? "\\f":
                   c == '\n'? "\\n":
                   c == '\r'? "\\r": "";
        }

        std::istream &input;
        std::stringstream tokenBuffer;
        PosInfo pos;
        PosInfo startPos;
        bool hasReturn;
    };

    class MacroStack {
    public:
        MacroStack(const std::string &x):
                car(x), cdr(nullptr) {}
        MacroStack(const std::string &x, std::shared_ptr<MacroStack> xs):
                car(x), cdr(xs) {}

        bool hasName(const std::string &key) {
            if (key == car)
                return true;
            else if (cdr)
                return cdr->hasName(key);
            else
                return false;
        }

        std::string car;
        std::shared_ptr<MacroStack> cdr;
    };

    class MacroExpander: public TokenStream {
    public:
        MacroExpander(std::shared_ptr<TokenStream> i, macro_table_t t, std::shared_ptr<MacroStack> s):
                input(i), macroTable(t), stack(s) {};

        virtual bool _finished() const {
            return input->finished() &&
                    (!expander || expander->finished());
        }

        virtual token_t _next();
    private:
        token_t expandMacro(token_t name);
        token_t expandObjectMacro(const Macro& macro);
        token_t expandFunctionMacro(token_t name, const FunctionMacro &macro);

        std::shared_ptr<std::deque<token_t>> scanArg(std::shared_ptr<std::deque<token_t>> curArg);
        std::deque<token_t> subBody(const FunctionMacro &macro, const std::vector<std::shared_ptr<std::deque<token_t>>> &args);

        std::shared_ptr<TokenStream> input;
        std::shared_ptr<TokenStream> expander;
        macro_table_t macroTable;
        std::shared_ptr<MacroStack> stack;
    };

    std::shared_ptr<MacroExpander> TokenStream::expandMacro(macro_table_t table) {
        return std::make_shared<MacroExpander>(std::shared_ptr<TokenStream>(this), table, std::shared_ptr<MacroStack>());
    }

    class DirectiveParser: public TokenStream {
    public:
        DirectiveParser(std::shared_ptr<TokenStream> i, macro_table_t t):
                input(i), table(t), ifStack(), lineStart(true) {}

        virtual bool _finished() {
            return input->finished();
        }

        virtual token_t _next();

        token_t parseDefine();
        token_t parseUndef();
        token_t parseIf(bool defined, bool neg = false);

        token_t skipLine();
    private:
        std::shared_ptr<TokenStream> input;
        macro_table_t table;
        std::vector<bool> ifStack;
        bool lineStart;
    };
}

#endif