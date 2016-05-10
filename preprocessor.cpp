#include <iostream>
#include <fstream>
#include "preprocessor.h"

FileContext::FileContext(std::istream &i): isFile(false), path(), istream(&i) {}

FileContext::FileContext(std::string &p): isFile(true), path(p) {
    std::ifstream *ifs = new std::ifstream();
    ifs->open(p);
    istream = ifs;
}

Preprocessor::Preprocessor(std::istream &i): fileContext(i) {}

Preprocessor::Preprocessor(std::string &p): fileContext(p) {}

void Preprocessor::preprocess() {

}

int main(int argc, char **argv) {
    if (argc == 1) {
        Preprocessor preprocessor(std::cin);
        preprocessor.preprocess();
    } else {
        for (int i = 1; i < argc; i++) {
            std::string path(argv[i]);
            Preprocessor preprocessor(path);
            preprocessor.preprocess();
        }
    }
    std::cout << argc;
}