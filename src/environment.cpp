#include "../include/environment.hpp"
#include "../include/error.hpp"

void Environment::define(const std::string& name, ValuePtr val,
                         bool mutable_, bool reactive) {
    vars_[name] = VarEntry{ std::move(val), mutable_, reactive };
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
            throw RuntimeError("Cannot reassign immutable variable '" + name + "'", line);
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

void Environment::addWatch(WatchEntry entry) {
    watches_.push_back(std::move(entry));
}

void Environment::declareSignal(const std::string& name) {
    if (!signals_.count(name))
        signals_[name] = SignalEntry{ name, {} };
}

void Environment::addSignalHandler(const std::string& name, void* body,
                                   std::shared_ptr<Environment> scope) {
    // Find signal - search up chain too
    auto it = signals_.find(name);
    if (it != signals_.end()) {
        it->second.handlers.push_back({ body, std::move(scope) });
        return;
    }
    if (parent_) {
        parent_->addSignalHandler(name, body, std::move(scope));
        return;
    }
    throw RuntimeError("Signal '" + name + "' is not declared");
}

SignalEntry* Environment::findSignal(const std::string& name) {
    auto it = signals_.find(name);
    if (it != signals_.end()) return &it->second;
    if (parent_) return parent_->findSignal(name);
    return nullptr;
}
