#include <iostream>
#include <fstream>
#include "preprocessor.h"

FileContext::FileContext(Preprocessor &pp, std::istream &i): preprocessor(pp), isFile(false), path(), istream(&i) {
    peek = i.get();
}

FileContext::FileContext(Preprocessor &pp, std::string &p): preprocessor(pp), isFile(true), path(p) {
    std::ifstream *ifs = new std::ifstream();
    ifs->open(p);
    istream = ifs;
    peek = ifs->get();
}

std::string FileContext::resolveFile(std::string &path) {
}

void FileContext::advance() {
    peek = istream->get();
}

bool FileContext::match(char c, bool trim) {
    if (peek == c) {
        advance();
        if (trim) skipSpace();
        return true;
    } else {
        return false;
    }
}

bool FileContext::match(const std::string &str, bool trim) {
    std::istream::streampos pos = istream->tellg();
    unsigned long l = str.length();
    for (int i = 0; i<l; i++) {
        if (peek != str[i]) {
            istream->seekg(pos);
            return false;
        }
    }
    if (isalpha(str[l-1]) &&
            peek != std::istream::traits_type::eof() &&
            isalpha(peek)) {
        istream->seekg(pos);
        return false;
    }
    if (trim) skipSpace();
    advance();
    return true;
}

void FileContext::skipSpace() {
    while (peek == ' ' || peek == '\t' || peek == '\v' || peek == '\f') {
        advance();
    }
}

void FileContext::parseGroup() {
    skipSpace();
    if (match('#')) {
        if (match("include")) {

        }
    }
    void(*f)() = []() {};
}

Preprocessor::Preprocessor(std::istream &i): fileContext(*this, i), includePaths(), line(0), col(0) {}

Preprocessor::Preprocessor(std::string &p): fileContext(*this, p), includePaths(), line(0), col(0) {}

void Preprocessor::preprocess() {
    fileContext.parseGroup();
}

int main(int argc, char **argv) {
    /*if (argc == 1) {
        Preprocessor preprocessor(std::cin);
        preprocessor.preprocess();
    } else {
        for (int i = 1; i < argc; i++) {
            std::string path(argv[i]);
            Preprocessor preprocessor(path);
            preprocessor.preprocess();
        }
    }*/
    std::ifstream ifs;
    ifs.open("/home/wsy/courses/OOP/lab3/lab3.h");

}