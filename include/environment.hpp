#pragma once
#include <string>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <functional>
#include "value.hpp"

struct WatchEntry {
    std::string varName;
    void* condExpr;   // Expr* (optional condition, nullptr if plain watch)
    void* body;       // Stmt*
    std::shared_ptr<Environment> scope;
};

struct SignalEntry {
    std::string name;
    std::vector<std::pair<void*, std::shared_ptr<Environment>>> handlers; // Stmt*, scope
};

struct VarEntry {
    ValuePtr value;
    bool mutable_;
    bool reactive;
};

class Environment : public std::enable_shared_from_this<Environment> {
public:
    explicit Environment(std::shared_ptr<Environment> parent = nullptr)
        : parent_(std::move(parent)) {}

    void define(const std::string& name, ValuePtr val,
                bool mutable_ = true, bool reactive = false);
    ValuePtr get(const std::string& name, int line = -1) const;
    void set(const std::string& name, ValuePtr val, int line = -1);

    bool has(const std::string& name) const;
    bool hasLocal(const std::string& name) const;

    // Reactive watches for this environment
    void addWatch(WatchEntry entry);
    std::vector<WatchEntry>& watches() { return watches_; }

    // Signal registry (global-ish, propagated up)
    void declareSignal(const std::string& name);
    void addSignalHandler(const std::string& name, void* body,
                          std::shared_ptr<Environment> scope);
    SignalEntry* findSignal(const std::string& name);

    std::shared_ptr<Environment> parent() const { return parent_; }

    std::map<std::string, VarEntry>& vars() { return vars_; }

private:
    std::shared_ptr<Environment> parent_;
    std::map<std::string, VarEntry> vars_;
    std::map<std::string, SignalEntry> signals_;
    std::vector<WatchEntry> watches_;
};
