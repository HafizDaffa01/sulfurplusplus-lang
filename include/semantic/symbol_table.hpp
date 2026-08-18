#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <sstream>
#include "semantic/type.hpp"

enum class ScopeKind {
    Global,
    Function,
    Block,
    Class,
    Loop,
    Module
};

struct Symbol {
    std::string name;
    TypePtr type = nullptr;
    bool isConst = false;
    bool isFunction = false;
    bool isClass = false;
    bool isStruct = false;
    bool isModule = false;
    bool isInitialized = false;
    bool isCaptured = false;  // Closed over by an inner function
    bool isShadowing = false; // Shadows an outer symbol
    int scopeDepth = 0;
    int line = 0;
    int col = 0;

    Symbol() = default;
    Symbol(std::string n, TypePtr t, bool c = false, int l = 0, int col_num = 0)
        : name(std::move(n)), type(std::move(t)), isConst(c), line(l), col(col_num) {}
};

class Scope : public std::enable_shared_from_this<Scope> {
public:
    std::string name;
    ScopeKind kind;
    int depth = 0;
    std::shared_ptr<Scope> parent;
    std::vector<std::shared_ptr<Scope>> children;
    std::unordered_map<std::string, Symbol> symbols;
    std::vector<std::string> symbolOrder;
    TypePtr functionReturnType = nullptr;

    explicit Scope(std::string name, ScopeKind k, std::shared_ptr<Scope> p = nullptr, int d = 0)
        : name(std::move(name)), kind(k), depth(d), parent(std::move(p)) {}

    bool define(const Symbol& sym);
    Symbol* lookupLocal(const std::string& name);
    Symbol* lookup(const std::string& name);
    Symbol* lookupWithHops(const std::string& name, int& outHops);

    std::string dump(const std::string& indent = "", bool isLast = true) const;
};

class SymbolTable {
public:
    SymbolTable();

    std::shared_ptr<Scope> enterScope(const std::string& name, ScopeKind kind, TypePtr fnRetType = nullptr);
    void exitScope();

    bool define(Symbol sym);
    Symbol* lookup(const std::string& name);
    Symbol* lookupCurrent(const std::string& name);
    Symbol* resolveIdentifier(const std::string& name);

    bool isInLoop() const;
    bool isInFunction() const;
    TypePtr currentFunctionReturnType() const;

    std::shared_ptr<Scope> currentScope() const { return currentScope_; }
    std::shared_ptr<Scope> globalScope() const { return globalScope_; }

    std::string dumpTree() const;

private:
    std::shared_ptr<Scope> currentScope_;
    std::shared_ptr<Scope> globalScope_;
};
