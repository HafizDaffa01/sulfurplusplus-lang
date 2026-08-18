#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "ast.hpp"
#include "error.hpp"

enum class FunctionContext {
    NONE,
    FUNCTION,
    METHOD,
    LAMBDA
};

enum class ClassContext {
    NONE,
    CLASS
};

enum class LoopContext {
    NONE,
    LOOP
};

struct FunctionSignature {
    std::vector<std::pair<std::string, std::string>> params;
    std::string returnType;
    int line;
};

struct SymbolInfo {
    bool isMutable;
    bool isDefined;
    int line;
    std::string typeName;
};

class Resolver {
public:
    Resolver();

    void resolve(const std::vector<StmtPtr>& stmts);

    std::string inferExprType(const Expr& e);
    bool isTypeCompatible(const std::string& expected, const std::string& actual);

private:
    std::vector<std::unordered_map<std::string, SymbolInfo>> scopes_;
    std::unordered_set<std::string> globals_;
    std::unordered_map<std::string, FunctionSignature> functionSignatures_;
    FunctionContext currentFunction_ = FunctionContext::NONE;
    ClassContext currentClass_ = ClassContext::NONE;
    std::string currentReturnType_ = "";
    int loopDepth_ = 0;

    void beginScope();
    void endScope();
    void declare(const std::string& name, bool isMutable, int line, const std::string& typeName = "any");
    void define(const std::string& name);
    SymbolInfo* lookupSymbol(const std::string& name);

    void resolveStmt(const Stmt& s);
    void resolveBlock(const BlockStmt& b);
    void resolveVarDecl(const VarDeclStmt& s);
    void resolveFnDecl(const FnDeclStmt& s);
    void resolveClassDecl(const ClassDeclStmt& s);
    void resolveStructDecl(const StructDeclStmt& s);
    void resolveInterfaceDecl(const InterfaceDeclStmt& s);
    void resolveIf(const IfStmt& s);
    void resolveWhile(const WhileStmt& s);
    void resolveFor(const ForStmt& s);
    void resolveForCStyle(const ForCStyleStmt& s);
    void resolveReturn(const ReturnStmt& s);
    void resolveThrow(const ThrowStmt& s);
    void resolveStreamOut(const StreamOutStmt& s);
    void resolveTryCatch(const TryCatchStmt& s);
    void resolveMatch(const MatchStmt& s);
    void resolveUnsafe(const UnsafeStmt& s);
    void resolveDefer(const DeferStmt& s);

    void resolveExpr(const Expr& e);
    void resolveAssign(const AssignExpr& e);
    void resolveBinary(const BinaryExpr& e);
    void resolveUnary(const UnaryExpr& e);
    void resolveCall(const CallExpr& e);
    void resolveIndex(const IndexExpr& e);
    void resolveMember(const MemberExpr& e);
    void resolvePipeline(const PipelineExpr& e);
    void resolveNullCoal(const NullCoalExpr& e);
    void resolveListLit(const ListLitExpr& e);
    void resolveDictLit(const DictLitExpr& e);
    void resolveLambda(const LambdaExpr& e);
    void resolveNew(const NewExpr& e);
    void resolveTernary(const TernaryExpr& e);
    void resolveIdent(const IdentExpr& e);
};
