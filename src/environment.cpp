#include "../include/environment.hpp"
#include "../include/error.hpp"

void Environment::define(const std::string& name, ValuePtr val,
                         bool mutable_) {
    for (auto& entry : vars_) {
        if (entry.name == name) {
            entry.var = VarEntry{ std::move(val), mutable_ };
            return;
        }
    }
    vars_.push_back({ name, VarEntry{ std::move(val), mutable_ } });
}

ValuePtr Environment::get(const std::string& name, int line) const {
    for (auto& entry : vars_) {
        if (entry.name == name) return entry.var.value;
    }
    if (parent_) return parent_->get(name, line);
    throw NameError(name, line);
}

void Environment::set(const std::string& name, ValuePtr val, int line) {
    for (auto& entry : vars_) {
        if (entry.name == name) {
            if (!entry.var.mutable_)
                throw RuntimeError("Cannot reassign immutable variable '" + name + "'", line, "E_RUNTIME_403");
            entry.var.value = std::move(val);
            return;
        }
    }
    if (parent_) { parent_->set(name, std::move(val), line); return; }
    throw NameError(name, line);
}

bool Environment::has(const std::string& name) const {
    for (auto& entry : vars_) {
        if (entry.name == name) return true;
    }
    if (parent_) return parent_->has(name);
    return false;
}

bool Environment::hasLocal(const std::string& name) const {
    for (auto& entry : vars_) {
        if (entry.name == name) return true;
    }
    return false;
}

ValuePtr* Environment::getAddr(const std::string& name) {
    for (auto& entry : vars_) {
        if (entry.name == name) return &(entry.var.value);
    }
    if (parent_) return parent_->getAddr(name);
    return nullptr;
}

