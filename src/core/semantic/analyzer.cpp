#include "semantic/analyzer.hpp"
#include "error.hpp"
#include <iostream>

SemanticAnalyzer::SemanticAnalyzer()
    : symTable_(), typeChecker_(symTable_) {}

void SemanticAnalyzer::analyze(const std::vector<StmtPtr>& stmts, const std::string& filename) {
    currentFilename_ = filename;
    diagnostics_.clear();

    // Pass 1: Forward declarations of functions, classes, and structs
    for (const auto& s : stmts) {
        if (!s) continue;
        if (auto* fn = std::get_if<FnDeclStmt>(&s->data)) {
            std::vector<TypePtr> paramTypes;
            for (const auto& p : fn->params) {
                paramTypes.push_back(parseTypeName(p.second));
            }
            TypePtr retType = parseTypeName(fn->retType);
            Symbol sym(fn->name, makeFunctionType(paramTypes, retType), true, fn->line);
            sym.isFunction = true;
            symTable_.define(sym);
        } else if (auto* cls = std::get_if<ClassDeclStmt>(&s->data)) {
            Symbol sym(cls->name, makeClassType(cls->name), true, cls->line);
            sym.isClass = true;
            symTable_.define(sym);
        } else if (auto* strct = std::get_if<StructDeclStmt>(&s->data)) {
            Symbol sym(strct->name, makeStructType(strct->name), true, strct->line);
            sym.isStruct = true;
            symTable_.define(sym);
        }
    }

    // Pass 2: Full statement & expression analysis
    for (const auto& s : stmts) {
        if (s) visitStmt(*s);
    }
}

bool SemanticAnalyzer::hasErrors() const {
    for (const auto& d : diagnostics_) {
        if (d.severity == DiagnosticSeverity::ERROR || d.severity == DiagnosticSeverity::FATAL) {
            return true;
        }
    }
    return false;
}

void SemanticAnalyzer::reportError(const std::string& message, int line, int col, const std::string& code, const std::string& hint) {
    diagnostics_.push_back({DiagnosticSeverity::ERROR, code, message, currentFilename_, line, col, 1, hint});
}

void SemanticAnalyzer::reportTypeError(const std::string& message, int line, int col, const std::string& hint) {
    reportError(message, line, col, "E_TYPE_406", hint);
}

void SemanticAnalyzer::visitStmt(const Stmt& stmt) {
    std::visit([&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, BlockStmt>) {
            visitBlock(node);
        } else if constexpr (std::is_same_v<T, VarDeclStmt>) {
            visitVarDecl(node);
        } else if constexpr (std::is_same_v<T, FnDeclStmt>) {
            visitFnDecl(node);
        } else if constexpr (std::is_same_v<T, ClassDeclStmt>) {
            visitClassDecl(node);
        } else if constexpr (std::is_same_v<T, StructDeclStmt>) {
            visitStructDecl(node);
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            visitIf(node);
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            visitWhile(node);
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            visitFor(node);
        } else if constexpr (std::is_same_v<T, ForCStyleStmt>) {
            visitForCStyle(node);
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            visitReturn(node);
        } else if constexpr (std::is_same_v<T, BreakStmt>) {
            visitBreak(node);
        } else if constexpr (std::is_same_v<T, ContinueStmt>) {
            visitContinue(node);
        } else if constexpr (std::is_same_v<T, DeferStmt>) {
            visitDefer(node);
        } else if constexpr (std::is_same_v<T, TryCatchStmt>) {
            visitTryCatch(node);
        } else if constexpr (std::is_same_v<T, MatchStmt>) {
            visitMatch(node);
        } else if constexpr (std::is_same_v<T, UnsafeStmt>) {
            visitUnsafe(node);
        } else if constexpr (std::is_same_v<T, ImportStmt>) {
            visitImport(node);
        } else if constexpr (std::is_same_v<T, ExportStmt>) {
            visitExport(node);
        } else if constexpr (std::is_same_v<T, ExposeStmt>) {
            visitExpose(node);
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
            if (node.expr) visitExpr(*node.expr);
        }
    }, stmt.data);
}

void SemanticAnalyzer::visitBlock(const BlockStmt& stmt, ScopeKind kind, TypePtr fnRetType) {
    symTable_.enterScope("Block", kind, fnRetType);
    for (const auto& s : stmt.stmts) {
        if (s) visitStmt(*s);
    }
    symTable_.exitScope();
}

void SemanticAnalyzer::visitVarDecl(const VarDeclStmt& stmt) {
    TypePtr declType = parseTypeName(stmt.type);
    TypePtr initType = nullptr;

    if (stmt.initializer) {
        initType = visitExpr(*stmt.initializer);
        if (!declType->isAny()) {
            typeChecker_.checkAssignment(declType, initType, stmt.line);
        }
    }

    TypePtr finalType = !declType->isAny() ? declType : (initType ? initType : makeAnyType());
    bool isConst = (stmt.keyword == "let");
    Symbol sym(stmt.name, finalType, isConst, stmt.line);
    sym.isInitialized = (stmt.initializer != nullptr);

    if (symTable_.lookupCurrent(stmt.name)) {
        // Warning or error on redeclaration in same scope
    }
    symTable_.define(sym);
}

void SemanticAnalyzer::visitFnDecl(const FnDeclStmt& stmt) {
    std::vector<TypePtr> paramTypes;
    for (const auto& p : stmt.params) {
        paramTypes.push_back(parseTypeName(p.second));
    }
    TypePtr retType = parseTypeName(stmt.retType);

    std::string scopeName = stmt.name.empty() ? "fn" : stmt.name;
    symTable_.enterScope(scopeName, ScopeKind::Function, retType);
    for (size_t i = 0; i < stmt.params.size(); i++) {
        Symbol paramSym(stmt.params[i].first, paramTypes[i], false, stmt.line);
        paramSym.isInitialized = true;
        symTable_.define(paramSym);
    }

    if (stmt.body) {
        if (auto* b = std::get_if<BlockStmt>(&stmt.body->data)) {
            for (const auto& s : b->stmts) {
                if (s) visitStmt(*s);
            }
        } else {
            visitStmt(*stmt.body);
        }
    }
    symTable_.exitScope();
}

void SemanticAnalyzer::visitClassDecl(const ClassDeclStmt& stmt) {
    symTable_.enterScope(stmt.name, ScopeKind::Class);
    symTable_.define(Symbol("this", makeClassType(stmt.name), true, stmt.line));
    symTable_.define(Symbol("self", makeClassType(stmt.name), true, stmt.line));

    for (const auto& m : stmt.members) {
        if (m) visitStmt(*m);
    }
    symTable_.exitScope();
}

void SemanticAnalyzer::visitStructDecl(const StructDeclStmt& stmt) {
    symTable_.enterScope("struct " + stmt.name, ScopeKind::Class);
    for (const auto& f : stmt.fields) {
        symTable_.define(Symbol(f.first, parseTypeName(f.second), false, stmt.line));
    }
    symTable_.exitScope();
}

void SemanticAnalyzer::visitIf(const IfStmt& stmt) {
    if (stmt.cond) visitExpr(*stmt.cond);
    if (stmt.thenBranch) visitStmt(*stmt.thenBranch);
    if (stmt.elseBranch) visitStmt(*stmt.elseBranch);
}

void SemanticAnalyzer::visitWhile(const WhileStmt& stmt) {
    symTable_.enterScope("while", ScopeKind::Loop);
    if (stmt.cond) visitExpr(*stmt.cond);
    if (stmt.body) visitStmt(*stmt.body);
    symTable_.exitScope();
}

void SemanticAnalyzer::visitFor(const ForStmt& stmt) {
    symTable_.enterScope("for", ScopeKind::Loop);
    if (stmt.iterable) visitExpr(*stmt.iterable);
    symTable_.define(Symbol(stmt.var, makeAnyType(), false, stmt.line));
    if (stmt.body) visitStmt(*stmt.body);
    symTable_.exitScope();
}

void SemanticAnalyzer::visitForCStyle(const ForCStyleStmt& stmt) {
    symTable_.enterScope("for", ScopeKind::Loop);
    if (stmt.init) visitStmt(*stmt.init);
    if (stmt.cond) visitExpr(*stmt.cond);
    if (stmt.post) visitExpr(*stmt.post);
    if (stmt.body) visitStmt(*stmt.body);
    symTable_.exitScope();
}

void SemanticAnalyzer::visitReturn(const ReturnStmt& stmt) {
    if (!symTable_.isInFunction()) {
        throw SemanticError("'return' statement outside of function", stmt.line, "E_SEMANTIC_400", "Place 'return' inside a function body.", 1);
    }
    TypePtr retType = makeVoidType();
    if (stmt.value) {
        retType = visitExpr(*stmt.value);
    }
    TypePtr expected = symTable_.currentFunctionReturnType();
    if (!expected->isAny() && !expected->isVoid()) {
        typeChecker_.checkAssignment(expected, retType, stmt.line);
    }
}

void SemanticAnalyzer::visitBreak(const BreakStmt& stmt) {
    if (!symTable_.isInLoop()) {
        throw SemanticError("'break' statement can only be used inside a loop", stmt.line, "E_SEMANTIC_400", "Remove 'break' or place inside 'for' or 'while' loop.", 1);
    }
}

void SemanticAnalyzer::visitContinue(const ContinueStmt& stmt) {
    if (!symTable_.isInLoop()) {
        throw SemanticError("'continue' statement can only be used inside a loop", stmt.line, "E_SEMANTIC_400", "Remove 'continue' or place inside 'for' or 'while' loop.", 1);
    }
}

void SemanticAnalyzer::visitDefer(const DeferStmt& stmt) {
    if (stmt.body) visitStmt(*stmt.body);
}

void SemanticAnalyzer::visitTryCatch(const TryCatchStmt& stmt) {
    if (stmt.tryBody) {
        try {
            visitStmt(*stmt.tryBody);
        } catch (const SulfurError&) {
            // Handled at runtime by catch block
        }
    }
    if (stmt.catchBody) {
        symTable_.enterScope("catch", ScopeKind::Block);
        symTable_.define(Symbol(stmt.catchVar, makeDictType(makeStringType(), makeAnyType()), false, stmt.line));
        visitStmt(*stmt.catchBody);
        symTable_.exitScope();
    }
    if (stmt.finallyBody) visitStmt(*stmt.finallyBody);
}

void SemanticAnalyzer::visitMatch(const MatchStmt& stmt) {
    if (stmt.value) visitExpr(*stmt.value);
    for (const auto& c : stmt.cases) {
        if (c.pattern) visitExpr(*c.pattern);
        if (c.body) visitStmt(*c.body);
    }
}

void SemanticAnalyzer::visitUnsafe(const UnsafeStmt& stmt) {
    if (stmt.body) visitStmt(*stmt.body);
}

void SemanticAnalyzer::visitImport(const ImportStmt& stmt) {
    std::string alias = !stmt.alias.empty() ? stmt.alias : stmt.pkg;
    symTable_.define(Symbol(alias, makeDictType(makeStringType(), makeAnyType()), true, stmt.line));
}

void SemanticAnalyzer::visitExport(const ExportStmt& stmt) {
    // Export registration
}

void SemanticAnalyzer::visitExpose(const ExposeStmt& stmt) {
    symTable_.define(Symbol(stmt.alias, makeAnyType(), false, stmt.line));
}

TypePtr SemanticAnalyzer::visitExpr(const Expr& expr) {
    return std::visit([&](const auto& node) -> TypePtr {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, IdentExpr>) {
            Symbol* sym = symTable_.resolveIdentifier(node.name);
            if (!sym) {
                // If not found in symbol table, dynamically bound or imported
                return makeAnyType();
            }
            return sym->type ? sym->type : makeAnyType();
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
            TypePtr leftType = visitExpr(*node.left);
            TypePtr rightType = visitExpr(*node.right);
            auto result = evaluateBinaryOp(node.op, leftType, rightType, node.line);
            if (!result.valid) {
                throw TypeError(result.errorMessage, node.line, "E_TYPE_406", "Check operand types for operator '" + node.op + "'", 1);
            }
            return result.resultType ? result.resultType : makeAnyType();
        }
        else if constexpr (std::is_same_v<T, AssignExpr>) {
            TypePtr valType = visitExpr(*node.value);
            if (auto* id = std::get_if<IdentExpr>(&node.target->data)) {
                Symbol* sym = symTable_.lookup(id->name);
                if (sym) {
                    if (sym->isConst) {
                        throw TypeError("cannot assign to immutable constant 'let " + id->name + "'",
                                        node.line, "E_TYPE_406", "Declare with 'var' if mutation is required.", 1);
                    }
                    if (sym->type && !sym->type->isAny()) {
                        typeChecker_.checkAssignment(sym->type, valType, node.line);
                    }
                }
            }
            return valType;
        }
        else if constexpr (std::is_same_v<T, CallExpr>) {
            TypePtr calleeType = visitExpr(*node.callee);
            std::vector<TypePtr> argTypes;
            for (const auto& a : node.args) {
                argTypes.push_back(visitExpr(*a));
            }
            if (auto* id = std::get_if<IdentExpr>(&node.callee->data)) {
                Symbol* sym = symTable_.lookup(id->name);
                if (sym) {
                    typeChecker_.checkFunctionCall(*sym, argTypes, node.line);
                    if (sym->type && sym->type->returnType) {
                        return sym->type->returnType;
                    }
                }
            }
            return makeAnyType();
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>) {
            TypePtr opType = visitExpr(*node.operand);
            return evaluateUnaryOp(node.op, opType, node.line);
        }
        else if constexpr (std::is_same_v<T, MemberExpr>) {
            visitExpr(*node.object);
            return makeAnyType();
        }
        else if constexpr (std::is_same_v<T, IndexExpr>) {
            visitExpr(*node.object);
            visitExpr(*node.index);
            return makeAnyType();
        }
        else if constexpr (std::is_same_v<T, PipelineExpr>) {
            visitExpr(*node.left);
            visitExpr(*node.right);
            return makeAnyType();
        }
        else if constexpr (std::is_same_v<T, TernaryExpr>) {
            visitExpr(*node.cond);
            TypePtr thenType = visitExpr(*node.thenExpr);
            TypePtr elseType = visitExpr(*node.elseExpr);
            if (thenType->equals(elseType)) return thenType;
            return makeAnyType();
        }
        else {
            return typeChecker_.inferExpression(expr);
        }
    }, expr.data);
}
