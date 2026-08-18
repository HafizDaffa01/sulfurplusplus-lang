#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include "ast.hpp"
#include "diagnostic.hpp"

struct LintIssue {
    DiagnosticSeverity severity;
    std::string rule;
    std::string message;
    std::string filename;
    int line;
    int col;
    std::string hint;
};

class Linter {
public:
    Linter() = default;

    std::vector<LintIssue> lint(const std::vector<StmtPtr>& stmts, const std::string& filename = "<linter>");

private:
    std::string filename_;
    std::vector<LintIssue> issues_;
    std::unordered_set<std::string> declaredVars_;
    std::unordered_set<std::string> usedVars_;

    void checkStmt(const Stmt& stmt);
    void checkExpr(const Expr& expr);
    void checkDeadCode(const std::vector<StmtPtr>& stmts);
};
