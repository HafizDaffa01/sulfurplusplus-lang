#include "../include/resolver.hpp"
#include <iostream>

Resolver::Resolver() {
    // Populate standard global identifiers and builtins
    globals_ = {
        "len", "typeOf", "toStr", "toInt", "toFloat", "toComplex", "toBool",
        "print", "println", "eprint", "eprintln", "sleep", "time", "clock",
        "math_sqrt", "math_sin", "math_cos", "math_tan", "math_abs", "math_floor",
        "math_ceil", "math_log", "math_exp", "math_pow", "math_complex",
        "http_get", "http_post", "http_put", "http_delete", "http_request",
        "crypto_sha256", "crypto_base64_encode", "crypto_base64_decode",
        "crypto_hex_encode", "crypto_hex_decode",
        "regex_match", "regex_search", "regex_replace", "regex_findall",
        "path_join", "path_basename", "path_dirname", "path_ext", "path_exists", "path_isabs",
        "Terminal", "TIO", "io_readfile", "io_writefile", "io_appendfile",
        "io_fileexists", "io_deletefile", "io_readlines", "io_writelines",
        "io_cwd", "io_listdir", "io_isdir", "io_isfile", "io_mkdir",
        "sys_exec", "sys_platform", "sys_arch", "sys_env", "sys_setenv",
        "json_parse", "json_stringify", "json_pretty"
    };
}

void Resolver::beginScope() {
    scopes_.push_back({});
}

void Resolver::endScope() {
    scopes_.pop_back();
}

void Resolver::declare(const std::string& name, bool isMutable, int line, const std::string& typeName) {
    if (scopes_.empty()) {
        globals_.insert(name);
        return;
    }
    auto& scope = scopes_.back();
    scope[name] = SymbolInfo{isMutable, true, line, typeName.empty() ? "any" : typeName};
}

void Resolver::define(const std::string& name) {
    if (scopes_.empty()) {
        globals_.insert(name);
        return;
    }
    if (scopes_.back().find(name) != scopes_.back().end()) {
        scopes_.back()[name].isDefined = true;
    }
}

SymbolInfo* Resolver::lookupSymbol(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto sit = it->find(name);
        if (sit != it->end()) {
            return &sit->second;
        }
    }
    return nullptr;
}

bool Resolver::isTypeCompatible(const std::string& expected, const std::string& actual) {
    if (expected.empty() || expected == "any" || actual.empty() || actual == "any") return true;
    if (expected == actual) return true;
    if (expected == "float_64" && (actual == "int_64" || actual == "float_32")) return true;
    if (expected == "complex_128" && (actual == "int_64" || actual == "float_64")) return true;
    if (expected == "int_64" && (actual == "int_32" || actual == "int_16" || actual == "int_8" || actual == "int")) return true;
    return false;
}

std::string Resolver::inferExprType(const Expr& e) {
    return std::visit([&](const auto& node) -> std::string {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IntLitExpr>) return "int_64";
        else if constexpr (std::is_same_v<T, FloatLitExpr>) return "float_64";
        else if constexpr (std::is_same_v<T, BoolLitExpr>) return "bool";
        else if constexpr (std::is_same_v<T, NullLitExpr>) return "null";
        else if constexpr (std::is_same_v<T, StringLitExpr> || std::is_same_v<T, PSStringExpr>) return "str";
        else if constexpr (std::is_same_v<T, CharLitExpr>) return "char";
        else if constexpr (std::is_same_v<T, ListLitExpr>) return "list";
        else if constexpr (std::is_same_v<T, DictLitExpr>) return "dict";
        else if constexpr (std::is_same_v<T, IdentExpr>) {
            auto* sym = lookupSymbol(node.name);
            return sym ? sym->typeName : "any";
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
            std::string lt = node.left ? inferExprType(*node.left) : "any";
            std::string rt = node.right ? inferExprType(*node.right) : "any";
            std::string op = node.op;

            if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") return "bool";
            if (op == "&&" || op == "||") return "bool";
            if (op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>") return "int_64";
            if (op == "+" && (lt == "str" || rt == "str")) return "str";
            if (lt == "complex_128" || rt == "complex_128") return "complex_128";
            if (lt == "float_64" || rt == "float_64") return "float_64";
            if (lt == "int_64" && rt == "int_64") return "int_64";
            return "any";
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>) {
            if (node.op == "!") return "bool";
            if (node.op == "~") return "int_64";
            return node.operand ? inferExprType(*node.operand) : "any";
        }
        else if constexpr (std::is_same_v<T, CallExpr>) {
            if (node.callee && std::holds_alternative<IdentExpr>(node.callee->data)) {
                std::string fnName = std::get<IdentExpr>(node.callee->data).name;
                if (functionSignatures_.count(fnName)) {
                    std::string rType = functionSignatures_[fnName].returnType;
                    return rType.empty() ? "any" : rType;
                }
            }
            return "any";
        }
        else if constexpr (std::is_same_v<T, TernaryExpr>) {
            return node.thenExpr ? inferExprType(*node.thenExpr) : "any";
        }
        return "any";
    }, e.data);
}

void Resolver::resolve(const std::vector<StmtPtr>& stmts) {
    for (const auto& s : stmts) {
        if (s) resolveStmt(*s);
    }
}

void Resolver::resolveStmt(const Stmt& s) {
    std::visit([&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, VarDeclStmt>)
            resolveVarDecl(node);
        else if constexpr (std::is_same_v<T, FnDeclStmt>)
            resolveFnDecl(node);
        else if constexpr (std::is_same_v<T, ClassDeclStmt>)
            resolveClassDecl(node);
        else if constexpr (std::is_same_v<T, StructDeclStmt>)
            resolveStructDecl(node);
        else if constexpr (std::is_same_v<T, InterfaceDeclStmt>)
            resolveInterfaceDecl(node);
        else if constexpr (std::is_same_v<T, BlockStmt>)
            resolveBlock(node);
        else if constexpr (std::is_same_v<T, IfStmt>)
            resolveIf(node);
        else if constexpr (std::is_same_v<T, WhileStmt>)
            resolveWhile(node);
        else if constexpr (std::is_same_v<T, ForStmt>)
            resolveFor(node);
        else if constexpr (std::is_same_v<T, ForCStyleStmt>)
            resolveForCStyle(node);
        else if constexpr (std::is_same_v<T, ReturnStmt>)
            resolveReturn(node);
        else if constexpr (std::is_same_v<T, BreakStmt>) {
            if (loopDepth_ <= 0) {
                throw ParseError("Cannot use 'break' outside of a loop", node.line, "E_SEMANTIC_400");
            }
        }
        else if constexpr (std::is_same_v<T, ContinueStmt>) {
            if (loopDepth_ <= 0) {
                throw ParseError("Cannot use 'continue' outside of a loop", node.line, "E_SEMANTIC_400");
            }
        }
        else if constexpr (std::is_same_v<T, ExprStmt>) {
            if (node.expr) resolveExpr(*node.expr);
        }
        else if constexpr (std::is_same_v<T, StreamOutStmt>)
            resolveStreamOut(node);
        else if constexpr (std::is_same_v<T, ThrowStmt>)
            resolveThrow(node);
        else if constexpr (std::is_same_v<T, TryCatchStmt>)
            resolveTryCatch(node);
        else if constexpr (std::is_same_v<T, MatchStmt>)
            resolveMatch(node);
        else if constexpr (std::is_same_v<T, UnsafeStmt>)
            resolveUnsafe(node);
        else if constexpr (std::is_same_v<T, DeferStmt>)
            resolveDefer(node);
        else if constexpr (std::is_same_v<T, ImportStmt>) {
            if (!node.alias.empty()) {
                declare(node.alias, false, node.line);
            }
        }
        else if constexpr (std::is_same_v<T, ExposeStmt>) {
            declare(node.alias, false, node.line);
        }
    }, s.data);
}

void Resolver::resolveBlock(const BlockStmt& b) {
    beginScope();
    for (const auto& s : b.stmts) {
        if (s) resolveStmt(*s);
    }
    endScope();
}

void Resolver::resolveVarDecl(const VarDeclStmt& s) {
    bool isMutable = (s.keyword == "var" || s.keyword == "auto");
    std::string inferredType = "any";
    if (s.initializer) {
        resolveExpr(*s.initializer);
        inferredType = inferExprType(*s.initializer);
    }
    if (!s.type.empty() && s.initializer) {
        if (!isTypeCompatible(s.type, inferredType)) {
            throw ParseError("Type mismatch: cannot initialize variable '" + s.name + "' of type '" + s.type + "' with expression of type '" + inferredType + "'", s.line, "E_TYPE_406");
        }
    }
    declare(s.name, isMutable, s.line, s.type.empty() ? inferredType : s.type);
    define(s.name);
}

void Resolver::resolveFnDecl(const FnDeclStmt& s) {
    declare(s.name, false, s.line, s.retType);
    define(s.name);
    functionSignatures_[s.name] = FunctionSignature{s.params, s.retType, s.line};

    FunctionContext enclosingFunction = currentFunction_;
    std::string enclosingReturnType = currentReturnType_;
    currentFunction_ = s.isMethod ? FunctionContext::METHOD : FunctionContext::FUNCTION;
    currentReturnType_ = s.retType;

    beginScope();
    for (const auto& p : s.params) {
        declare(p.first, true, s.line, p.second);
        define(p.first);
    }
    if (s.body) {
        if (std::holds_alternative<BlockStmt>(s.body->data)) {
            const auto& b = std::get<BlockStmt>(s.body->data);
            for (const auto& stmt : b.stmts) {
                if (stmt) resolveStmt(*stmt);
            }
        } else {
            resolveStmt(*s.body);
        }
    }
    endScope();

    currentFunction_ = enclosingFunction;
    currentReturnType_ = enclosingReturnType;
}

void Resolver::resolveClassDecl(const ClassDeclStmt& s) {
    declare(s.name, false, s.line);
    define(s.name);

    ClassContext enclosingClass = currentClass_;
    currentClass_ = ClassContext::CLASS;

    beginScope();
    declare("this", false, s.line);
    define("this");

    for (const auto& member : s.members) {
        if (member) resolveStmt(*member);
    }

    endScope();
    currentClass_ = enclosingClass;
}

void Resolver::resolveStructDecl(const StructDeclStmt& s) {
    declare(s.name, false, s.line);
    define(s.name);
}

void Resolver::resolveInterfaceDecl(const InterfaceDeclStmt& s) {
    declare(s.name, false, s.line);
    define(s.name);
}

void Resolver::resolveIf(const IfStmt& s) {
    if (s.cond) resolveExpr(*s.cond);
    if (s.thenBranch) resolveStmt(*s.thenBranch);
    if (s.elseBranch) resolveStmt(*s.elseBranch);
}

void Resolver::resolveWhile(const WhileStmt& s) {
    if (s.cond) resolveExpr(*s.cond);
    loopDepth_++;
    if (s.body) resolveStmt(*s.body);
    loopDepth_--;
}

void Resolver::resolveFor(const ForStmt& s) {
    if (s.iterable) resolveExpr(*s.iterable);
    beginScope();
    declare(s.var, true, s.line);
    define(s.var);
    loopDepth_++;
    if (s.body) resolveStmt(*s.body);
    loopDepth_--;
    endScope();
}

void Resolver::resolveForCStyle(const ForCStyleStmt& s) {
    beginScope();
    if (s.init) resolveStmt(*s.init);
    if (s.cond) resolveExpr(*s.cond);
    if (s.post) resolveExpr(*s.post);
    loopDepth_++;
    if (s.body) resolveStmt(*s.body);
    loopDepth_--;
    endScope();
}

void Resolver::resolveReturn(const ReturnStmt& s) {
    if (currentFunction_ == FunctionContext::NONE) {
        throw ParseError("Cannot use 'return' outside of a function", s.line, "E_SEMANTIC_400");
    }
    if (s.value) {
        resolveExpr(*s.value);
        if (!currentReturnType_.empty() && currentReturnType_ != "void" && currentReturnType_ != "any") {
            std::string retValType = inferExprType(*s.value);
            if (!isTypeCompatible(currentReturnType_, retValType)) {
                throw ParseError("Return type mismatch: function expected '" + currentReturnType_ + "', got '" + retValType + "'", s.line, "E_TYPE_406");
            }
        }
    }
}

void Resolver::resolveThrow(const ThrowStmt& s) {
    if (s.value) resolveExpr(*s.value);
}

void Resolver::resolveStreamOut(const StreamOutStmt& s) {
    if (s.target) resolveExpr(*s.target);
    if (s.value) resolveExpr(*s.value);
}

void Resolver::resolveTryCatch(const TryCatchStmt& s) {
    if (s.tryBody) resolveStmt(*s.tryBody);
    if (s.catchBody) {
        beginScope();
        declare(s.catchVar, true, s.line);
        define(s.catchVar);
        resolveStmt(*s.catchBody);
        endScope();
    }
    if (s.finallyBody) resolveStmt(*s.finallyBody);
}

void Resolver::resolveMatch(const MatchStmt& s) {
    if (s.value) resolveExpr(*s.value);
    for (const auto& c : s.cases) {
        if (c.pattern) resolveExpr(*c.pattern);
        if (c.body) resolveStmt(*c.body);
    }
}

void Resolver::resolveUnsafe(const UnsafeStmt& s) {
    if (s.body) resolveStmt(*s.body);
}

void Resolver::resolveDefer(const DeferStmt& s) {
    if (s.body) resolveStmt(*s.body);
}

void Resolver::resolveExpr(const Expr& e) {
    std::visit([&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IdentExpr>)
            resolveIdent(node);
        else if constexpr (std::is_same_v<T, AssignExpr>)
            resolveAssign(node);
        else if constexpr (std::is_same_v<T, BinaryExpr>)
            resolveBinary(node);
        else if constexpr (std::is_same_v<T, UnaryExpr>)
            resolveUnary(node);
        else if constexpr (std::is_same_v<T, CallExpr>)
            resolveCall(node);
        else if constexpr (std::is_same_v<T, IndexExpr>)
            resolveIndex(node);
        else if constexpr (std::is_same_v<T, MemberExpr>)
            resolveMember(node);
        else if constexpr (std::is_same_v<T, PipelineExpr>)
            resolvePipeline(node);
        else if constexpr (std::is_same_v<T, NullCoalExpr>)
            resolveNullCoal(node);
        else if constexpr (std::is_same_v<T, ListLitExpr>)
            resolveListLit(node);
        else if constexpr (std::is_same_v<T, DictLitExpr>)
            resolveDictLit(node);
        else if constexpr (std::is_same_v<T, LambdaExpr>)
            resolveLambda(node);
        else if constexpr (std::is_same_v<T, NewExpr>)
            resolveNew(node);
        else if constexpr (std::is_same_v<T, TernaryExpr>)
            resolveTernary(node);
        else if constexpr (std::is_same_v<T, AddrOfExpr>) {
            if (node.operand) resolveExpr(*node.operand);
        }
        else if constexpr (std::is_same_v<T, DerefExpr>) {
            if (node.operand) resolveExpr(*node.operand);
        }
        else if constexpr (std::is_same_v<T, DeleteExpr>) {
            if (node.operand) resolveExpr(*node.operand);
        }
    }, e.data);
}

void Resolver::resolveIdent(const IdentExpr& e) {
    if (e.name == "this") {
        if (currentClass_ == ClassContext::NONE) {
            throw ParseError("Cannot use 'this' outside of a class method", e.line, "E_SEMANTIC_400");
        }
    }
}

void Resolver::resolveAssign(const AssignExpr& e) {
    if (e.value) resolveExpr(*e.value);
    if (e.target) {
        if (std::holds_alternative<IdentExpr>(e.target->data)) {
            const auto& id = std::get<IdentExpr>(e.target->data);
            if (id.name == "this") {
                throw ParseError("Cannot reassign 'this'", e.line, "E_SEMANTIC_400");
            }
            auto* sym = lookupSymbol(id.name);
            if (sym) {
                if (!sym->isMutable) {
                    throw ParseError("Cannot reassign to immutable variable '" + id.name + "' declared with 'let'", e.line, "E_SEMANTIC_400");
                }
                if (sym->typeName != "any" && !sym->typeName.empty() && e.value) {
                    std::string valType = inferExprType(*e.value);
                    if (!isTypeCompatible(sym->typeName, valType)) {
                        throw ParseError("Type mismatch in assignment: cannot assign '" + valType + "' to variable '" + id.name + "' of type '" + sym->typeName + "'", e.line, "E_TYPE_406");
                    }
                }
            }
        } else {
            resolveExpr(*e.target);
        }
    }
}

void Resolver::resolveBinary(const BinaryExpr& e) {
    if (e.left) resolveExpr(*e.left);
    if (e.right) resolveExpr(*e.right);
}

void Resolver::resolveUnary(const UnaryExpr& e) {
    if (e.operand) resolveExpr(*e.operand);
}

void Resolver::resolveCall(const CallExpr& e) {
    if (e.callee) resolveExpr(*e.callee);
    for (const auto& a : e.args) {
        if (a) resolveExpr(*a);
    }
    if (e.callee && std::holds_alternative<IdentExpr>(e.callee->data)) {
        std::string fnName = std::get<IdentExpr>(e.callee->data).name;
        if (functionSignatures_.count(fnName)) {
            const auto& sig = functionSignatures_[fnName];
            if (!sig.params.empty() && e.args.size() == sig.params.size()) {
                for (size_t i = 0; i < e.args.size(); ++i) {
                    const auto& expectedType = sig.params[i].second;
                    if (!expectedType.empty() && expectedType != "any" && e.args[i]) {
                        std::string argType = inferExprType(*e.args[i]);
                        if (!isTypeCompatible(expectedType, argType)) {
                            throw ParseError("Argument type mismatch for parameter '" + sig.params[i].first + "': expected '" + expectedType + "', got '" + argType + "'", e.line, "E_TYPE_406");
                        }
                    }
                }
            }
        }
    }
}

void Resolver::resolveIndex(const IndexExpr& e) {
    if (e.object) resolveExpr(*e.object);
    if (e.index) resolveExpr(*e.index);
}

void Resolver::resolveMember(const MemberExpr& e) {
    if (e.object) resolveExpr(*e.object);
}

void Resolver::resolvePipeline(const PipelineExpr& e) {
    if (e.left) resolveExpr(*e.left);
    if (e.right) resolveExpr(*e.right);
}

void Resolver::resolveNullCoal(const NullCoalExpr& e) {
    if (e.left) resolveExpr(*e.left);
    if (e.right) resolveExpr(*e.right);
}

void Resolver::resolveListLit(const ListLitExpr& e) {
    for (const auto& el : e.elements) {
        if (el) resolveExpr(*el);
    }
}

void Resolver::resolveDictLit(const DictLitExpr& e) {
    for (const auto& p : e.pairs) {
        if (p.first) resolveExpr(*p.first);
        if (p.second) resolveExpr(*p.second);
    }
}

void Resolver::resolveLambda(const LambdaExpr& e) {
    FunctionContext enclosingFunction = currentFunction_;
    currentFunction_ = FunctionContext::LAMBDA;

    beginScope();
    for (const auto& p : e.params) {
        declare(p.first, true, e.line);
        define(p.first);
    }
    if (e.body) resolveStmt(*e.body);
    endScope();

    currentFunction_ = enclosingFunction;
}

void Resolver::resolveNew(const NewExpr& e) {
    for (const auto& a : e.args) {
        if (a) resolveExpr(*a);
    }
}

void Resolver::resolveTernary(const TernaryExpr& e) {
    if (e.cond) resolveExpr(*e.cond);
    if (e.thenExpr) resolveExpr(*e.thenExpr);
    if (e.elseExpr) resolveExpr(*e.elseExpr);
}
