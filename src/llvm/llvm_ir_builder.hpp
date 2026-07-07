#pragma once

#ifdef ENABLE_LLVM

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>

#include "../../include/ast.hpp"

// ---------------------------------------------------------------------------
// Loop context for break/continue support
// ---------------------------------------------------------------------------
struct LoopContext {
    llvm::BasicBlock *condBB;   // Where to branch for 'continue'
    llvm::BasicBlock *afterBB;  // Where to branch for 'break'
};

// ---------------------------------------------------------------------------
// LLVMIRBuilder
//
// Translates Sulfur++ AST nodes directly into LLVM IR.
//
// Key design decisions:
//  - Forward-declare all functions in a first pass so mutual/recursive calls
//    work correctly before the body is emitted.
//  - Keep a loop-context stack for break/continue codegen.
//  - Provide optimizeModule() to run the LLVM New Pass Manager O3 pipeline
//    before the module is handed off to the JIT or written to disk.
// ---------------------------------------------------------------------------
class LLVMIRBuilder {
public:
    explicit LLVMIRBuilder(llvm::LLVMContext &ctx);

    // Create a new module with the given name
    void createModule(const std::string &name);

    // -----------------------------------------------------------------------
    // High-level entry points
    // -----------------------------------------------------------------------

    // Forward-declare every function in 'stmts' so recursive / mutual
    // calls resolve correctly.  Must be called before emitAllFunctions.
    void forwardDeclareAll(const std::vector<StmtPtr> &stmts);

    // Emit IR for all top-level function declarations
    void emitAllFunctions(const std::vector<StmtPtr> &stmts);

    // Emit a single function (with forward declaration if not yet done)
    llvm::Function *emitFunction(const FnDeclStmt *func);

    // Emit a single statement (public so the interpreter can call it)
    void emitStatement(const Stmt *stmt);

    // Emit an expression and return its LLVM Value*
    llvm::Value *emitExpression(const Expr *expr);

    // -----------------------------------------------------------------------
    // Optimization
    // -----------------------------------------------------------------------

    // Run the LLVM New Pass Manager at O3 on the current module.
    // Call this after all IR has been emitted but before JIT compilation.
    void optimizeModule();

    // -----------------------------------------------------------------------
    // Module access
    // -----------------------------------------------------------------------
    llvm::Module *getModule() { return Mod.get(); }
    std::unique_ptr<llvm::Module> takeModule() { return std::move(Mod); }

private:
    llvm::LLVMContext &Ctx;
    std::unique_ptr<llvm::Module> Mod;
    llvm::IRBuilder<> Builder;

    // Per-function local variable allocas
    std::unordered_map<std::string, llvm::AllocaInst *> namedValues;

    // Global function registry (populated by forward declarations + emitFunction)
    std::unordered_map<std::string, llvm::Function *> functionMap;

    // The function currently being code-generated
    llvm::Function *currentFunction = nullptr;

    // Stack of enclosing loop contexts (for break/continue)
    std::vector<LoopContext> loopStack;

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    // Map a Sulfur++ type name to an LLVM type
    llvm::Type *getLLVMType(const std::string &typeName);

    // Create an alloca in the entry block (for mem2reg friendliness)
    llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *theFunction,
                                              const std::string &varName,
                                              llvm::Type *type);

    // Coerce 'val' to 'targetType' if possible (int <-> float promotion)
    llvm::Value *coerce(llvm::Value *val, llvm::Type *targetType,
                        const std::string &name = "");

    // Promote both operands to the widest type (double wins over i64)
    void promoteOperands(llvm::Value *&L, llvm::Value *&R);

    // Forward-declare a single function (does nothing if already declared)
    llvm::Function *forwardDeclare(const FnDeclStmt *func);

    // Emit a condition value, normalising it to i1
    llvm::Value *emitCond(const Expr *expr);
};

#endif // ENABLE_LLVM
