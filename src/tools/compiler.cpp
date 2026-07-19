#include "compiler.hpp"
#include "error.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw IOError("Cannot open file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string escapeForCpp(const std::string& src) {
    std::string result;
    result.reserve(src.size());
    for (char c : src) {
        switch (c) {
            case '\\': result += "\\\\"; break;
            case '"':  result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:   result += c; break;
        }
    }
    return result;
}

int compileFile(const std::string& filename, const std::string& outputPath) {
    std::string src;
    try {
        src = readFile(filename);
    } catch (const std::exception& e) {
        std::cerr << "\033[31;1mError: " << e.what() << "\033[0m\n";
        return 1;
    }

    std::string stem = std::filesystem::path(filename).stem().string();
    std::string out = outputPath.empty() ? stem : outputPath;

    std::string tmpCpp = "/tmp/sulfur_compile_" + stem + ".cpp";
    std::string tmpBin = "/tmp/sulfur_compile_" + stem + "_bin";

    std::string includePath = std::filesystem::absolute("include").string();
    std::string sulfurLibPath = std::filesystem::absolute("build").string();

    std::cerr << "[compiler] Generating C++ source...\n";
    {
        std::ofstream ofs(tmpCpp);
        if (!ofs) {
            std::cerr << "\033[31;1mError: Cannot create temporary file: " << tmpCpp << "\033[0m\n";
            return 1;
        }
        ofs << "#include \"interpreter.hpp\"\n"
            << "#include \"lexer.hpp\"\n"
            << "#include \"parser.hpp\"\n"
            << "#include \"error.hpp\"\n"
            << "#include <iostream>\n"
            << "#include <string>\n\n"
            << "static const char* sfppSource = \"" << escapeForCpp(src) << "\";\n\n"
            << "int main() {\n"
            << "    try {\n"
            << "        Lexer lex(sfppSource, \"" << stem << ".sfpp\");\n"
            << "        auto tokens = lex.tokenize();\n"
            << "        Parser par(std::move(tokens));\n"
            << "        auto stmts = par.parse();\n"
            << "        Interpreter interp(false, false);\n"
            << "        interp.injectBuiltinsIntoGlobal();\n"
            << "        interp.run(stmts, \"" << stem << ".sfpp\");\n"
            << "    } catch (SulfurError& e) {\n"
            << "        std::cerr << \"\\033[31;1m[\" << e.code << \"] \";\n"
            << "        if (e.line > 0) std::cerr << \"line \" << e.line << \": \";\n"
            << "        std::cerr << e.what() << \"\\033[0m\\n\";\n"
            << "        return 1;\n"
            << "    } catch (std::exception& e) {\n"
            << "        std::cerr << \"\\033[31;1mError: \" << e.what() << \"\\033[0m\\n\";\n"
            << "        return 1;\n"
            << "    }\n"
            << "    return 0;\n"
            << "}\n";
    }

    std::cerr << "[compiler] Compiling to native binary...\n";
    std::string compileCmd = "g++ -std=c++17 -O2"
        " -I \"" + includePath + "\""
        " -o \"" + tmpBin + "\""
        " \"" + tmpCpp + "\""
        " -L \"" + sulfurLibPath + "\""
        " -lsulfur -lpthread 2>&1";

    int ret = std::system(compileCmd.c_str());
    std::filesystem::remove(tmpCpp);
    if (ret != 0) {
        std::cerr << "\033[31;1mError: Compilation failed.\033[0m\n";
        return 1;
    }

    try {
        std::filesystem::copy_file(tmpBin, out, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::permissions(out,
            std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec,
            std::filesystem::perm_options::add);
    } catch (const std::exception& e) {
        std::cerr << "\033[31;1mError: Failed to copy binary: " << e.what() << "\033[0m\n";
        std::filesystem::remove(tmpBin);
        return 1;
    }

    std::filesystem::remove(tmpBin);

    std::string stdSrc = "src/stdlib";
    std::string stdDst = std::filesystem::path(out).parent_path().string();
    if (stdDst.empty()) stdDst = ".";
    stdDst += "/std";

    if (std::filesystem::exists(stdSrc)) {
        try {
            std::filesystem::copy(stdSrc, stdDst,
                std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
        } catch (...) {}
    }

    std::cerr << "[compiler] Done: " << out << "\n";
    return 0;
}
