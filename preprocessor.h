#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <istream>

/*
 * C++ Preprocessor Grammar
 * preprocessing-token:
 *      header-name
 *      identifier
 *      pp-number
 *      character-literal
 *      
 *
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