#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <iostream>

// Simple macro/alias system for Sulfur++
// Usage: Alias::create("print(%1)", "Terminal.Out << \"%1\" << Terminal.EOL");
// Then calls like print("Hello") are expanded at runtime.

class Alias {
public:
    // Register a new alias pattern. %1, %2, ... act as placeholders for arguments.
    static void create(const std::string& pattern, const std::string& expansion) {
        map_[pattern] = expansion;
    }

    // Check if an alias exists for a given function name.
    static bool has(const std::string& name) {
        for (const auto& kv : map_) {
            const std::string& pat = kv.first;
            size_t pos = pat.find('(');
            if (pos != std::string::npos && pat.substr(0, pos) == name) {
                return true;
            }
        }
        return false;
    }

    // Invoke an alias with a list of argument strings.
    // Returns expanded source code or empty string if no matching alias.
    static std::string invoke(const std::string& name, const std::vector<std::string>& args) {
        for (const auto& kv : map_) {
            const std::string& pat = kv.first;
            size_t pos = pat.find('(');
            if (pos == std::string::npos) continue;
            if (pat.substr(0, pos) != name) continue;
            int placeholderCount = 0;
            for (size_t i = pos; i + 1 < pat.size(); ++i) {
                if (pat[i] == '%' && std::isdigit(pat[i + 1])) ++placeholderCount;
            }
            if ((int)args.size() != placeholderCount) continue; // arity mismatch
            std::string result = kv.second;
            for (int i = 1; i <= placeholderCount; ++i) {
                std::string ph = "%" + std::to_string(i);
                size_t p = result.find(ph);
                while (p != std::string::npos) {
                    result.replace(p, ph.size(), args[i - 1]);
                    p = result.find(ph, p + args[i - 1].size());
                }
            }
            return result;
        }
        return "";
    }

    // Expand a call string according to the registered pattern.
    // Returns the expanded source code (e.g., "Terminal.Out << \"Hello\" << Terminal.EOL").
    static std::string expand(const std::string& call) {
        // Find '(' to separate name and args
        auto pos = call.find('(');
        if (pos == std::string::npos) return call; // not a function-like call
        std::string name = call.substr(0, pos);
        std::string argsPart = call.substr(pos + 1, call.rfind(')') - pos - 1);
        // Try each registered pattern
        for (const auto& kv : map_) {
            const std::string& pattern = kv.first;
            // pattern must start with the same name
            if (pattern.compare(0, name.size(), name) != 0) continue;
            // Count placeholders %N in pattern
            int expected = 0;
            for (size_t i = 0; i + 1 < pattern.size(); ++i) {
                if (pattern[i] == '%' && std::isdigit(pattern[i + 1])) ++expected;
            }
            std::vector<std::string> args = splitArgs(argsPart);
            if ((int)args.size() != expected) continue; // arity mismatch
            // Substitute placeholders
            std::string result = kv.second;
            for (int i = 1; i <= expected; ++i) {
                std::string ph = "%" + std::to_string(i);
                size_t p = result.find(ph);
                while (p != std::string::npos) {
                    result.replace(p, ph.size(), args[i - 1]);
                    p = result.find(ph, p + args[i - 1].size());
                }
            }
            return result;
        }
        return call; // no alias matched
    }

private:
    static std::unordered_map<std::string, std::string> map_;

    // Simple comma splitter (doesn't handle nested structures).
    static std::vector<std::string> splitArgs(const std::string& s) {
        std::vector<std::string> res; std::string cur; int depth = 0;
        for (char c : s) {
            if (c == '(') ++depth; if (c == ')') --depth;
            if (c == ',' && depth == 0) { res.push_back(trim(cur)); cur.clear(); }
            else cur.push_back(c);
        }
        if (!cur.empty()) res.push_back(trim(cur));
        return res;
    }
    static std::string trim(const std::string& str) {
        const char* ws = " \t\n\r";
        size_t start = str.find_first_not_of(ws);
        size_t end = str.find_last_not_of(ws);
        if (start == std::string::npos) return "";
        return str.substr(start, end - start + 1);
    }
};

inline std::unordered_map<std::string, std::string> Alias::map_;
