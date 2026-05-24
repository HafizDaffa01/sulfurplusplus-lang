#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>
#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/version.hpp"
#include "../include/interpreter.hpp"
#include "../include/error.hpp"
#include <iomanip>
#include <csignal>
#include <cstdlib>

static std::string formatDuration(double ms) {
    double us = ms * 1000.0;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    if (ms >= 86400000.0) {
        oss << (ms / 86400000.0) << " d (" << us << " us)";
    } else if (ms >= 3600000.0) {
        oss << (ms / 3600000.0) << " h (" << us << " us)";
    } else if (ms >= 60000.0) {
        oss << (ms / 60000.0) << " m (" << us << " us)";
    } else if (ms >= 1000.0) {
        oss << (ms / 1000.0) << " s (" << us << " us)";
    } else if (ms >= 1.0) {
        oss << ms << " ms (" << us << " us)";
    } else {
        oss << us << " us";
    }
    return oss.str();
}

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw IOError("Cannot open file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void printErrorContext(const std::string& filename, int errorLine, const std::string& colorCode = "\033[31;1m") {
    if (errorLine <= 0 || filename == "<repl>") return;
    try {
        std::ifstream f(filename);
        if (!f) return;
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(f, line)) {
            lines.push_back(line);
        }
        
        int startLine = std::max(1, errorLine - 5);
        int endLine = std::min((int)lines.size(), errorLine + 2);

        std::cerr << "\n";
        for (int i = startLine; i <= endLine; i++) {
            if (i == errorLine) {
                std::cerr << "\033[36m" << std::setw(4) << i << " |\033[0m " << colorCode << lines[i - 1] << "\033[0m\n";
                size_t firstNonSpace = lines[i - 1].find_first_not_of(" \t");
                if (firstNonSpace == std::string::npos) firstNonSpace = 0;
                std::cerr << "       " << colorCode << std::string(firstNonSpace, ' ') << "^-- Here\033[0m\n";
            } else {
                std::cerr << "\033[36m" << std::setw(4) << i << " |\033[0m " << lines[i - 1] << "\n";
            }
        }
        std::cerr << "\n";
    } catch (...) {}
}

static void printBanner() {
    std::cerr
        << "+-----------------------------------+\n"
        << "|  combust - Sulfur++ Runtime v1.0  |\n"
        << "+-----------------------------------+\n";
}

static void runREPL(bool debug) {
    printBanner();
    std::cerr << "Type 'exit' or Ctrl+C to quit.\n\n";
    Interpreter interp(debug);
    if (debug) {
        std::cerr << "[DEBUG] Initializing Sulfur++ Debug Mode...\n"
                  << "[DEBUG]   File Path: <repl>\n"
                  << "[DEBUG]   Active Flags: --debug\n"
                  << "[DEBUG]   Sulfur++ Version: " << __SULFUR_VERSION__ << "\n"
                  << "[DEBUG]   Combust Version:  " << __COMBUST_VERSION__ << "\n"
                  << "[DEBUG]   Fuse Version:     " << __FUSE_VERSION__ << "\n"
                  << "[DEBUG] --------------------------------------------------\n";
    }

    // Auto-inject standard functions and constants into global scope in REPL
    interp.injectBuiltinsIntoGlobal();

    std::string line;
    while (true) {
        std::cerr << "\nsfpp> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "exit" || line == "quit") break;
        if (line.empty()) continue;
        try {
            auto start = std::chrono::high_resolution_clock::now();
            Lexer lex(line, "<repl>");
            auto tokens = lex.tokenize();
            Parser par(std::move(tokens));
            auto stmts = par.parse();
            interp.run(stmts, "<repl>");
            if (debug) {
                auto end = std::chrono::high_resolution_clock::now();
                double duration = std::chrono::duration<double, std::milli>(end - start).count();
                std::cerr << "\n[DEBUG] Execution finished in " << formatDuration(duration) << "\n";
            }
        } catch (SulfurError& e) {
            std::string colorCode = "\033[31;1m";
            if (e.code.rfind("FE_", 0) == 0) colorCode = "\033[35;1m";
            else if (e.code.rfind("W_", 0) == 0) colorCode = "\033[33;1m";

            std::cerr << colorCode << "[" << e.code << "]";
            if (e.line > 0) std::cerr << " line " << e.line;
            std::cerr << ": " << e.what() << "\033[0m\n";
            if (!e.hint.empty()) std::cerr << "  \033[33mhint: " << e.hint << "\033[0m\n";
        } catch (std::exception& e) {
            std::cerr << "\033[31;1mError: " << e.what() << "\033[0m\n";
        }
    }
}

static int runFile(const std::string& filename, bool debug, bool watch) {
    auto runOnce = [&]() -> int {
        std::string absPath = filename;
        if (filename != "<repl>" && !filename.empty()) {
            try {
                absPath = std::filesystem::absolute(filename).string();
            } catch (...) {}
        }
        if (debug) {
            std::cerr << "[DEBUG] Initializing Sulfur++ Debug Mode...\n"
                      << "[DEBUG]   File Path: " << absPath << "\n"
                      << "[DEBUG]   Active Flags: --debug" << (watch ? ", --watch" : "") << "\n"
                      << "[DEBUG]   Sulfur++ Version: " << __SULFUR_VERSION__ << "\n"
                      << "[DEBUG]   Combust Version:  " << __COMBUST_VERSION__ << "\n"
                      << "[DEBUG]   Fuse Version:     " << __FUSE_VERSION__ << "\n"
                      << "[DEBUG] --------------------------------------------------\n";
        }
        try {
            auto start = std::chrono::high_resolution_clock::now();
            std::string src = readFile(filename);
            Lexer lex(src, filename);
            auto tokens = lex.tokenize();
            Parser par(std::move(tokens));
            auto stmts = par.parse();
            Interpreter interp(debug);
            interp.run(stmts, absPath);
            if (debug) {
                auto end = std::chrono::high_resolution_clock::now();
                double duration = std::chrono::duration<double, std::milli>(end - start).count();
                std::cerr << "\n[DEBUG] Execution finished in " << formatDuration(duration) << "\n";
            }
            return 0;
        } catch (SulfurError& e) {
            std::string colorCode = "\033[31;1m";
            if (e.code.rfind("FE_", 0) == 0) colorCode = "\033[35;1m";
            else if (e.code.rfind("W_", 0) == 0) colorCode = "\033[33;1m";

            std::string loc = e.line > 0 ? " line " + std::to_string(e.line) : "";
            std::cerr << colorCode << "[" << e.code << "]" << loc << ": " << e.what() << "\033[0m\n";
            printErrorContext(filename, e.line, colorCode);
            if (!e.hint.empty()) std::cerr << "  \033[33mhint: " << e.hint << "\033[0m\n";
            return 1;
        } catch (std::exception& e) {
            std::cerr << "\033[31;1mError: " << e.what() << "\033[0m\n";
            return 1;
        }
    };

    if (!watch) return runOnce();

    // Watch mode
    std::cerr << "[combust] Watching " << filename << " ...\n";
    auto lastWrite = std::filesystem::last_write_time(filename);
    runOnce();
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        try {
            auto cur = std::filesystem::last_write_time(filename);
            if (cur != lastWrite) {
                lastWrite = cur;
                std::cerr << "\n[combust] File changed — reloading...\n";
                runOnce();
            }
        } catch (...) {}
    }
    return 0;
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, [](int sig) {
        std::cout << "\n[E_KEYBOARD_INT!] Whoops! Keyboard interrupt called!\n" << std::flush;
        std::_Exit(sig);
    });

    bool debug = false;
    bool watch = false;
    std::string filename;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--debug" || arg == "-d") debug = true;
        else if (arg == "--watch" || arg == "-w") watch = true;
        else if (arg == "--version" || arg == "-v") {
            std::cout << "combust " << __COMBUST_VERSION__ << " (Sulfur++ Runtime)\n";
            return 0;
        }
        else if (arg == "--help" || arg == "-h") {
            std::cout <<
                "combust — Sulfur++ Runtime\n\n"
                "Usage:\n"
                "  combust <file.sfpp>          Run a Sulfur++ program\n"
                "  combust <file.sfpp> --debug  Run with debug trace\n"
                "  combust <file.sfpp> --watch  Run and reload on file change\n"
                "  combust                      Start interactive REPL\n"
                "  combust --version            Show version\n\n"
                "Options:\n"
                "  -d, --debug    Enable debug/trace mode\n"
                "  -w, --watch    Watch mode (auto-reload)\n"
                "  -v, --version  Show version\n"
                "  -h, --help     Show this help\n";
            return 0;
        }
        else if (arg[0] != '-') {
            filename = arg;
        }
    }

    if (filename.empty()) {
        runREPL(debug);
        return 0;
    }

    return runFile(filename, debug, watch);
}
