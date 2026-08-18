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

    std::filesystem::path tempDir = std::filesystem::temp_directory_path();
    std::string tmpCpp = (tempDir / ("sulfur_compile_" + stem + ".cpp")).string();
#ifdef _WIN32
    if (out.size() < 4 || out.substr(out.size() - 4) != ".exe") {
        out += ".exe";
    }
    std::string tmpBin = (tempDir / ("sulfur_compile_" + stem + ".exe")).string();
#else
    std::string tmpBin = (tempDir / ("sulfur_compile_" + stem + "_bin")).string();
#endif

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
            << "#include \"diagnostic.hpp\"\n"
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
            << "        Diagnostic d{\n"
            << "            (e.code.rfind(\"FE_\", 0) == 0) ? DiagnosticSeverity::FATAL :\n"
            << "            (e.code.rfind(\"W_\", 0) == 0) ? DiagnosticSeverity::WARNING :\n"
            << "            DiagnosticSeverity::ERROR,\n"
            << "            e.code, e.what(), \"" << stem << ".sfpp\", e.line, e.col, 1, e.hint\n"
            << "        };\n"
            << "        DiagnosticEngine::render(d, std::cerr, true);\n"
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
        auto parent = std::filesystem::path(out).parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }
        std::filesystem::copy_file(tmpBin, out, std::filesystem::copy_options::overwrite_existing);
#ifndef _WIN32
        std::filesystem::permissions(out,
            std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec,
            std::filesystem::perm_options::add);
#endif
    } catch (const std::exception& e) {
        std::cerr << "\033[31;1mError: Failed to copy binary: " << e.what() << "\033[0m\n";
        std::filesystem::remove(tmpBin);
        return 1;
    }

    std::filesystem::remove(tmpBin);

#ifdef _WIN32
    std::string dllSrc = sulfurLibPath + "/libsulfur.dll";
    std::string outDir = std::filesystem::path(out).parent_path().string();
    if (outDir.empty()) outDir = ".";
    std::string dllDst = outDir + "/libsulfur.dll";
    if (std::filesystem::exists(dllSrc) && dllSrc != dllDst) {
        try {
            std::filesystem::copy_file(dllSrc, dllDst, std::filesystem::copy_options::overwrite_existing);
        } catch (...) {}
    }
#endif

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
