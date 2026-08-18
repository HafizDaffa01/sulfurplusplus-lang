#include "semantic/symbol_table.hpp"
#include <iostream>

bool Scope::define(const Symbol& sym) {
    if (symbols.find(sym.name) != symbols.end()) {
        return false; // Already defined in this scope
    }
    symbols[sym.name] = sym;
    symbolOrder.push_back(sym.name);
    return true;
}

Symbol* Scope::lookupLocal(const std::string& name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) return &it->second;
    return nullptr;
}

Symbol* Scope::lookup(const std::string& name) {
    for (Scope* s = this; s != nullptr; s = s->parent.get()) {
        auto it = s->symbols.find(name);
        if (it != s->symbols.end()) return &it->second;
    }
    return nullptr;
}

Symbol* Scope::lookupWithHops(const std::string& name, int& outHops) {
    outHops = 0;
    for (Scope* s = this; s != nullptr; s = s->parent.get()) {
        auto it = s->symbols.find(name);
        if (it != s->symbols.end()) return &it->second;
        outHops++;
    }
    outHops = -1;
    return nullptr;
}

std::string Scope::dump(const std::string& indent, bool isLast) const {
    std::ostringstream ss;
    ss << indent;
    if (parent != nullptr) {
        ss << (isLast ? "└── " : "├── ");
    }
    ss << name << " Scope\n";

    std::string childIndent = indent + (parent == nullptr ? "" : (isLast ? "    " : "│   "));

    // Print symbols
    size_t totalItems = symbolOrder.size() + children.size();
    size_t currentIndex = 0;

    for (const auto& symName : symbolOrder) {
        currentIndex++;
        bool lastItem = (currentIndex == totalItems);
        auto it = symbols.find(symName);
        if (it == symbols.end()) continue;
        const auto& sym = it->second;

        ss << childIndent << (lastItem ? "└── " : "├── ") << sym.name << " : ";
        if (sym.type) {
            ss << sym.type->toString();
        } else {
            ss << "?";
        }
        if (sym.isConst) ss << " [const]";
        if (sym.isCaptured) ss << " [captured]";
        if (sym.isShadowing) ss << " [shadows outer]";
        ss << "\n";
    }

    // Print child scopes
    for (size_t i = 0; i < children.size(); i++) {
        currentIndex++;
        bool lastChild = (i == children.size() - 1);
        ss << children[i]->dump(childIndent, lastChild);
    }

    return ss.str();
}

SymbolTable::SymbolTable() {
    globalScope_ = std::make_shared<Scope>("Global", ScopeKind::Global, nullptr, 0);
    currentScope_ = globalScope_;

    // Inject standard global builtins/keywords
    globalScope_->define(Symbol("print", makeFunctionType({makeAnyType()}, makeVoidType())));
    globalScope_->define(Symbol("println", makeFunctionType({makeAnyType()}, makeVoidType())));
    globalScope_->define(Symbol("assert", makeFunctionType({makeBoolType(), makeStringType()}, makeVoidType())));
}

std::shared_ptr<Scope> SymbolTable::enterScope(const std::string& name, ScopeKind kind, TypePtr fnRetType) {
    int nextDepth = currentScope_ ? currentScope_->depth + 1 : 0;
    auto newScope = std::make_shared<Scope>(name, kind, currentScope_, nextDepth);
    newScope->functionReturnType = fnRetType ? fnRetType : (currentScope_ ? currentScope_->functionReturnType : nullptr);
    if (currentScope_) {
        currentScope_->children.push_back(newScope);
    }
    currentScope_ = newScope;
    return newScope;
}

void SymbolTable::exitScope() {
    if (currentScope_ && currentScope_->parent) {
        currentScope_ = currentScope_->parent;
    }
}

bool SymbolTable::define(Symbol sym) {
    if (!currentScope_) return false;
    sym.scopeDepth = currentScope_->depth;

    // Check if this symbol shadows an outer symbol
    if (currentScope_->parent) {
        Symbol* outer = currentScope_->parent->lookup(sym.name);
        if (outer) {
            sym.isShadowing = true;
        }
    }

    return currentScope_->define(sym);
}

Symbol* SymbolTable::lookup(const std::string& name) {
    if (!currentScope_) return nullptr;
    return currentScope_->lookup(name);
}

Symbol* SymbolTable::lookupCurrent(const std::string& name) {
    if (!currentScope_) return nullptr;
    return currentScope_->lookupLocal(name);
}

Symbol* SymbolTable::resolveIdentifier(const std::string& name) {
    if (!currentScope_) return nullptr;
    int hops = 0;
    Symbol* sym = currentScope_->lookupWithHops(name, hops);
    if (sym && hops > 0) {
        // If accessed across a function boundary, mark as captured closure upvalue
        bool crossedFunctionBoundary = false;
        Scope* s = currentScope_.get();
        for (int i = 0; i < hops && s != nullptr; i++, s = s->parent.get()) {
            if (s->kind == ScopeKind::Function) {
                crossedFunctionBoundary = true;
                break;
            }
        }
        if (crossedFunctionBoundary && sym->scopeDepth > 0) {
            sym->isCaptured = true;
        }
    }
    return sym;
}

bool SymbolTable::isInLoop() const {
    for (Scope* s = currentScope_.get(); s != nullptr; s = s->parent.get()) {
        if (s->kind == ScopeKind::Loop) return true;
    }
    return false;
}

bool SymbolTable::isInFunction() const {
    for (Scope* s = currentScope_.get(); s != nullptr; s = s->parent.get()) {
        if (s->kind == ScopeKind::Function) return true;
    }
    return false;
}

TypePtr SymbolTable::currentFunctionReturnType() const {
    for (Scope* s = currentScope_.get(); s != nullptr; s = s->parent.get()) {
        if (s->kind == ScopeKind::Function && s->functionReturnType) {
            return s->functionReturnType;
        }
    }
    return makeAnyType();
}

std::string SymbolTable::dumpTree() const {
    if (!globalScope_) return "";
    return globalScope_->dump();
}
