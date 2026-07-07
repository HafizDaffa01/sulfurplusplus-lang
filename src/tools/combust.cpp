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

#ifdef ENABLE_LLVM
#include "../llvm/llvm_aot.hpp"
#endif

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

static int runFile(const std::string& filename, bool debug, bool watch, bool aot, bool jit) {
    auto runOnce = [&](const std::string& filename, bool debug, bool watch, bool aot, bool jit) -> int {
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

            if (aot) {
#ifdef ENABLE_LLVM
                std::string objPath = filename.substr(0, filename.find_last_of('.')) + ".o";
                std::cerr << "[combust] AOT Compiling to native object: " << objPath << " ...\n";
                sulfur_emit_object(stmts, "AOTModule", objPath);
                std::cerr << "[combust] AOT Compilation complete.\n";
                return 0;
#else
                std::cerr << "\033[31;1mError: combust was built without LLVM support. AOT compilation is not available.\033[0m\n";
                return 1;
#endif
            }

            Interpreter interp(debug, jit);
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

    if (!watch) return runOnce(filename, debug, watch, aot, jit);

    // Watch mode
    std::cerr << "[combust] Watching " << filename << " ...\n";
    auto lastWrite = std::filesystem::last_write_time(filename);
    runOnce(filename, debug, watch, aot, jit);
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        try {
            auto cur = std::filesystem::last_write_time(filename);
            if (cur != lastWrite) {
                lastWrite = cur;
                std::cerr << "\n[combust] File changed - reloading...\n";
                runOnce(filename, debug, watch, aot, jit);
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
    bool aot = false;
    bool jit = false;
    std::string filename;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--debug" || arg == "-d") debug = true;
        else if (arg == "--watch" || arg == "-w") watch = true;
        else if (arg == "--aot" || arg == "-c") aot = true;
        else if (arg == "--jit" || arg == "-j") jit = true;
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
                "    -c, --aot        Ahead-of-Time (AOT) compile to a native object file (.o)\n"
                "    -j, --jit        Force Just-In-Time (JIT) compilation for all functions\n"
                "    -v, --version    Print version information\n"
                "    -h, --help       Print this help message\n";
            return 0;
        }
        else if (arg[0] != '-') {
            filename = arg;
        }
    }

    if (filename.empty()) {
        runREPL(debug, jit);
        return 0;
    }

    return runFile(filename, debug, watch, aot, jit);
}
