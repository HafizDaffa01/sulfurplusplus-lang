#pragma once
#include <vector>
#include "token.hpp"
#include "ast.hpp"

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::vector<StmtPtr> parse();

private:
    std::vector<Token> tokens_;
    size_t pos_ = 0;

    Token& peek(int offset = 0);
    const Token& peek(int offset = 0) const;
    Token& advance();
    bool check(TokenType t) const;
    bool checkIdent() const;
    bool isTypeToken(TokenType t) const;
    bool match(TokenType t);
    bool match(std::initializer_list<TokenType> types);
    Token expect(TokenType t, const std::string& msg);
    bool isAtEnd() const;

    // Statements
    StmtPtr parseStmt();
    StmtPtr parseVarDecl(const std::string& keyword);
    StmtPtr parseFnDecl(bool isMethod = false);
    StmtPtr parseClassDecl();
    StmtPtr parseStructDecl();
    StmtPtr parseInterfaceDecl();
    StmtPtr parseBlock();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseFor();
    StmtPtr parseReturn();
    StmtPtr parseThrow();

    StmtPtr parseImport();
    StmtPtr parseExport();
    StmtPtr parseExpose();
    StmtPtr parseOverwrite();
    StmtPtr parseUnsafe();
    StmtPtr parseDefer();
    StmtPtr parseTryCatch();
    StmtPtr parseMatch();
    StmtPtr parseExprStmt();

    // Expressions
    ExprPtr parseExpr();
    ExprPtr parseAssign();
    ExprPtr parseTernary();
    ExprPtr parseNullCoal();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseBitwiseOr();
    ExprPtr parseBitwiseXor();
    ExprPtr parseBitwiseAnd();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseAddSub();
    ExprPtr parseShift();
    ExprPtr parseMulDiv();
    ExprPtr parseUnary();
    ExprPtr parsePipeline();
    ExprPtr parsePostfix();
    ExprPtr parsePrimary();
    ExprPtr parsePSString(const std::string& raw, int line);

    std::vector<std::pair<std::string,std::string>> parseParamList();
    std::string parseType();
};
