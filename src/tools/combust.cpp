#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>
#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/resolver.hpp"
#include "../include/semantic/analyzer.hpp"
#include "../include/version.hpp"
#include "../include/interpreter.hpp"
#include "../include/error.hpp"
#include "../include/compiler.hpp"
#include "../include/diagnostic.hpp"
#include <iomanip>
#include <csignal>

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
        << "|  combust - Sulfur++ Runtime  |\n"
        << "+-----------------------------------+\n";
}

static void runREPL(bool debug, bool jit) {
    printBanner();
    std::cerr << "Type 'exit' or Ctrl+C to quit.\n\n";
    Interpreter interp(debug, jit);
    if (debug) {
        std::cerr << "[DEBUG] Initializing Sulfur++ Debug Mode...\n"
                  << "[DEBUG]   File Path: <repl>\n"
                  << "[DEBUG]   Active Flags: --debug\n"
                  << "[DEBUG]   Sulfur++ Version: " << __SULFUR_VERSION__ << "\n"
                  << "[DEBUG]   Combust Version:  " << __COMBUST_VERSION__ << "\n"
                  << "[DEBUG] --------------------------------------------------\n";
    }

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

static int runFile(const std::string& filename, bool debug, bool watch, bool jit) {
    auto runOnce = [&](const std::string& filename, bool debug, bool watch, bool jit) -> int {
        std::string absPath = filename;
        if (filename != "<repl>" && !filename.empty()) {
            try {
                absPath = std::filesystem::absolute(filename).string();
            } catch (...) {}
        }
        if (debug) {
            std::cerr << "[DEBUG] Initializing Sulfur++ Debug Mode...\n"
                      << "[DEBUG]   File Path: " << absPath << "\n"
                      << "[DEBUG]   Active Flags: --debug" << (watch ? ", --watch" : "") << (jit ? ", --jit" : "") << "\n"
                      << "[DEBUG]   Sulfur++ Version: " << __SULFUR_VERSION__ << "\n"
                      << "[DEBUG]   Combust Version:  " << __COMBUST_VERSION__ << "\n"
                      << "[DEBUG] --------------------------------------------------\n";
        }
        try {
            auto start = std::chrono::high_resolution_clock::now();
            std::string src = readFile(filename);
            Lexer lex(src, filename);
            auto tokens = lex.tokenize();
            Parser par(std::move(tokens));
            auto stmts = par.parse();

            SemanticAnalyzer sem;
            sem.analyze(stmts, filename);

            if (debug) {
                std::cerr << "\n[DEBUG] Symbol Table & Scope Hierarchy:\n" << sem.dumpSymbols() << "\n";
            }

            Interpreter interp(debug, jit);
            interp.injectBuiltinsIntoGlobal();
            interp.run(stmts, absPath);
            if (debug) {
                auto end = std::chrono::high_resolution_clock::now();
                double duration = std::chrono::duration<double, std::milli>(end - start).count();
                std::cerr << "\n[DEBUG] Execution finished in " << formatDuration(duration) << "\n";
            }
            return 0;
        } catch (SulfurError& e) {
            Diagnostic d{
                (e.code.rfind("FE_", 0) == 0) ? DiagnosticSeverity::FATAL :
                (e.code.rfind("W_", 0) == 0) ? DiagnosticSeverity::WARNING :
                DiagnosticSeverity::ERROR,
                e.code,
                e.what(),
                filename,
                e.line,
                e.col,
                1,
                e.hint
            };
            DiagnosticEngine::render(d, std::cerr, true);
            return 1;
        } catch (std::exception& e) {
            std::cerr << "\033[31;1mError: " << e.what() << "\033[0m\n";
            return 1;
        }
    };

    if (!watch) return runOnce(filename, debug, watch, jit);

    std::cerr << "[combust] Watching " << filename << " ...\n";
    auto lastWrite = std::filesystem::last_write_time(filename);
    runOnce(filename, debug, watch, jit);
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        try {
            auto cur = std::filesystem::last_write_time(filename);
            if (cur != lastWrite) {
                lastWrite = cur;
                std::cerr << "\n[combust] File changed - reloading...\n";
                runOnce(filename, debug, watch, jit);
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
    bool compile = false;
    bool jit = false;
    std::string filename;
    std::string outputPath;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--debug" || arg == "-d") debug = true;
        else if (arg == "--watch" || arg == "-w") watch = true;
        else if (arg == "--compile" || arg == "-c") compile = true;
        else if (arg == "--jit" || arg == "-j") jit = true;
        else if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        }
        else if (arg == "--version" || arg == "-v") {
            std::cout << "combust " << __COMBUST_VERSION__ << " (Sulfur++ Runtime)\n";
            return 0;
        }
        else if (arg == "--help" || arg == "-h") {
            std::cout <<
                "combust - The Sulfur++ Execution Engine\n\n"
                "USAGE:\n"
                "    combust [OPTIONS] [FILE]\n\n"
                "DESCRIPTION:\n"
                "    Executes Sulfur++ scripts (.sfpp). If no file is provided, an interactive\n"
                "    REPL session is started.\n\n"
                "OPTIONS:\n"
                "    -d, --debug      Enable verbose debug tracing and memory tracking\n"
                "    -w, --watch      Start the runtime in watch mode (auto-reloads on save)\n"
                "    -c, --compile    Compile to a standalone native executable\n"
                "    -o <output>      Specify output file path (used with -c)\n"
                "    -j, --jit        Force Just-In-Time (JIT) compilation for all functions\n"
                "    -v, --version    Print version information\n"
                "    -h, --help       Print this help message\n";
            return 0;
        }
        else if (arg[0] != '-') {
            filename = arg;
        }
    }

    if (filename.empty() && !compile) {
        runREPL(debug, jit);
        return 0;
    }

    if (compile) {
        if (filename.empty()) {
            std::cerr << "\033[31;1mError: --compile requires a source file.\033[0m\n";
            return 1;
        }
        return compileFile(filename, outputPath);
    }

    return runFile(filename, debug, watch, jit);
}
