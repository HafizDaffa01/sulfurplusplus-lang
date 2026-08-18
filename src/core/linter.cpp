#include "linter.hpp"
#include <cctype>

std::vector<LintIssue> Linter::lint(const std::vector<StmtPtr>& stmts, const std::string& filename) {
    filename_ = filename;
    issues_.clear();
    declaredVars_.clear();
    usedVars_.clear();
    scopeDepth_ = 0;

    checkDeadCode(stmts);
    for (const auto& s : stmts) {
        if (s) checkStmt(*s);
    }
    checkUnusedVars();

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

void Linter::checkUnusedVars() {
    for (const auto& decl : declaredVars_) {
        if (usedVars_.count(decl.name)) continue;
        issues_.push_back({
            DiagnosticSeverity::WARNING,
            "W_UNUSED_VAR",
            "Variable '" + decl.name + "' is declared but never used",
            filename_,
            decl.line,
            1,
            "Remove the declaration, or prefix the name with '_' to keep it intentionally."
        });
    }
}

void Linter::checkFnDecl(const FnDeclStmt& node) {
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

void Linter::checkStmt(const Stmt& stmt) {
    std::visit([&](const auto& node) {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, VarDeclStmt>) {
            if (node.initializer) checkExpr(*node.initializer);
            // Top level declarations form the exported namespace of a module, so only
            // block scoped ones can be judged unused. A leading '_' opts a name out.
            if (scopeDepth_ > 0 && !node.name.empty() && node.name[0] != '_') {
                declaredVars_.push_back({node.name, node.line});
            }
        }
        else if constexpr (std::is_same_v<T, FnDeclStmt>) {
            checkFnDecl(node);
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
                if (!m) continue;
                // Fields are read through member access, which cannot be resolved back
                // to a declaration, so they are inspected but never tracked by name.
                if (const auto* field = std::get_if<VarDeclStmt>(&m->data)) {
                    if (field->initializer) checkExpr(*field->initializer);
                    continue;
                }
                checkStmt(*m);
            }
        }
        else if constexpr (std::is_same_v<T, InterfaceDeclStmt>) {
            for (const auto& m : node.methods) {
                checkFnDecl(m);
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
            scopeDepth_++;
            for (const auto& s : node.stmts) {
                if (s) checkStmt(*s);
            }
            scopeDepth_--;
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
        else if constexpr (std::is_same_v<T, ForCStyleStmt>) {
            if (node.init) checkStmt(*node.init);
            if (node.cond) checkExpr(*node.cond);
            if (node.post) checkExpr(*node.post);
            if (node.body) checkStmt(*node.body);
        }
        else if constexpr (std::is_same_v<T, ReturnStmt>) {
            if (node.value) checkExpr(*node.value);
        }
        else if constexpr (std::is_same_v<T, ThrowStmt>) {
            if (node.value) checkExpr(*node.value);
        }
        else if constexpr (std::is_same_v<T, ExprStmt>) {
            if (node.expr) checkExpr(*node.expr);
        }
        else if constexpr (std::is_same_v<T, StreamOutStmt>) {
            if (node.target) checkExpr(*node.target);
            if (node.value) checkExpr(*node.value);
        }
        else if constexpr (std::is_same_v<T, UnsafeStmt>) {
            if (node.body) checkStmt(*node.body);
        }
        else if constexpr (std::is_same_v<T, DeferStmt>) {
            if (node.body) checkStmt(*node.body);
        }
        else if constexpr (std::is_same_v<T, TryCatchStmt>) {
            if (node.tryBody) checkStmt(*node.tryBody);
            if (node.catchBody) checkStmt(*node.catchBody);
            if (node.finallyBody) checkStmt(*node.finallyBody);
        }
        else if constexpr (std::is_same_v<T, MatchStmt>) {
            if (node.value) checkExpr(*node.value);
            for (const auto& c : node.cases) {
                if (c.pattern) checkExpr(*c.pattern);
                if (c.body) checkStmt(*c.body);
            }
        }
        else if constexpr (std::is_same_v<T, OverwriteStmt>) {
            // 'overwrite' rebinds an existing symbol, so the target counts as a use.
            size_t dot = node.target.find('.');
            usedVars_.insert(dot == std::string::npos ? node.target : node.target.substr(0, dot));
            if (node.value) checkExpr(*node.value);
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
        else if constexpr (std::is_same_v<T, DictLitExpr>) {
            for (const auto& p : node.pairs) {
                if (p.first) checkExpr(*p.first);
                if (p.second) checkExpr(*p.second);
            }
        }
        else if constexpr (std::is_same_v<T, PSStringExpr>) {
            for (const auto& seg : node.segments) {
                if (seg.isExpr && seg.expr) checkExpr(*seg.expr);
            }
        }
        else if constexpr (std::is_same_v<T, IndexExpr>) {
            if (node.object) checkExpr(*node.object);
            if (node.index) checkExpr(*node.index);
        }
        else if constexpr (std::is_same_v<T, MemberExpr>) {
            if (node.object) checkExpr(*node.object);
        }
        else if constexpr (std::is_same_v<T, PipelineExpr>) {
            if (node.left) checkExpr(*node.left);
            if (node.right) checkExpr(*node.right);
        }
        else if constexpr (std::is_same_v<T, NullCoalExpr>) {
            if (node.left) checkExpr(*node.left);
            if (node.right) checkExpr(*node.right);
        }
        else if constexpr (std::is_same_v<T, LambdaExpr>) {
            if (node.body) checkStmt(*node.body);
        }
        else if constexpr (std::is_same_v<T, NewExpr>) {
            for (const auto& a : node.args) {
                if (a) checkExpr(*a);
            }
        }
        else if constexpr (std::is_same_v<T, TernaryExpr>) {
            if (node.cond) checkExpr(*node.cond);
            if (node.thenExpr) checkExpr(*node.thenExpr);
            if (node.elseExpr) checkExpr(*node.elseExpr);
        }
        else if constexpr (std::is_same_v<T, AddrOfExpr> ||
                           std::is_same_v<T, DerefExpr> ||
                           std::is_same_v<T, DeleteExpr>) {
            if (node.operand) checkExpr(*node.operand);
        }
    }, expr.data);
}
