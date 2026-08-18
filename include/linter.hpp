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
    struct DeclaredVar {
        std::string name;
        int line;
    };

    std::string filename_;
    std::vector<LintIssue> issues_;
    std::vector<DeclaredVar> declaredVars_;
    std::unordered_set<std::string> usedVars_;
    int scopeDepth_ = 0;

    void checkStmt(const Stmt& stmt);
    void checkExpr(const Expr& expr);
    void checkFnDecl(const FnDeclStmt& node);
    void checkDeadCode(const std::vector<StmtPtr>& stmts);
    void checkUnusedVars();
};
