#pragma once
#include <string>
#include <vector>
#include "token.hpp"

class Lexer {
public:
    explicit Lexer(const std::string& source, const std::string& filename = "<input>");
    std::vector<Token> tokenize();

private:
    std::string source_;
    std::string filename_;
    size_t pos_ = 0;
    int line_ = 1;
    int col_  = 1;

    char peek(int offset = 0) const;
    char advance();
    bool match(char c);
    bool matchStr(const std::string& s);
    void skipWhitespaceAndComments();

    Token makeToken(TokenType type, const std::string& val);

    Token lexNumber();
    Token lexString(char delim);
    Token lexChar();
    Token lexPSString();
    Token lexIdent();
    Token lexLifecycle(); // +N> or ~N>
};
