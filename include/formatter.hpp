#pragma once

#include <string>
#include <vector>
#include <memory>
#include "ast.hpp"

class Formatter {
public:
    explicit Formatter(int indentSize = 4) : indentSize_(indentSize) {}

    std::string format(const std::vector<StmtPtr>& stmts);
    std::string formatSource(const std::string& source, const std::string& filename = "<formatter>");

private:
    int indentSize_;
    int currentIndent_ = 0;

    std::string getIndent() const;
    void indent() { currentIndent_ += indentSize_; }
    void dedent() { currentIndent_ = std::max(0, currentIndent_ - indentSize_); }

    std::string formatStmt(const Stmt& stmt);
    std::string formatExpr(const Expr& expr);
    std::string formatBlock(const BlockStmt& stmt);
};
