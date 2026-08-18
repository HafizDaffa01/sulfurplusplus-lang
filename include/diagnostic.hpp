#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

enum class DiagnosticSeverity {
    NOTE,
    WARNING,
    ERROR,
    FATAL
};

struct Diagnostic {
    DiagnosticSeverity severity;
    std::string code;
    std::string message;
    std::string filename;
    int line;
    int col;
    int length;
    std::string hint;
};

class DiagnosticEngine {
public:
    static DiagnosticEngine& instance() {
        static DiagnosticEngine eng;
        return eng;
    }

    void report(DiagnosticSeverity severity, const std::string& code,
                const std::string& message, const std::string& filename,
                int line = 1, int col = 1, int length = 1,
                const std::string& hint = "") {
        diagnostics_.push_back({severity, code, message, filename, line, col, length, hint});
        if (severity == DiagnosticSeverity::ERROR || severity == DiagnosticSeverity::FATAL) {
            errorCount_++;
        } else if (severity == DiagnosticSeverity::WARNING) {
            warningCount_++;
        }
    }

    void clear() {
        diagnostics_.clear();
        errorCount_ = 0;
        warningCount_ = 0;
    }

    bool hasErrors() const { return errorCount_ > 0; }
    size_t errorCount() const { return errorCount_; }
    size_t warningCount() const { return warningCount_; }
    const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }

    void renderAll(std::ostream& os = std::cerr, bool useColor = true) const {
        for (const auto& d : diagnostics_) {
            render(d, os, useColor);
        }
    }

    static void render(const Diagnostic& d, std::ostream& os = std::cerr, bool useColor = true) {
        std::string red    = useColor ? "\033[31;1m" : "";
        std::string yellow = useColor ? "\033[33;1m" : "";
        std::string blue   = useColor ? "\033[34;1m" : "";
        std::string cyan   = useColor ? "\033[36m"   : "";
        std::string bold   = useColor ? "\033[1m"    : "";
        std::string dim    = useColor ? "\033[2m"    : "";
        std::string reset  = useColor ? "\033[0m"    : "";

        std::string sevStr;
        std::string sevColor;
        switch (d.severity) {
            case DiagnosticSeverity::FATAL:
                sevStr = "fatal error";
                sevColor = red;
                break;
            case DiagnosticSeverity::ERROR:
                sevStr = "error";
                sevColor = red;
                break;
            case DiagnosticSeverity::WARNING:
                sevStr = "warning";
                sevColor = yellow;
                break;
            case DiagnosticSeverity::NOTE:
                sevStr = "note";
                sevColor = blue;
                break;
        }

        // Header: filename:line:col: severity [code]: message
        os << bold << d.filename << ":" << d.line << ":" << d.col << ": "
           << sevColor << sevStr << " [" << d.code << "]: " << reset
           << bold << d.message << reset << "\n";

        // Source snippet
        if (!d.filename.empty() && d.filename != "<repl>" && d.filename != "<input>") {
            try {
                std::ifstream file(d.filename);
                if (file.is_open()) {
                    std::string lineContent;
                    int cur = 1;
                    while (std::getline(file, lineContent)) {
                        if (cur == d.line) {
                            // Gutter: " 12 | "
                            std::string gutter = std::to_string(cur);
                            while (gutter.size() < 4) gutter = " " + gutter;
                            os << cyan << gutter << " | " << reset << lineContent << "\n";

                            // Caret pointer: "      |     ^~~~~"
                            int colPos = std::max(1, d.col);
                            int spanLen = std::max(1, d.length);
                            std::string indent(colPos - 1, ' ');
                            std::string caret = "^" + std::string(spanLen > 1 ? spanLen - 1 : 0, '~');
                            os << cyan << "     | " << reset << sevColor << indent << caret << reset << "\n";
                            break;
                        }
                        cur++;
                    }
                }
            } catch (...) {}
        }

        // Optional Hint
        if (!d.hint.empty()) {
            os << cyan << "     = " << blue << "hint: " << reset << d.hint << "\n";
        }
        os << "\n";
    }

private:
    std::vector<Diagnostic> diagnostics_;
    size_t errorCount_ = 0;
    size_t warningCount_ = 0;
};
