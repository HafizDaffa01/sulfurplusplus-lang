#pragma once

#include <iostream>
#include <string>
#include "error.hpp"

namespace sulfur {

// ANSI color codes for terminal output
inline const char* RED   = "\033[31m";
inline const char* YELLOW= "\033[33m";
inline const char* GREEN = "\033[32m";
inline const char* RESET = "\033[0m";

/**
 * Pretty‑print a SulfurError (or derived) to stderr with colour and optional hint.
 * Output format: [SEVERITY] CODE: message (line N)
 *   hint (if any) in yellow
 *   optional source snippet with caret at column.
 */
inline void printError(const SulfurError& err, const std::string& sourceLine = "", int column = -1) {
    std::string severity;
    if (err.code.rfind("FE_", 0) == 0) severity = "FATAL";
    else if (err.code.rfind("E_", 0) == 0) severity = "ERROR";
    else if (err.code.rfind("W_", 0) == 0) severity = "WARN";
    else severity = "INFO";

    const char* color = (severity == "ERROR" || severity == "FATAL") ? RED : YELLOW;
    std::cerr << color << "[" << severity << "] " << err.code << ": " << err.what();
    if (err.line >= 0) std::cerr << " (line " << err.line << ")";
    std::cerr << RESET << "\n";
    if (!err.hint.empty()) {
        std::cerr << YELLOW << "  hint: " << err.hint << RESET << "\n";
    }
    if (!sourceLine.empty() && column >= 0) {
        std::cerr << "    " << sourceLine << "\n";
        std::cerr << "    " << std::string(column, ' ') << "^" << "\n";
    }
}

} // namespace sulfur
