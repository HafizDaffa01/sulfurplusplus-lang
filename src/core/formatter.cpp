#include "formatter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <sstream>

std::string Formatter::getIndent() const {
    return std::string(currentIndent_, ' ');
}

std::string Formatter::formatSource(const std::string& source, const std::string& filename) {
    try {
        Lexer lexer(source, filename);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        auto stmts = parser.parse();
        return format(stmts);
    } catch (...) {
        return source; // If syntax error, return source unchanged
    }
}

std::string Formatter::format(const std::vector<StmtPtr>& stmts) {
    std::ostringstream ss;
    for (size_t i = 0; i < stmts.size(); i++) {
        if (!stmts[i]) continue;
        ss << formatStmt(*stmts[i]);
        if (i + 1 < stmts.size()) {
            ss << "\n";
        }
    }
    return ss.str();
}

std::string Formatter::formatStmt(const Stmt& stmt) {
    return std::visit([&](const auto& node) -> std::string {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, VarDeclStmt>) {
            std::ostringstream ss;
            ss << getIndent() << node.keyword << " " << node.name;
            if (!node.type.empty()) {
                ss << ": " << node.type;
            }
            if (node.initializer) {
                ss << " = " << formatExpr(*node.initializer);
            }
            ss << ";";
            return ss.str();
        }
        else if constexpr (std::is_same_v<T, FnDeclStmt>) {
            std::ostringstream ss;
            ss << getIndent() << "fn " << node.name << "(";
            for (size_t i = 0; i < node.params.size(); i++) {
                ss << node.params[i].first;
                if (!node.params[i].second.empty()) {
                    ss << ": " << node.params[i].second;
                }
                if (i + 1 < node.params.size()) ss << ", ";
            }
            ss << ")";
            if (!node.retType.empty()) {
                ss << " -> " << node.retType;
            }
            ss << " {\n";
            indent();
            if (node.body) {
                if (auto* b = std::get_if<BlockStmt>(&node.body->data)) {
                    for (const auto& s : b->stmts) {
                        if (s) ss << formatStmt(*s) << "\n";
                    }
                } else {
                    ss << formatStmt(*node.body) << "\n";
                }
            }
            dedent();
            ss << getIndent() << "}";
            return ss.str();
        }
        else if constexpr (std::is_same_v<T, ReturnStmt>) {
            std::ostringstream ss;
            ss << getIndent() << "return";
            if (node.value) {
                ss << " " << formatExpr(*node.value);
            }
            ss << ";";
            return ss.str();
        }
        else if constexpr (std::is_same_v<T, IfStmt>) {
            std::ostringstream ss;
            ss << getIndent() << "if (" << (node.cond ? formatExpr(*node.cond) : "") << ") {\n";
            indent();
            if (node.thenBranch) {
                if (auto* b = std::get_if<BlockStmt>(&node.thenBranch->data)) {
                    for (const auto& s : b->stmts) {
                        if (s) ss << formatStmt(*s) << "\n";
                    }
                } else {
                    ss << formatStmt(*node.thenBranch) << "\n";
                }
            }
            dedent();
            ss << getIndent() << "}";
            if (node.elseBranch) {
                ss << " else {\n";
                indent();
                if (auto* b = std::get_if<BlockStmt>(&node.elseBranch->data)) {
                    for (const auto& s : b->stmts) {
                        if (s) ss << formatStmt(*s) << "\n";
                    }
                } else {
                    ss << formatStmt(*node.elseBranch) << "\n";
                }
                dedent();
                ss << getIndent() << "}";
            }
            return ss.str();
        }
        else if constexpr (std::is_same_v<T, WhileStmt>) {
            std::ostringstream ss;
            ss << getIndent() << "while (" << (node.cond ? formatExpr(*node.cond) : "") << ") {\n";
            indent();
            if (node.body) {
                if (auto* b = std::get_if<BlockStmt>(&node.body->data)) {
                    for (const auto& s : b->stmts) {
                        if (s) ss << formatStmt(*s) << "\n";
                    }
                } else {
                    ss << formatStmt(*node.body) << "\n";
                }
            }
            dedent();
            ss << getIndent() << "}";
            return ss.str();
        }
        else if constexpr (std::is_same_v<T, ForStmt>) {
            std::ostringstream ss;
            ss << getIndent() << "for (" << node.var << " in " << (node.iterable ? formatExpr(*node.iterable) : "") << ") {\n";
            indent();
            if (node.body) {
                if (auto* b = std::get_if<BlockStmt>(&node.body->data)) {
                    for (const auto& s : b->stmts) {
                        if (s) ss << formatStmt(*s) << "\n";
                    }
                } else {
                    ss << formatStmt(*node.body) << "\n";
                }
            }
            dedent();
            ss << getIndent() << "}";
            return ss.str();
        }
        else if constexpr (std::is_same_v<T, BreakStmt>) {
            return getIndent() + "break;";
        }
        else if constexpr (std::is_same_v<T, ContinueStmt>) {
            return getIndent() + "continue;";
        }
        else if constexpr (std::is_same_v<T, ExprStmt>) {
            return getIndent() + (node.expr ? formatExpr(*node.expr) : "") + ";";
        }
        else if constexpr (std::is_same_v<T, ImportStmt>) {
            std::ostringstream ss;
            ss << getIndent() << "import " << node.pkg;
            if (!node.alias.empty()) {
                ss << " as " << node.alias;
            }
            ss << ";";
            return ss.str();
        }
        else if constexpr (std::is_same_v<T, BlockStmt>) {
            return formatBlock(node);
        }
        else {
            return getIndent() + "// statement";
        }
    }, stmt.data);
}

std::string Formatter::formatBlock(const BlockStmt& stmt) {
    std::ostringstream ss;
    ss << getIndent() << "{\n";
    indent();
    for (const auto& s : stmt.stmts) {
        if (s) ss << formatStmt(*s) << "\n";
    }
    dedent();
    ss << getIndent() << "}";
    return ss.str();
}

std::string Formatter::formatExpr(const Expr& expr) {
    return std::visit([&](const auto& node) -> std::string {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, IntLitExpr>) {
            return std::to_string(node.value);
        }
        else if constexpr (std::is_same_v<T, FloatLitExpr>) {
            std::string s = std::to_string(node.value);
            s.erase(s.find_last_not_of('0') + 1, std::string::npos);
            if (s.back() == '.') s += "0";
            return s;
        }
        else if constexpr (std::is_same_v<T, BoolLitExpr>) {
            return node.value ? "true" : "false";
        }
        else if constexpr (std::is_same_v<T, NullLitExpr>) {
            return "null";
        }
        else if constexpr (std::is_same_v<T, StringLitExpr>) {
            return "\"" + node.value + "\"";
        }
        else if constexpr (std::is_same_v<T, CharLitExpr>) {
            return std::string("'") + node.value + "'";
        }
        else if constexpr (std::is_same_v<T, IdentExpr>) {
            return node.name;
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
            return formatExpr(*node.left) + " " + node.op + " " + formatExpr(*node.right);
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return node.op + formatExpr(*node.operand);
        }
        else if constexpr (std::is_same_v<T, AssignExpr>) {
            return formatExpr(*node.target) + " = " + formatExpr(*node.value);
        }
        else if constexpr (std::is_same_v<T, CallExpr>) {
            std::ostringstream ss;
            ss << formatExpr(*node.callee) << "(";
            for (size_t i = 0; i < node.args.size(); i++) {
                if (node.args[i]) ss << formatExpr(*node.args[i]);
                if (i + 1 < node.args.size()) ss << ", ";
            }
            ss << ")";
            return ss.str();
        }
        else if constexpr (std::is_same_v<T, MemberExpr>) {
            return formatExpr(*node.object) + "." + node.member;
        }
        else if constexpr (std::is_same_v<T, IndexExpr>) {
            return formatExpr(*node.object) + "[" + formatExpr(*node.index) + "]";
        }
        else if constexpr (std::is_same_v<T, ListLitExpr>) {
            std::ostringstream ss;
            ss << "[";
            for (size_t i = 0; i < node.elements.size(); i++) {
                if (node.elements[i]) ss << formatExpr(*node.elements[i]);
                if (i + 1 < node.elements.size()) ss << ", ";
            }
            ss << "]";
            return ss.str();
        }
        else {
            return "...";
        }
    }, expr.data);
}
