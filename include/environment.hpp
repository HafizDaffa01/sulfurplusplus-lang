#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "value.hpp"

struct VarEntry {
    ValuePtr value;
    bool mutable_;
};

class Environment : public std::enable_shared_from_this<Environment> {
public:
    explicit Environment(std::shared_ptr<Environment> parent = nullptr)
        : parent_(std::move(parent)) {
    }

    void define(const std::string& name, ValuePtr val,
                bool mutable_ = true);
    ValuePtr get(const std::string& name, int line = -1) const;
    void set(const std::string& name, ValuePtr val, int line = -1);
    ValuePtr* getAddr(const std::string& name);

    bool has(const std::string& name) const;
    bool hasLocal(const std::string& name) const;

    std::shared_ptr<Environment> parent() const { return parent_; }

    std::unordered_map<std::string, VarEntry>& vars() { return vars_; }
    const std::unordered_map<std::string, VarEntry>& vars() const { return vars_; }

private:
    std::shared_ptr<Environment> parent_;
    std::unordered_map<std::string, VarEntry> vars_;
};
