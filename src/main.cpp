#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>
#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/interpreter.hpp"
#include "../include/error.hpp"

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void printBanner() {
    std::cerr
        << "\033[33m"
        << "╔═══════════════════════════════════╗\n"
        << "║  combust — Sulfur++ Runtime v1.0  ║\n"
        << "╚═══════════════════════════════════╝\n"
        << "\033[0m";
}

static void runREPL(bool debug) {
    printBanner();
    std::cerr << "Type 'exit' or Ctrl+D to quit.\n\n";
    Interpreter interp(debug);
    std::string line;
    while (true) {
        std::cerr << "\033[36msfpp>\033[0m ";
        if (!std::getline(std::cin, line)) break;
        if (line == "exit" || line == "quit") break;
        if (line.empty()) continue;
        try {
            Lexer lex(line, "<repl>");
            auto tokens = lex.tokenize();
            Parser par(std::move(tokens));
            auto stmts = par.parse();
            interp.run(stmts);
        } catch (SulfurError& e) {
            std::cerr << "\033[31m[" << e.code << "]";
            if (e.line > 0) std::cerr << " line " << e.line;
            std::cerr << ": " << e.what() << "\033[0m\n";
        } catch (std::exception& e) {
            std::cerr << "\033[31mError: " << e.what() << "\033[0m\n";
        }
    }
}

static int runFile(const std::string& filename, bool debug, bool watch) {
    auto runOnce = [&]() -> int {
        try {
            std::string src = readFile(filename);
            Lexer lex(src, filename);
            auto tokens = lex.tokenize();
            Parser par(std::move(tokens));
            auto stmts = par.parse();
            Interpreter interp(debug);
            interp.run(stmts);
            return 0;
        } catch (SulfurError& e) {
            std::string loc = e.line > 0 ? " line " + std::to_string(e.line) : "";
            std::cerr << "\033[31m[" << e.code << "]" << loc << ": " << e.what() << "\033[0m\n";
            return 1;
        } catch (std::exception& e) {
            std::cerr << "\033[31mError: " << e.what() << "\033[0m\n";
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
    bool debug = false;
    bool watch = false;
    std::string filename;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--debug" || arg == "-d") debug = true;
        else if (arg == "--watch" || arg == "-w") watch = true;
        else if (arg == "--version" || arg == "-v") {
            std::cout << "combust 1.0.0 (Sulfur++ Runtime)\n";
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
