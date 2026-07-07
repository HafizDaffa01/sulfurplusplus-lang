#include "diagnostic.hpp"
#include "../include/environment.hpp"
#include "../include/error.hpp"
#include <algorithm>
#include <vector>
#include <climits>

// -------------------------------------------------------------------------
// Levenshtein distance – simple O(n*m) implementation
// -------------------------------------------------------------------------
static int levenshtein(const std::string& a, const std::string& b) {
    const size_t n = a.size(), m = b.size();
    std::vector<int> cur(m + 1), prev(m + 1);
    for (size_t j = 0; j <= m; ++j) prev[j] = j;
    for (size_t i = 1; i <= n; ++i) {
        cur[0] = i;
        for (size_t j = 1; j <= m; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            cur[j] = std::min({ prev[j] + 1,          // deletion
                               cur[j - 1] + 1,      // insertion
                               prev[j - 1] + cost });// substitution
        }
        std::swap(prev, cur);
    }
    return prev[m];
}

// -------------------------------------------------------------------------
// Find a close variable name in the current or parent scopes
// -------------------------------------------------------------------------
static std::string suggestName(const Environment* env, const std::string& unknown) {
    const int MAX_DIST = 2;
    std::string best;
    int bestDist = INT_MAX;
    // current scope
    for (const auto& kv : env->vars()) {
        int d = levenshtein(unknown, kv.first);
        if (d < bestDist) { bestDist = d; best = kv.first; }
    }
    // parent scopes
    if (env->parent()) {
        std::string parentSug = suggestName(env->parent().get(), unknown);
        if (!parentSug.empty()) {
            int d = levenshtein(unknown, parentSug);
            if (d < bestDist) { bestDist = d; best = parentSug; }
        }
    }
    return (bestDist <= MAX_DIST) ? best : std::string{};
}

void Environment::define(const std::string& name, ValuePtr val,
                         bool mutable_) {
    vars_[name] = VarEntry{ std::move(val), mutable_ };
}

ValuePtr Environment::get(const std::string& name, int line) const {
    auto it = vars_.find(name);
    if (it != vars_.end()) return it->second.value;
    if (parent_) return parent_->get(name, line);
    // hint if possible
    std::string hint = suggestName(this, name);
    if (!hint.empty())
        throw NameError(name, line, hint);
    else
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
    // hint if possible
    std::string hint = suggestName(this, name);
    if (!hint.empty())
        throw NameError(name, line, hint);
    else
        throw NameError(name, line);
}

bool Environment::has(const std::string& name) const {
    if (vars_.find(name) != vars_.end()) return true;
    if (parent_) return parent_->has(name);
    return false;
}

bool Environment::hasLocal(const std::string& name) const {
    return vars_.find(name) != vars_.end();
}

ValuePtr* Environment::getAddr(const std::string& name) {
    auto it = vars_.find(name);
    if (it != vars_.end()) return &(it->second.value);
    if (parent_) return parent_->getAddr(name);
    return nullptr;
}
