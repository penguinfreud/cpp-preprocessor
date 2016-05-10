#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <istream>

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

class FileContext {
private:
    bool isFile;
    std::string path;
    std::istream *istream;
public:
    FileContext(std::istream &i);
    FileContext(std::string &p);
};

class Preprocessor {
private:
    FileContext fileContext;
public:
    Preprocessor(std::istream &i);
    Preprocessor(std::string &p);
    void preprocess();
};

#endif