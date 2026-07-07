#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include "ast.hpp"
#include "value.hpp"
#include "environment.hpp"

class Interpreter {
public:
    explicit Interpreter(bool debugMode = false, bool forceJIT = false);

    void run(const std::vector<StmtPtr>& stmts, const std::string& filepath = "");
    void injectBuiltinsIntoGlobal();
    void setStdout(std::ostream* out) { stdout_ = out; }
    void setStderr(std::ostream* err) { stderr_ = err; }
    void setStdin(std::istream* in)   { stdin_  = in; }

private:
    std::shared_ptr<Environment> globalEnv_;
    std::shared_ptr<Environment> currentEnv_;

    // Private registry for native builtins, not exposed globally to users
    ValuePtr builtinsRegistry_;
    bool debugMode_;
    bool forceJIT_;
    std::ostream* stdout_;
    std::ostream* stderr_;
    std::istream* stdin_;

    // Deferred blocks stack
    std::vector<std::vector<Stmt*>> deferStack_;

    // Current file stack
    std::vector<std::string> fileStack_;
    std::string currentFile() const { return fileStack_.empty() ? "" : fileStack_.back(); }

    // Registry for modules that export themselves
    std::unordered_map<std::string, ValuePtr> exportedModules_;

    // Alias registry: maps alias name -> {paramCount, expansionTemplate}
    struct AliasEntry {
        std::vector<std::string> paramNames;
        std::string expansion; // template with parameter names
    };
    std::unordered_map<std::string, AliasEntry> aliasRegistry_;

    // Execute statement
    void execStmt(const Stmt& s);
    void execBlock(const BlockStmt& b, std::shared_ptr<Environment> env = nullptr);
    void execVarDecl(const VarDeclStmt& s);
    void execFnDecl(const FnDeclStmt& s);
    void execClassDecl(const ClassDeclStmt& s);
    void execStructDecl(const StructDeclStmt& s);
    void execInterfaceDecl(const InterfaceDeclStmt& s);
    void execIf(const IfStmt& s);
    void execWhile(const WhileStmt& s);
    void execFor(const ForStmt& s);
    void execReturn(const ReturnStmt& s);
    void execThrow(const ThrowStmt& s);
    void execStreamOut(const StreamOutStmt& s);

    void execImport(const ImportStmt& s);
    void execExport(const ExportStmt& s);
    void execExpose(const ExposeStmt& s);
    void execOverwrite(const OverwriteStmt& s);
    void execUnsafe(const UnsafeStmt& s);
    void execDefer(const DeferStmt& s);
    void execTryCatch(const TryCatchStmt& s);

    // Evaluate expression
    ValuePtr evalExpr(const Expr& e);
    ValuePtr evalIntLit(const IntLitExpr& e);
    ValuePtr evalFloatLit(const FloatLitExpr& e);
    ValuePtr evalBoolLit(const BoolLitExpr& e);
    ValuePtr evalNullLit(const NullLitExpr& e);
    ValuePtr evalStringLit(const StringLitExpr& e);
    ValuePtr evalCharLit(const CharLitExpr& e);
    ValuePtr evalPSString(const PSStringExpr& e);
    ValuePtr evalIdent(const IdentExpr& e);
    ValuePtr evalBinary(const BinaryExpr& e);
    ValuePtr evalUnary(const UnaryExpr& e);
    ValuePtr evalAssign(const AssignExpr& e);
    ValuePtr evalCall(const CallExpr& e);
    ValuePtr evalIndex(const IndexExpr& e);
    ValuePtr evalMember(const MemberExpr& e);
    ValuePtr evalPipeline(const PipelineExpr& e);
    ValuePtr evalNullCoal(const NullCoalExpr& e);
    ValuePtr evalListLit(const ListLitExpr& e);
    ValuePtr evalDictLit(const DictLitExpr& e);
    ValuePtr evalNew(const NewExpr& e);
    ValuePtr evalTernary(const TernaryExpr& e);
    ValuePtr evalDelete(const DeleteExpr& e);

    // Function call helpers
    ValuePtr callFunction(std::shared_ptr<FunctionValue> fn,
                          std::vector<ValuePtr> args, int line);
    ValuePtr callMethod(std::shared_ptr<ClassInstance> inst,
                        const std::string& name,
                        std::vector<ValuePtr> args, int line);



    // Built-in methods on values
    ValuePtr callBuiltinMethod(ValuePtr obj, const std::string& method,
                               std::vector<ValuePtr> args, int line);

    // Standard library / built-in functions
    void registerBuiltins();

    // Output helpers
    void print(const std::string& s);
    void printErr(const std::string& s);
    std::string readLine();

    // Arithmetic helpers
    ValuePtr applyBinaryArith(const std::string& op, ValuePtr l, ValuePtr r, int line);
    ValuePtr applyBinaryCompare(const std::string& op, ValuePtr l, ValuePtr r, int line);

    // Env push/pop
    void pushEnv();
    void popEnv();

    // Debug trace
    void trace(const std::string& msg);

    // Kept-alive ASTs for imported modules
    std::vector<std::vector<StmtPtr>> moduleASTs_;

    // Simulated hardware and memory states
    std::unordered_map<int64_t, ValuePtr> virtualRAM_;
    std::unordered_map<int, int> gpioStates_;
    std::unordered_map<int, int> gpioModes_;
    int64_t nextAddress_ = 0x1000;
};
