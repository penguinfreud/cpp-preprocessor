#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "preprocessor.h"

cpp::PosInfo cpp::posStart("");

inline void assert(bool cond) {
    if (!cond)
        throw cpp::ParsingException("Assertion failed", cpp::posStart);
}

void testTokenizer() {
    using namespace cpp;
    auto ss = std::make_shared<std::stringstream>();
    ss->str("a/*foo\n*/b //\n");
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
        ss->clear();
        ss->str("''u'cd'U'\\000''\\n'U\"aaaa\\\"\"u8R\"/*(\nfoo)/*\"");
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
    auto ss = std::make_shared<std::stringstream>();
    ss->str("foo");
    macro_table_t table = std::make_shared<std::map<std::string, std::shared_ptr<Macro>>>();
    auto macro = std::make_shared<Macro>("foo");
    PosInfo pi("anon");
    std::deque<token_t> body;
    body.push_back(std::make_shared<Token>(Token::NUMBER, "2", pi));
    macro->setBody(body);
    (*table)["foo"] = macro;
    auto tokenizer = std::make_shared<Tokenizer>(ss, "anon");
    auto expander = std::make_shared<MacroExpander>(tokenizer, table, std::shared_ptr<MacroStack>());
    auto token = expander->next();
    std::cout << token->type() << std::endl;
    std::cout << token->value() << std::endl;
}

void processFile(std::shared_ptr<std::istream> input, const std::string &file) {
    using namespace cpp;
    auto macroTable = std::make_shared<std::map<std::string, std::shared_ptr<Macro>>>();
    auto stack = std::shared_ptr<MacroStack>();
    auto tokenizer = std::make_shared<Tokenizer>(input, file);
    auto dirParser = std::make_shared<DirectiveParser>(tokenizer, macroTable, stack, file, 0);
    auto expander = std::make_shared<MacroExpander>(dirParser, macroTable, stack);
    try {
        expander->print(std::cout);
        std::cout << std::endl;
    } catch (ParsingException &e) {
        std::cerr << e.what() << std::endl;
    }
}

int main(int argc, char **argv) {
    if (argc == 1) {
        std::shared_ptr<std::istream> cin(static_cast<std::istream*>(&std::cin));
        processFile(cin, "");
    } else {
        for (int i = 1; i<argc; i++) {
            auto input = std::make_shared<std::ifstream>();
            input->open(argv[i]);
            if (input->is_open()) {
                processFile(input, argv[i]);
            } else {
                std::cerr << "Open file failed: ";
                std::cerr << argv[i] << std::endl;
            }
        }
    }
}
