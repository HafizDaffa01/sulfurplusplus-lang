#include "linter.hpp"
#include <cctype>

std::vector<LintIssue> Linter::lint(const std::vector<StmtPtr>& stmts, const std::string& filename) {
    filename_ = filename;
    issues_.clear();
    declaredVars_.clear();
    usedVars_.clear();

    checkDeadCode(stmts);
    for (const auto& s : stmts) {
        if (s) checkStmt(*s);
    }

    return issues_;
}

void Linter::checkDeadCode(const std::vector<StmtPtr>& stmts) {
    bool hasTerminated = false;
    for (const auto& s : stmts) {
        if (!s) continue;
        if (hasTerminated) {
            int line = 1;
            std::visit([&](const auto& n) { line = n.line; }, s->data);
            issues_.push_back({
                DiagnosticSeverity::WARNING,
                "W_DEAD_CODE",
                "Unreachable code detected after terminating statement",
                filename_,
                line,
                1,
                "Remove unreachable statements to clean up code."
            });
            break;
        }

        if (std::holds_alternative<ReturnStmt>(s->data) ||
            std::holds_alternative<BreakStmt>(s->data) ||
            std::holds_alternative<ContinueStmt>(s->data) ||
            std::holds_alternative<ThrowStmt>(s->data)) {
            hasTerminated = true;
        }
    }
}

void Linter::checkStmt(const Stmt& stmt) {
    std::visit([&](const auto& node) {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, VarDeclStmt>) {
            if (node.initializer) checkExpr(*node.initializer);
            declaredVars_.insert(node.name);
        }
        else if constexpr (std::is_same_v<T, FnDeclStmt>) {
            if (!node.name.empty() && std::isupper(node.name[0])) {
                issues_.push_back({
                    DiagnosticSeverity::WARNING,
                    "W_NAMING_STYLE",
                    "Function name '" + node.name + "' should start with lowercase (camelCase/snake_case)",
                    filename_,
                    node.line,
                    1,
                    "Rename function to start with lowercase letter."
                });
            }
            if (node.body) checkStmt(*node.body);
        }
        else if constexpr (std::is_same_v<T, ClassDeclStmt>) {
            if (!node.name.empty() && std::islower(node.name[0])) {
                issues_.push_back({
                    DiagnosticSeverity::WARNING,
                    "W_NAMING_STYLE",
                    "Class name '" + node.name + "' should use PascalCase",
                    filename_,
                    node.line,
                    1,
                    "Rename class to start with uppercase letter (e.g. " + std::string(1, std::toupper(node.name[0])) + node.name.substr(1) + ")."
                });
            }
            for (const auto& m : node.members) {
                if (m) checkStmt(*m);
            }
        }
        else if constexpr (std::is_same_v<T, BlockStmt>) {
            if (node.stmts.empty()) {
                issues_.push_back({
                    DiagnosticSeverity::WARNING,
                    "W_EMPTY_BLOCK",
                    "Empty block statement detected",
                    filename_,
                    node.line,
                    1,
                    "Remove empty block or implement logic inside."
                });
            }
            checkDeadCode(node.stmts);
            for (const auto& s : node.stmts) {
                if (s) checkStmt(*s);
            }
        }
        else if constexpr (std::is_same_v<T, IfStmt>) {
            if (node.cond) checkExpr(*node.cond);
            if (node.thenBranch) checkStmt(*node.thenBranch);
            if (node.elseBranch) checkStmt(*node.elseBranch);
        }
        else if constexpr (std::is_same_v<T, WhileStmt>) {
            if (node.cond) checkExpr(*node.cond);
            if (node.body) checkStmt(*node.body);
        }
        else if constexpr (std::is_same_v<T, ForStmt>) {
            if (node.iterable) checkExpr(*node.iterable);
            if (node.body) checkStmt(*node.body);
        }
        else if constexpr (std::is_same_v<T, ReturnStmt>) {
            if (node.value) checkExpr(*node.value);
        }
        else if constexpr (std::is_same_v<T, ExprStmt>) {
            if (node.expr) checkExpr(*node.expr);
        }
    }, stmt.data);
}

void Linter::checkExpr(const Expr& expr) {
    std::visit([&](const auto& node) {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, IdentExpr>) {
            usedVars_.insert(node.name);
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
            if (node.left) checkExpr(*node.left);
            if (node.right) checkExpr(*node.right);
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>) {
            if (node.operand) checkExpr(*node.operand);
        }
        else if constexpr (std::is_same_v<T, AssignExpr>) {
            if (node.target) checkExpr(*node.target);
            if (node.value) checkExpr(*node.value);
        }
        else if constexpr (std::is_same_v<T, CallExpr>) {
            if (node.callee) checkExpr(*node.callee);
            for (const auto& a : node.args) {
                if (a) checkExpr(*a);
            }
        }
        else if constexpr (std::is_same_v<T, ListLitExpr>) {
            for (const auto& el : node.elements) {
                if (el) checkExpr(*el);
            }
        }
    }, expr.data);
}
