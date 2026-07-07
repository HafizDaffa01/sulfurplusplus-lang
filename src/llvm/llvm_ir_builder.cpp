#ifdef ENABLE_LLVM

#include "llvm_ir_builder.hpp"

#include <iostream>
#include <stdexcept>

#include <llvm/IR/Verifier.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>

// New Pass Manager (LLVM 14+)
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/IR/PassManager.h>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

LLVMIRBuilder::LLVMIRBuilder(llvm::LLVMContext &ctx)
    : Ctx(ctx), Builder(ctx) {}

void LLVMIRBuilder::createModule(const std::string &name) {
    Mod = std::make_unique<llvm::Module>(name, Ctx);
}

// ---------------------------------------------------------------------------
// Type mapping
// ---------------------------------------------------------------------------

llvm::Type *LLVMIRBuilder::getLLVMType(const std::string &typeName) {
    if (typeName == "float_64" || typeName == "float" || typeName == "double")
        return llvm::Type::getDoubleTy(Ctx);
    if (typeName == "float_32")
        return llvm::Type::getFloatTy(Ctx);
    if (typeName == "bool")
        return llvm::Type::getInt1Ty(Ctx);
    if (typeName == "int_8")
        return llvm::Type::getInt8Ty(Ctx);
    if (typeName == "int_16")
        return llvm::Type::getInt16Ty(Ctx);
    if (typeName == "int_32")
        return llvm::Type::getInt32Ty(Ctx);
    if (typeName == "void")
        return llvm::Type::getVoidTy(Ctx);
    // Default / int / int_64 / auto / empty
    return llvm::Type::getInt64Ty(Ctx);
}

// ---------------------------------------------------------------------------
// Entry-block alloca (keeps mem2reg happy — no allocas in loop bodies)
// ---------------------------------------------------------------------------

llvm::AllocaInst *LLVMIRBuilder::createEntryBlockAlloca(
    llvm::Function *theFunction, const std::string &varName, llvm::Type *type) {
    llvm::IRBuilder<> tmpB(&theFunction->getEntryBlock(),
                           theFunction->getEntryBlock().begin());
    return tmpB.CreateAlloca(type, nullptr, varName);
}

// ---------------------------------------------------------------------------
// Type coercion helper
// ---------------------------------------------------------------------------

llvm::Value *LLVMIRBuilder::coerce(llvm::Value *val, llvm::Type *target,
                                    const std::string &name) {
    if (!val || !target || val->getType() == target)
        return val;

    // Integer sign-extend / truncate
    if (target->isIntegerTy() && val->getType()->isIntegerTy())
        return Builder.CreateSExtOrTrunc(val, target, name.empty() ? "sext" : name);

    // int → float
    if (target->isFloatingPointTy() && val->getType()->isIntegerTy())
        return Builder.CreateSIToFP(val, target, name.empty() ? "sitofp" : name);

    // float → int
    if (target->isIntegerTy() && val->getType()->isFloatingPointTy())
        return Builder.CreateFPToSI(val, target, name.empty() ? "fptosi" : name);

    // float widening/narrowing
    if (target->isFloatingPointTy() && val->getType()->isFloatingPointTy())
        return Builder.CreateFPCast(val, target, name.empty() ? "fpcast" : name);

    // i1 → i64
    if (target->isIntegerTy() && val->getType()->isIntegerTy(1))
        return Builder.CreateZExt(val, target, "zext");

    return val; // best effort
}

// ---------------------------------------------------------------------------
// Promote both operands: double wins over i64
// ---------------------------------------------------------------------------

void LLVMIRBuilder::promoteOperands(llvm::Value *&L, llvm::Value *&R) {
    bool lDbl = L->getType()->isDoubleTy();
    bool rDbl = R->getType()->isDoubleTy();
    if (lDbl && !rDbl)
        R = Builder.CreateSIToFP(R, llvm::Type::getDoubleTy(Ctx), "r2f");
    else if (!lDbl && rDbl)
        L = Builder.CreateSIToFP(L, llvm::Type::getDoubleTy(Ctx), "l2f");
    else if (!lDbl && !rDbl) {
        // Both integer but different widths — promote to i64
        auto *i64 = llvm::Type::getInt64Ty(Ctx);
        L = Builder.CreateSExtOrTrunc(L, i64);
        R = Builder.CreateSExtOrTrunc(R, i64);
    }
}

// ---------------------------------------------------------------------------
// Emit a condition and normalise to i1
// ---------------------------------------------------------------------------

llvm::Value *LLVMIRBuilder::emitCond(const Expr *expr) {
    llvm::Value *v = emitExpression(expr);
    if (!v) return nullptr;

    llvm::Type *t = v->getType();
    if (t->isIntegerTy(1))
        return v;
    if (t->isIntegerTy())
        return Builder.CreateICmpNE(
            v, llvm::ConstantInt::get(t, 0), "cond");
    if (t->isDoubleTy())
        return Builder.CreateFCmpONE(
            v, llvm::ConstantFP::get(Ctx, llvm::APFloat(0.0)), "cond");
    return v;
}

// ---------------------------------------------------------------------------
// Forward-declare a single function
// ---------------------------------------------------------------------------

llvm::Function *LLVMIRBuilder::forwardDeclare(const FnDeclStmt *func) {
    // Already declared?
    if (auto *existing = Mod->getFunction(func->name))
        return existing;

    std::vector<llvm::Type *> argTypes;
    for (const auto &p : func->params)
        argTypes.push_back(getLLVMType(p.second));

    llvm::Type *retType = getLLVMType(func->retType);
    auto *funcType = llvm::FunctionType::get(retType, argTypes, false);
    auto *llvmFunc = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, func->name, Mod.get());

    functionMap[func->name] = llvmFunc;
    return llvmFunc;
}

// ---------------------------------------------------------------------------
// Forward-declare all functions in a statement list
// ---------------------------------------------------------------------------

void LLVMIRBuilder::forwardDeclareAll(const std::vector<StmtPtr> &stmts) {
    for (const auto &s : stmts) {
        if (auto *fn = std::get_if<FnDeclStmt>(&s->data))
            forwardDeclare(fn);
    }
}

// ---------------------------------------------------------------------------
// Emit all functions in a statement list
// ---------------------------------------------------------------------------

void LLVMIRBuilder::emitAllFunctions(const std::vector<StmtPtr> &stmts) {
    for (const auto &s : stmts) {
        if (auto *fn = std::get_if<FnDeclStmt>(&s->data))
            emitFunction(fn);
    }
}

// ---------------------------------------------------------------------------
// Emit a single function (forward-declare first if needed)
// ---------------------------------------------------------------------------

llvm::Function *LLVMIRBuilder::emitFunction(const FnDeclStmt *func) {
    // 1. Ensure forward declaration exists
    llvm::Function *llvmFunc = forwardDeclare(func);

    // If the function already has a body, skip re-emission
    if (!llvmFunc->empty())
        return llvmFunc;

    currentFunction = llvmFunc;

    // 2. Create entry basic block
    llvm::BasicBlock *entryBB =
        llvm::BasicBlock::Create(Ctx, "entry", llvmFunc);
    Builder.SetInsertPoint(entryBB);

    // 3. Allocate parameters in the entry block and store arguments
    namedValues.clear();
    unsigned idx = 0;
    for (auto &arg : llvmFunc->args()) {
        const std::string &paramName = func->params[idx].first;
        arg.setName(paramName);
        llvm::AllocaInst *alloca =
            createEntryBlockAlloca(llvmFunc, paramName, arg.getType());
        Builder.CreateStore(&arg, alloca);
        namedValues[paramName] = alloca;
        ++idx;
    }

    // 4. Emit function body
    if (func->body)
        emitStatement(func->body.get());

    // 5. Ensure every basic block has a terminator
    for (auto &BB : *llvmFunc) {
        if (!BB.getTerminator()) {
            Builder.SetInsertPoint(&BB);
            llvm::Type *retTy = llvmFunc->getReturnType();
            if (retTy->isVoidTy())
                Builder.CreateRetVoid();
            else if (retTy->isDoubleTy())
                Builder.CreateRet(
                    llvm::ConstantFP::get(Ctx, llvm::APFloat(0.0)));
            else
                Builder.CreateRet(llvm::ConstantInt::get(retTy, 0));
        }
    }

    // 6. Verify
    std::string errBuf;
    llvm::raw_string_ostream errStream(errBuf);
    if (llvm::verifyFunction(*llvmFunc, &errStream)) {
        std::cerr << "[LLVM] Function verification failed for '"
                  << func->name << "': " << errBuf << "\n";
        llvmFunc->eraseFromParent();
        functionMap.erase(func->name);
        return nullptr;
    }

    return llvmFunc;
}

// ---------------------------------------------------------------------------
// Statement code generation
// ---------------------------------------------------------------------------

void LLVMIRBuilder::emitStatement(const Stmt *stmt) {
    if (!stmt) return;

    // If the current block already has a terminator (e.g. after a return),
    // skip unreachable statements.
    if (Builder.GetInsertBlock() && Builder.GetInsertBlock()->getTerminator())
        return;

    std::visit([&](const auto &node) {
        using T = std::decay_t<decltype(node)>;

        // ----------------------------------------------------------------
        // Block
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, BlockStmt>) {
            for (const auto &s : node.stmts) {
                if (Builder.GetInsertBlock() &&
                    Builder.GetInsertBlock()->getTerminator())
                    break;
                emitStatement(s.get());
            }
        }

        // ----------------------------------------------------------------
        // Variable declaration
        // ----------------------------------------------------------------
        else if constexpr (std::is_same_v<T, VarDeclStmt>) {
            llvm::Type *declaredType = getLLVMType(node.type);
            llvm::AllocaInst *alloca =
                createEntryBlockAlloca(currentFunction, node.name, declaredType);
            namedValues[node.name] = alloca;

            llvm::Value *initVal = nullptr;
            if (node.initializer) {
                initVal = emitExpression(node.initializer.get());
                if (initVal)
                    initVal = coerce(initVal, declaredType);
            }
            if (!initVal) {
                if (declaredType->isDoubleTy())
                    initVal = llvm::ConstantFP::get(Ctx, llvm::APFloat(0.0));
                else
                    initVal = llvm::ConstantInt::get(declaredType, 0);
            }
            Builder.CreateStore(initVal, alloca);
        }

        // ----------------------------------------------------------------
        // Return
        // ----------------------------------------------------------------
        else if constexpr (std::is_same_v<T, ReturnStmt>) {
            if (node.value) {
                llvm::Value *retVal = emitExpression(node.value.get());
                llvm::Type *retTy = currentFunction->getReturnType();
                if (retVal)
                    retVal = coerce(retVal, retTy, "ret");
                if (retVal)
                    Builder.CreateRet(retVal);
                else
                    Builder.CreateRetVoid();
            } else {
                Builder.CreateRetVoid();
            }
        }

        // ----------------------------------------------------------------
        // If / else
        // ----------------------------------------------------------------
        else if constexpr (std::is_same_v<T, IfStmt>) {
            llvm::Value *condVal = emitCond(node.cond.get());
            if (!condVal) return;

            llvm::BasicBlock *thenBB =
                llvm::BasicBlock::Create(Ctx, "then", currentFunction);
            llvm::BasicBlock *elseBB =
                llvm::BasicBlock::Create(Ctx, "else");
            llvm::BasicBlock *mergeBB =
                llvm::BasicBlock::Create(Ctx, "ifcont");

            Builder.CreateCondBr(condVal, thenBB,
                                  node.elseBranch ? elseBB : mergeBB);

            // Then branch
            Builder.SetInsertPoint(thenBB);
            emitStatement(node.thenBranch.get());
            if (!Builder.GetInsertBlock()->getTerminator())
                Builder.CreateBr(mergeBB);

            // Else branch
            if (node.elseBranch) {
                currentFunction->insert(currentFunction->end(), elseBB);
                Builder.SetInsertPoint(elseBB);
                emitStatement(node.elseBranch.get());
                if (!Builder.GetInsertBlock()->getTerminator())
                    Builder.CreateBr(mergeBB);
            }

            currentFunction->insert(currentFunction->end(), mergeBB);
            Builder.SetInsertPoint(mergeBB);
        }

        // ----------------------------------------------------------------
        // While loop
        // ----------------------------------------------------------------
        else if constexpr (std::is_same_v<T, WhileStmt>) {
            llvm::BasicBlock *condBB =
                llvm::BasicBlock::Create(Ctx, "while.cond", currentFunction);
            llvm::BasicBlock *bodyBB =
                llvm::BasicBlock::Create(Ctx, "while.body", currentFunction);
            llvm::BasicBlock *afterBB =
                llvm::BasicBlock::Create(Ctx, "while.end", currentFunction);

            loopStack.push_back({condBB, afterBB});

            Builder.CreateBr(condBB);
            Builder.SetInsertPoint(condBB);

            llvm::Value *condVal = emitCond(node.cond.get());
            if (!condVal) { loopStack.pop_back(); return; }
            Builder.CreateCondBr(condVal, bodyBB, afterBB);

            Builder.SetInsertPoint(bodyBB);
            emitStatement(node.body.get());
            if (!Builder.GetInsertBlock()->getTerminator())
                Builder.CreateBr(condBB);

            loopStack.pop_back();
            Builder.SetInsertPoint(afterBB);
        }

        // ----------------------------------------------------------------
        // For loop  (range over integer: for x in range(n))
        // We lower a ForStmt into a counted while loop for numeric ranges.
        // Non-numeric iterables fall back gracefully (emit nothing in IR).
        // ----------------------------------------------------------------
        else if constexpr (std::is_same_v<T, ForStmt>) {
            // Attempt to detect range(N) pattern
            bool isRange = false;
            const Expr *rangeArg = nullptr;
            if (auto *call = std::get_if<CallExpr>(&node.iterable->data)) {
                if (auto *ident = std::get_if<IdentExpr>(&call->callee->data)) {
                    if (ident->name == "range" && call->args.size() == 1) {
                        isRange = true;
                        rangeArg = call->args[0].get();
                    }
                }
            }

            if (isRange && rangeArg) {
                // Emit:  i = 0; while (i < N) { var = i; body; i++; }
                auto *i64 = llvm::Type::getInt64Ty(Ctx);
                llvm::AllocaInst *iAlloca =
                    createEntryBlockAlloca(currentFunction, node.var + ".idx", i64);
                Builder.CreateStore(llvm::ConstantInt::get(i64, 0), iAlloca);

                llvm::AllocaInst *varAlloca =
                    createEntryBlockAlloca(currentFunction, node.var, i64);
                namedValues[node.var] = varAlloca;

                llvm::Value *limit = emitExpression(rangeArg);
                limit = coerce(limit, i64);

                llvm::BasicBlock *condBB =
                    llvm::BasicBlock::Create(Ctx, "for.cond", currentFunction);
                llvm::BasicBlock *bodyBB =
                    llvm::BasicBlock::Create(Ctx, "for.body", currentFunction);
                llvm::BasicBlock *incrBB =
                    llvm::BasicBlock::Create(Ctx, "for.incr", currentFunction);
                llvm::BasicBlock *afterBB =
                    llvm::BasicBlock::Create(Ctx, "for.end", currentFunction);

                loopStack.push_back({incrBB, afterBB});

                Builder.CreateBr(condBB);
                Builder.SetInsertPoint(condBB);
                llvm::Value *idx = Builder.CreateLoad(i64, iAlloca, "i");
                llvm::Value *cond = Builder.CreateICmpSLT(idx, limit, "cmp");
                Builder.CreateCondBr(cond, bodyBB, afterBB);

                Builder.SetInsertPoint(bodyBB);
                Builder.CreateStore(idx, varAlloca);
                emitStatement(node.body.get());
                if (!Builder.GetInsertBlock()->getTerminator())
                    Builder.CreateBr(incrBB);

                // Increment
                Builder.SetInsertPoint(incrBB);
                llvm::Value *next = Builder.CreateAdd(
                    Builder.CreateLoad(i64, iAlloca, "i2"),
                    llvm::ConstantInt::get(i64, 1), "i.next");
                Builder.CreateStore(next, iAlloca);
                Builder.CreateBr(condBB);

                loopStack.pop_back();
                Builder.SetInsertPoint(afterBB);
            }
            // Non-range for-loops cannot be lowered to IR without a runtime —
            // they stay interpreted.  Do nothing so the JIT function still
            // compiles for the parts that can be optimised.
        }

        // ----------------------------------------------------------------
        // Break / Continue
        // ----------------------------------------------------------------
        else if constexpr (std::is_same_v<T, BreakStmt>) {
            if (!loopStack.empty())
                Builder.CreateBr(loopStack.back().afterBB);
        }
        else if constexpr (std::is_same_v<T, ContinueStmt>) {
            if (!loopStack.empty())
                Builder.CreateBr(loopStack.back().condBB);
        }

        // ----------------------------------------------------------------
        // Expression statement
        // ----------------------------------------------------------------
        else if constexpr (std::is_same_v<T, ExprStmt>) {
            emitExpression(node.expr.get());
        }

        // Other statement kinds are not lowered to IR; they stay interpreted.

    }, stmt->data);
}

// ---------------------------------------------------------------------------
// Expression code generation
// ---------------------------------------------------------------------------

llvm::Value *LLVMIRBuilder::emitExpression(const Expr *expr) {
    if (!expr) return nullptr;

    return std::visit([&](const auto &node) -> llvm::Value * {
        using T = std::decay_t<decltype(node)>;

        // ----------------------------------------------------------------
        // Literals
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, IntLitExpr>)
            return llvm::ConstantInt::get(Ctx, llvm::APInt(64, node.value, true));

        else if constexpr (std::is_same_v<T, FloatLitExpr>)
            return llvm::ConstantFP::get(Ctx, llvm::APFloat(node.value));

        else if constexpr (std::is_same_v<T, BoolLitExpr>)
            return llvm::ConstantInt::get(Ctx, llvm::APInt(1, node.value ? 1 : 0));

        else if constexpr (std::is_same_v<T, NullLitExpr>)
            return llvm::ConstantInt::get(Ctx, llvm::APInt(64, 0));

        // ----------------------------------------------------------------
        // Identifier (variable load)
        // ----------------------------------------------------------------
        else if constexpr (std::is_same_v<T, IdentExpr>) {
            auto it = namedValues.find(node.name);
            if (it != namedValues.end()) {
                llvm::AllocaInst *alloca = it->second;
                return Builder.CreateLoad(alloca->getAllocatedType(),
                                          alloca, node.name.c_str());
            }
            // Function reference
            auto fit = functionMap.find(node.name);
            if (fit != functionMap.end())
                return fit->second;

            std::cerr << "[LLVM] CodeGen: undefined variable '"
                      << node.name << "'\n";
            return nullptr;
        }

        // ----------------------------------------------------------------
        // Assignment
        // ----------------------------------------------------------------
        else if constexpr (std::is_same_v<T, AssignExpr>) {
            llvm::Value *rhs = emitExpression(node.value.get());
            if (!rhs) return nullptr;

            if (auto *ident = std::get_if<IdentExpr>(&node.target->data)) {
                auto it = namedValues.find(ident->name);
                if (it == namedValues.end()) {
                    std::cerr << "[LLVM] CodeGen: assignment to undeclared '"
                              << ident->name << "'\n";
                    return nullptr;
                }
                llvm::AllocaInst *alloca = it->second;
                rhs = coerce(rhs, alloca->getAllocatedType(), "assign");
                // Handle compound assignment operators
                if (node.op != "=") {
                    llvm::Value *cur = Builder.CreateLoad(
                        alloca->getAllocatedType(), alloca, "cur");
                    promoteOperands(cur, rhs);
                    bool fp = cur->getType()->isDoubleTy();
                    if (node.op == "+=")
                        rhs = fp ? Builder.CreateFAdd(cur, rhs)
                                 : Builder.CreateAdd(cur, rhs);
                    else if (node.op == "-=")
                        rhs = fp ? Builder.CreateFSub(cur, rhs)
                                 : Builder.CreateSub(cur, rhs);
                    else if (node.op == "*=")
                        rhs = fp ? Builder.CreateFMul(cur, rhs)
                                 : Builder.CreateMul(cur, rhs);
                    else if (node.op == "/=")
                        rhs = fp ? Builder.CreateFDiv(cur, rhs)
                                 : Builder.CreateSDiv(cur, rhs);
                    rhs = coerce(rhs, alloca->getAllocatedType(), "compound");
                }
                Builder.CreateStore(rhs, alloca);
                return rhs;
            }
            std::cerr << "[LLVM] CodeGen: lvalue must be a simple identifier\n";
            return nullptr;
        }

        // ----------------------------------------------------------------
        // Unary operators
        // ----------------------------------------------------------------
        else if constexpr (std::is_same_v<T, UnaryExpr>) {
            llvm::Value *operand = emitExpression(node.operand.get());
            if (!operand) return nullptr;

            if (node.op == "-") {
                if (operand->getType()->isDoubleTy())
                    return Builder.CreateFNeg(operand, "fneg");
                return Builder.CreateNeg(operand, "neg");
            }
            if (node.op == "!") {
                llvm::Value *cond = coerce(
                    operand, llvm::Type::getInt1Ty(Ctx));
                return Builder.CreateNot(cond, "not");
            }
            if (node.op == "~")
                return Builder.CreateNot(operand, "bitnot");

            return operand;
        }

        // ----------------------------------------------------------------
        // Binary operators
        // ----------------------------------------------------------------
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
            // Short-circuit logical operators
            if (node.op == "&&" || node.op == "and") {
                llvm::Value *lv = emitCond(node.left.get());
                if (!lv) return nullptr;
                llvm::BasicBlock *rhsBB =
                    llvm::BasicBlock::Create(Ctx, "and.rhs", currentFunction);
                llvm::BasicBlock *mergeBB =
                    llvm::BasicBlock::Create(Ctx, "and.end", currentFunction);
                llvm::BasicBlock *lhsBB = Builder.GetInsertBlock();
                Builder.CreateCondBr(lv, rhsBB, mergeBB);

                Builder.SetInsertPoint(rhsBB);
                llvm::Value *rv = emitCond(node.right.get());
                if (!rv) rv = llvm::ConstantInt::getFalse(Ctx);
                llvm::BasicBlock *rhsEndBB = Builder.GetInsertBlock();
                Builder.CreateBr(mergeBB);

                Builder.SetInsertPoint(mergeBB);
                llvm::PHINode *phi =
                    Builder.CreatePHI(llvm::Type::getInt1Ty(Ctx), 2, "and");
                phi->addIncoming(llvm::ConstantInt::getFalse(Ctx), lhsBB);
                phi->addIncoming(rv, rhsEndBB);
                return phi;
            }
            if (node.op == "||" || node.op == "or") {
                llvm::Value *lv = emitCond(node.left.get());
                if (!lv) return nullptr;
                llvm::BasicBlock *rhsBB =
                    llvm::BasicBlock::Create(Ctx, "or.rhs", currentFunction);
                llvm::BasicBlock *mergeBB =
                    llvm::BasicBlock::Create(Ctx, "or.end", currentFunction);
                llvm::BasicBlock *lhsBB = Builder.GetInsertBlock();
                Builder.CreateCondBr(lv, mergeBB, rhsBB);

                Builder.SetInsertPoint(rhsBB);
                llvm::Value *rv = emitCond(node.right.get());
                if (!rv) rv = llvm::ConstantInt::getFalse(Ctx);
                llvm::BasicBlock *rhsEndBB = Builder.GetInsertBlock();
                Builder.CreateBr(mergeBB);

                Builder.SetInsertPoint(mergeBB);
                llvm::PHINode *phi =
                    Builder.CreatePHI(llvm::Type::getInt1Ty(Ctx), 2, "or");
                phi->addIncoming(llvm::ConstantInt::getTrue(Ctx), lhsBB);
                phi->addIncoming(rv, rhsEndBB);
                return phi;
            }

            llvm::Value *L = emitExpression(node.left.get());
            llvm::Value *R = emitExpression(node.right.get());
            if (!L || !R) return nullptr;

            promoteOperands(L, R);
            bool fp = L->getType()->isDoubleTy();

            // Arithmetic
            if (node.op == "+") return fp ? Builder.CreateFAdd(L, R, "fadd")
                                           : Builder.CreateAdd(L, R, "add");
            if (node.op == "-") return fp ? Builder.CreateFSub(L, R, "fsub")
                                           : Builder.CreateSub(L, R, "sub");
            if (node.op == "*") return fp ? Builder.CreateFMul(L, R, "fmul")
                                           : Builder.CreateMul(L, R, "mul");
            if (node.op == "/") return fp ? Builder.CreateFDiv(L, R, "fdiv")
                                           : Builder.CreateSDiv(L, R, "div");
            if (node.op == "%") return fp ? Builder.CreateFRem(L, R, "frem")
                                           : Builder.CreateSRem(L, R, "rem");
            // Bitwise
            if (node.op == "&") return Builder.CreateAnd(L, R, "band");
            if (node.op == "|") return Builder.CreateOr(L, R, "bor");
            if (node.op == "^") return Builder.CreateXor(L, R, "bxor");
            if (node.op == "<<") return Builder.CreateShl(L, R, "shl");
            if (node.op == ">>") return Builder.CreateAShr(L, R, "ashr");

            // Comparison — always returns i1
            if (node.op == "==")
                return fp ? Builder.CreateFCmpOEQ(L, R, "eq")
                           : Builder.CreateICmpEQ(L, R, "eq");
            if (node.op == "!=")
                return fp ? Builder.CreateFCmpONE(L, R, "ne")
                           : Builder.CreateICmpNE(L, R, "ne");
            if (node.op == "<")
                return fp ? Builder.CreateFCmpOLT(L, R, "lt")
                           : Builder.CreateICmpSLT(L, R, "lt");
            if (node.op == "<=")
                return fp ? Builder.CreateFCmpOLE(L, R, "le")
                           : Builder.CreateICmpSLE(L, R, "le");
            if (node.op == ">")
                return fp ? Builder.CreateFCmpOGT(L, R, "gt")
                           : Builder.CreateICmpSGT(L, R, "gt");
            if (node.op == ">=")
                return fp ? Builder.CreateFCmpOGE(L, R, "ge")
                           : Builder.CreateICmpSGE(L, R, "ge");

            std::cerr << "[LLVM] CodeGen: unsupported binary op '"
                      << node.op << "'\n";
            return nullptr;
        }

        // ----------------------------------------------------------------
        // Function call
        // ----------------------------------------------------------------
        else if constexpr (std::is_same_v<T, CallExpr>) {
            // Resolve callee name
            std::string calleeName;
            if (auto *id = std::get_if<IdentExpr>(&node.callee->data))
                calleeName = id->name;
            else {
                std::cerr << "[LLVM] CodeGen: callee must be a simple identifier\n";
                return nullptr;
            }

            // Look up in module (covers recursion + forward declarations)
            llvm::Function *calleeFn = Mod->getFunction(calleeName);
            if (!calleeFn) {
                auto fit = functionMap.find(calleeName);
                if (fit != functionMap.end())
                    calleeFn = fit->second;
            }
            if (!calleeFn) {
                std::cerr << "[LLVM] CodeGen: unknown function '"
                          << calleeName << "'\n";
                return nullptr;
            }

            // Build argument list with coercion
            std::vector<llvm::Value *> argsV;
            unsigned i = 0;
            for (const auto &arg : node.args) {
                llvm::Value *v = emitExpression(arg.get());
                if (!v) return nullptr;
                if (i < calleeFn->arg_size()) {
                    llvm::Type *paramTy =
                        calleeFn->getFunctionType()->getParamType(i);
                    v = coerce(v, paramTy, "arg");
                }
                argsV.push_back(v);
                ++i;
            }

            // Recursive call gets a name; void calls get none
            bool returnsVoid = calleeFn->getReturnType()->isVoidTy();
            return Builder.CreateCall(
                calleeFn, argsV, returnsVoid ? "" : "calltmp");
        }

        // ----------------------------------------------------------------
        // Ternary  cond ? then : else
        // ----------------------------------------------------------------
        else if constexpr (std::is_same_v<T, TernaryExpr>) {
            llvm::Value *cond = emitCond(node.cond.get());
            if (!cond) return nullptr;

            llvm::BasicBlock *thenBB =
                llvm::BasicBlock::Create(Ctx, "tern.then", currentFunction);
            llvm::BasicBlock *elseBB =
                llvm::BasicBlock::Create(Ctx, "tern.else", currentFunction);
            llvm::BasicBlock *mergeBB =
                llvm::BasicBlock::Create(Ctx, "tern.end", currentFunction);

            Builder.CreateCondBr(cond, thenBB, elseBB);

            Builder.SetInsertPoint(thenBB);
            llvm::Value *thenVal = emitExpression(node.thenExpr.get());
            llvm::BasicBlock *thenEnd = Builder.GetInsertBlock();
            Builder.CreateBr(mergeBB);

            Builder.SetInsertPoint(elseBB);
            llvm::Value *elseVal = emitExpression(node.elseExpr.get());
            llvm::BasicBlock *elseEnd = Builder.GetInsertBlock();
            Builder.CreateBr(mergeBB);

            Builder.SetInsertPoint(mergeBB);
            if (!thenVal || !elseVal) return nullptr;

            // Unify types
            promoteOperands(thenVal, elseVal);

            llvm::PHINode *phi =
                Builder.CreatePHI(thenVal->getType(), 2, "tern");
            phi->addIncoming(thenVal, thenEnd);
            phi->addIncoming(elseVal, elseEnd);
            return phi;
        }

        // All other expression kinds are not lowered to IR.
        return nullptr;

    }, expr->data);
}

// ---------------------------------------------------------------------------
// Module optimization — LLVM New Pass Manager, O3
// ---------------------------------------------------------------------------

void LLVMIRBuilder::optimizeModule() {
    if (!Mod) return;

    llvm::LoopAnalysisManager      LAM;
    llvm::FunctionAnalysisManager  FAM;
    llvm::CGSCCAnalysisManager     CGAM;
    llvm::ModuleAnalysisManager    MAM;

    llvm::PassBuilder PB;

    // Register all standard analyses
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // Build the O3 default pipeline
    llvm::ModulePassManager MPM =
        PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);

    MPM.run(*Mod, MAM);
}

#endif // ENABLE_LLVM
