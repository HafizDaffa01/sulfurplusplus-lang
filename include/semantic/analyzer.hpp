#pragma once

#include <vector>
#include <string>
#include <memory>
#include "ast.hpp"
#include "diagnostic.hpp"
#include "semantic/type.hpp"
#include "semantic/symbol_table.hpp"
#include "semantic/type_checker.hpp"

class SemanticAnalyzer {
public:
    SemanticAnalyzer();

    // Analyzes the given statements. Throws SulfurError (TypeError / SemanticError) if invalid.
    void analyze(const std::vector<StmtPtr>& stmts, const std::string& filename = "");

    const std::vector<Diagnostic>& getDiagnostics() const { return diagnostics_; }
    bool hasErrors() const;

    const SymbolTable& getSymbolTable() const { return symTable_; }
    std::string dumpSymbols() const { return symTable_.dumpTree(); }

private:
    SymbolTable symTable_;
    TypeChecker typeChecker_;
    std::string currentFilename_;
    std::vector<Diagnostic> diagnostics_;

    void visitStmt(const Stmt& stmt);
    void visitBlock(const BlockStmt& stmt, ScopeKind kind = ScopeKind::Block, TypePtr fnRetType = nullptr);
    void visitVarDecl(const VarDeclStmt& stmt);
    void visitFnDecl(const FnDeclStmt& stmt);
    void visitClassDecl(const ClassDeclStmt& stmt);
    void visitStructDecl(const StructDeclStmt& stmt);
    void visitIf(const IfStmt& stmt);
    void visitWhile(const WhileStmt& stmt);
    void visitFor(const ForStmt& stmt);
    void visitForCStyle(const ForCStyleStmt& stmt);
    void visitReturn(const ReturnStmt& stmt);
    void visitBreak(const BreakStmt& stmt);
    void visitContinue(const ContinueStmt& stmt);
    void visitDefer(const DeferStmt& stmt);
    void visitTryCatch(const TryCatchStmt& stmt);
    void visitMatch(const MatchStmt& stmt);
    void visitUnsafe(const UnsafeStmt& stmt);
    void visitImport(const ImportStmt& stmt);
    void visitExport(const ExportStmt& stmt);
    void visitExpose(const ExposeStmt& stmt);

    TypePtr visitExpr(const Expr& expr);

    void reportError(const std::string& message, int line, int col = 1, const std::string& code = "E_SEMANTIC_400", const std::string& hint = "");
    void reportTypeError(const std::string& message, int line, int col = 1, const std::string& hint = "");
};
