#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <memory>
#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/resolver.hpp"
#include "../include/interpreter.hpp"
#include "../include/version.hpp"

class SulfurDebugger {
public:
    SulfurDebugger(const std::string& filename) : filename_(filename) {}

    int start() {
        std::ifstream file(filename_);
        if (!file.is_open()) {
            std::cerr << "[Debugger] Could not open file: " << filename_ << "\n";
            return 1;
        }

        std::string line;
        while (std::getline(file, line)) {
            sourceLines_.push_back(line);
        }
        file.close();

        std::cout << "===================================================\n"
                  << "  Sulfur++ Interactive Debugger v" << __SULFUR_VERSION__ << "\n"
                  << "  Target: " << filename_ << " (" << sourceLines_.size() << " lines)\n"
                  << "  Type 'help' for commands, 'run' or 'c' to start\n"
                  << "===================================================\n";

        // Read source into memory
        std::string source;
        for (const auto& l : sourceLines_) source += l + "\n";

        try {
            Lexer lex(source, filename_);
            auto toks = lex.tokenize();
            Parser parser(std::move(toks));
            ast_ = parser.parse();
            Resolver resolver;
            resolver.resolve(ast_);
        } catch (const std::exception& e) {
            std::cerr << "[Debugger Error] " << e.what() << "\n";
            return 1;
        }

        repl();
        return 0;
    }

private:
    std::string filename_;
    std::vector<std::string> sourceLines_;
    std::vector<StmtPtr> ast_;
    std::set<int> breakpoints_;
    int currentLine_ = 1;

    void repl() {
        std::string cmd;
        while (true) {
            std::cout << "sfpp-dbg> " << std::flush;
            if (!std::getline(std::cin, cmd)) break;
            if (cmd.empty()) continue;

            std::stringstream ss(cmd);
            std::string action;
            ss >> action;

            if (action == "help" || action == "h") {
                printHelp();
            } else if (action == "quit" || action == "q") {
                break;
            } else if (action == "break" || action == "b") {
                int lineNum;
                if (ss >> lineNum) {
                    breakpoints_.insert(lineNum);
                    std::cout << "Breakpoint set at line " << lineNum << "\n";
                } else {
                    std::cout << "Usage: break <line_number>\n";
                }
            } else if (action == "clear") {
                int lineNum;
                if (ss >> lineNum) {
                    breakpoints_.erase(lineNum);
                    std::cout << "Breakpoint removed from line " << lineNum << "\n";
                } else {
                    breakpoints_.clear();
                    std::cout << "All breakpoints cleared.\n";
                }
            } else if (action == "list" || action == "l") {
                int start = std::max(1, currentLine_ - 5);
                int end = std::min((int)sourceLines_.size(), currentLine_ + 5);
                for (int i = start; i <= end; ++i) {
                    std::string bpMarker = breakpoints_.count(i) ? " * " : "   ";
                    std::string curMarker = (i == currentLine_) ? "=> " : "   ";
                    std::cout << curMarker << bpMarker << i << "\t| " << sourceLines_[i - 1] << "\n";
                }
            } else if (action == "run" || action == "r" || action == "continue" || action == "c") {
                std::cout << "[Debugger] Executing program...\n";
                try {
                    Interpreter interp;
                    interp.run(ast_, filename_);
                    std::cout << "[Debugger] Program finished normally.\n";
                } catch (const std::exception& e) {
                    std::cerr << "[Debugger Exception] " << e.what() << "\n";
                }
            } else if (action == "info" && ss.rdbuf()->in_avail()) {
                std::string sub;
                ss >> sub;
                if (sub == "break" || sub == "b") {
                    std::cout << "Breakpoints:\n";
                    for (int bp : breakpoints_) {
                        std::cout << "  - Line " << bp << ": " << sourceLines_[bp - 1] << "\n";
                    }
                }
            } else {
                std::cout << "Unknown debugger command: '" << action << "'. Type 'help'.\n";
            }
        }
    }

    void printHelp() {
        std::cout << "Debugger Commands:\n"
                  << "  break, b <line>   Set breakpoint at line\n"
                  << "  clear [line]      Clear breakpoint(s)\n"
                  << "  list, l           Display source around current line\n"
                  << "  run, r            Run program from beginning\n"
                  << "  continue, c       Continue execution\n"
                  << "  info break        List active breakpoints\n"
                  << "  quit, q           Exit debugger\n";
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: sulfur-debug <file.sfpp>\n";
        return 1;
    }
    SulfurDebugger dbg(argv[1]);
    return dbg.start();
}
