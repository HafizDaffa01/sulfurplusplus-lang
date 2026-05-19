#include "../include/environment.hpp"
#include "../include/error.hpp"

void Environment::define(const std::string& name, ValuePtr val,
                         bool mutable_) {
    vars_[name] = VarEntry{ std::move(val), mutable_ };
}

ValuePtr Environment::get(const std::string& name, int line) const {
    auto it = vars_.find(name);
    if (it != vars_.end()) return it->second.value;
    if (parent_) return parent_->get(name, line);
    throw NameError(name, line);
}

void Environment::set(const std::string& name, ValuePtr val, int line) {
    auto it = vars_.find(name);
    if (it != vars_.end()) {
        if (!it->second.mutable_)
            throw RuntimeError("Cannot reassign immutable variable '" + name + "'", line, "E_RUNTIME_403");
        it->second.value = std::move(val);
        return;
    }
    if (parent_) { parent_->set(name, std::move(val), line); return; }
    throw NameError(name, line);
}

bool Environment::has(const std::string& name) const {
    if (vars_.count(name)) return true;
    if (parent_) return parent_->has(name);
    return false;
}

bool Environment::hasLocal(const std::string& name) const {
    return vars_.count(name) > 0;
}

ValuePtr* Environment::getAddr(const std::string& name) {
    auto it = vars_.find(name);
    if (it != vars_.end()) return &(it->second.value);
    if (parent_) return parent_->getAddr(name);
    return nullptr;
}

