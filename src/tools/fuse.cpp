#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include "../include/version.hpp"
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <csignal>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

// ─── Helpers ──────────────────────────────────────────────────────────────────

static bool isWhitespace(const std::string& s) {
    for (char c : s)
        if (!std::isspace(static_cast<unsigned char>(c))) return false;
    return true;
}

static bool getInput(std::string& output) {
    if (!std::getline(std::cin, output)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return false;
    }
    return true;
}

static void printBanner() {
    std::cout
        << "+-----------------------------------+\n"
        << "|  fuse - Sulfur++ Package Manager  |\n"
        << "+-----------------------------------+\n";
}

static void printHelp() {
    std::cout <<
        "fuse -- Sulfur++ Package Manager\n\n"
        "Usage:\n"
        "  fuse init                        Initialize a new Sulfur++ project\n"
        "  fuse add <user>/<package>        Install a package from GitHub\n"
        "  fuse add local <path>            Install a local package\n"
        "  fuse rem <user>/<package>        Remove an installed package\n"
        "  fuse update                      Update all installed packages\n"
        "  fuse update <user>/<package>     Update a specific package\n"
        "  fuse list                        List installed packages\n"
        "  fuse run <script>                Run a script from fuse.json\n"
        "  fuse publish                     Create a publishable package template\n"
        "  fuse --help                      Show this help message\n"
        "  fuse --version                   Show version\n\n"
        "Packages are stored in the packages/ directory.\n"
        "Import syntax: import user/package;\n";
}

// ─── JSON helpers (minimal, no deps) ─────────────────────────────────────────

// Read raw fuse.json into string
static std::string readFuseJson() {
    std::ifstream f("fuse.json");
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Write string to fuse.json
static void writeFuseJson(const std::string& content) {
    std::ofstream f("fuse.json");
    f << content;
}

// Get value of a top-level string key from JSON
static std::string jsonGetStr(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos);
    if (pos == std::string::npos) return "";
    size_t end = json.find('"', pos + 1);
    if (end == std::string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

// Get all entries in "dependencies": { ... } as vector of "user/repo" strings
static std::vector<std::string> jsonGetDeps(const std::string& json) {
    std::vector<std::string> deps;
    size_t depIdx = json.find("\"dependencies\"");
    if (depIdx == std::string::npos) return deps;
    size_t open = json.find('{', depIdx);
    size_t close = json.find('}', open);
    if (open == std::string::npos || close == std::string::npos) return deps;
    std::string block = json.substr(open + 1, close - open - 1);
    size_t p = 0;
    while ((p = block.find('"', p)) != std::string::npos) {
        size_t e = block.find('"', p + 1);
        if (e == std::string::npos) break;
        std::string key = block.substr(p + 1, e - p - 1);
        if (!key.empty()) deps.push_back(key);
        p = block.find(',', e);
        if (p == std::string::npos) break;
        p++;
    }
    return deps;
}

// Add a dependency entry to fuse.json content string
static std::string jsonAddDep(const std::string& content, const std::string& pkg) {
    size_t depIdx = content.find("\"dependencies\"");
    if (depIdx == std::string::npos) return content;
    size_t open = content.find('{', depIdx);
    size_t close = content.find('}', open);
    if (open == std::string::npos || close == std::string::npos) return content;
    std::string body = content.substr(open + 1, close - open - 1);
    std::string newDep = "\"" + pkg + "\": \"latest\"";
    if (content.find("\"" + pkg + "\"") != std::string::npos) return content; // already present
    bool empty = true;
    for (char c : body) if (!std::isspace(static_cast<unsigned char>(c))) { empty = false; break; }
    std::string newBody = empty
        ? "\n    " + newDep + "\n  "
        : body.substr(0, body.find_last_not_of(" \t\r\n") + 1) + ",\n    " + newDep + "\n  ";
    return content.substr(0, open + 1) + newBody + content.substr(close);
}

// Remove a dependency entry from fuse.json content string
static std::string jsonRemDep(const std::string& content, const std::string& pkg) {
    size_t depIdx = content.find("\"dependencies\"");
    if (depIdx == std::string::npos) return content;
    size_t open = content.find('{', depIdx);
    size_t close = content.find('}', open);
    if (open == std::string::npos || close == std::string::npos) return content;
    std::string body = content.substr(open + 1, close - open - 1);
    size_t pkgPos = body.find("\"" + pkg + "\"");
    if (pkgPos == std::string::npos) return content;
    size_t lineStart = body.rfind('\n', pkgPos);
    if (lineStart == std::string::npos) lineStart = 0;
    size_t lineEnd = body.find('\n', pkgPos);
    if (lineEnd == std::string::npos) lineEnd = body.size();
    std::string newBody = body.substr(0, lineStart) + body.substr(lineEnd);
    // trim trailing comma
    size_t lc = newBody.rfind(',');
    if (lc != std::string::npos) {
        bool onlySpace = true;
        for (size_t i = lc + 1; i < newBody.size(); i++)
            if (!std::isspace(static_cast<unsigned char>(newBody[i]))) { onlySpace = false; break; }
        if (onlySpace) newBody.erase(lc, 1);
    }
    return content.substr(0, open + 1) + newBody + content.substr(close);
}

// ─── packages/ path helpers ────────────────────────────────────────────────

// packages/user/repo.sfpp
static fs::path packagePath(const std::string& user, const std::string& repo) {
    return fs::path("packages") / user / (repo + ".sfpp");
}

// Download url to dest via PowerShell Invoke-WebRequest
static bool downloadFile(const std::string& url, const fs::path& dest) {
    std::string cmd = "powershell -Command \"Invoke-WebRequest -Uri '" + url +
                      "' -OutFile '" + dest.string() + "' -ErrorAction SilentlyContinue\"";
    int r = std::system(cmd.c_str());
    return r == 0 && fs::exists(dest) && fs::file_size(dest) > 0;
}

// Write a stub/mock package file
static void writeMockPackage(const fs::path& dest, const std::string& user, const std::string& repo) {
    std::ofstream f(dest);
    f << "// Mock package @" << user << "/" << repo << "\n"
      << "export this as @" << user << "/" << repo << ";\n\n"
      << "import std/builtin --use=[NOLIBNAME];\n\n"
      << "fn hello() {\n"
      << "    Terminal.Out << \"Hello from @" << user << "/" << repo << "!\\n\";\n"
      << "}\n";
}

// ─── Command handlers ──────────────────────────────────────────────────────────

static void handleInit() {
    printBanner();
    std::cout << "\nInitializing new Sulfur++ project...\n\n";

    std::string projName, useGitInput;
    std::string defaultName = fs::current_path().filename().string();
    if (defaultName.empty()) defaultName = "sulfur-project";

    std::cout << "Enter project name (default: " << defaultName << "): ";
    if (!getInput(projName)) return;
    if (isWhitespace(projName)) projName = defaultName;
    for (char& c : projName) if (c == ' ') c = '-';

    std::cout << "Initialize git? (y/n) [y]: ";
    if (!getInput(useGitInput)) return;
    bool useGit = isWhitespace(useGitInput) ||
        std::tolower(static_cast<unsigned char>(useGitInput[0])) != 'n';

    std::cout << "\nCreating project...\n";
    try {
        fs::create_directories(projName);
        fs::current_path(projName);
        std::cout << "  [CREATED] " << projName << "/\n";
    } catch (const std::exception& e) {
        std::cerr << "  [ERROR] " << e.what() << "\n";
        return;
    }

    // fuse.json
    {
        std::ofstream f("fuse.json");
        f << "{\n"
          << "  \"name\": \"" << projName << "\",\n"
          << "  \"version\": \"1.0.0\",\n"
          << "  \"description\": \"A Sulfur++ project\",\n"
          << "  \"main\": \"main.sfpp\",\n"
          << "  \"scripts\": {\n"
          << "    \"start\": \"combust main.sfpp\"\n"
          << "  },\n"
          << "  \"dependencies\": {}\n"
          << "}\n";
        std::cout << "  [CREATED] fuse.json\n";
    }

    // main.sfpp
    if (!fs::exists("main.sfpp")) {
        std::ofstream f("main.sfpp");
        f << "// " << projName << " - entry point\n\n"
          << "import std/builtin --use=[NOLIBNAME];\n\n"
          << "Terminal.Out << \"Hello from " << projName << "!\\n\";\n";
        std::cout << "  [CREATED] main.sfpp\n";
    }

    // packages/
    fs::create_directories("packages");
    std::cout << "  [CREATED] packages/\n";

    // git
    if (useGit) {
        std::cout << "\nInitializing git...\n";
        if (std::system("git init") == 0) {
            if (!fs::exists(".gitignore")) {
                std::ofstream g(".gitignore");
                g << "build/\nbin/\n*.exe\npackages/\n";
                std::cout << "  [CREATED] .gitignore\n";
            }
        } else {
            std::cout << "  [WARNING] git not found on PATH.\n";
        }
    }

    std::cout << "\n[SUCCESS] Project '" << projName << "' ready!\n"
              << "  Run: combust main.sfpp\n";
}

static void handleAdd(const std::string& package) {
    if (!fs::exists("fuse.json")) {
        std::cerr << "[E_FUSE_404]: No fuse.json found. Run 'fuse init' first.\n";
        return;
    }
    size_t slash = package.find('/');
    if (slash == std::string::npos || slash == 0 || slash == package.size() - 1) {
        std::cerr << "[E_FUSE_400]: Invalid format '" << package << "'. Expected user/repo\n";
        return;
    }
    std::string user = package.substr(0, slash);
    std::string repo = package.substr(slash + 1);
    fs::path dest = packagePath(user, repo);

    std::cout << "Adding @" << user << "/" << repo << "...\n";
    try { fs::create_directories(dest.parent_path()); }
    catch (const std::exception& e) {
        std::cerr << "[E_IO_500]: " << e.what() << "\n"; return;
    }

    std::string url = "https://raw.githubusercontent.com/" + user + "/" + repo + "/main/" + repo + ".sfpp";
    std::cout << "  Fetching " << url << "...\n";
    bool ok = downloadFile(url, dest);
    if (!ok) {
        std::cout << "  [INFO] Download failed. Creating stub package...\n";
        writeMockPackage(dest, user, repo);
        ok = true;
        std::cout << "  [CREATED] " << dest.string() << " (stub)\n";
    } else {
        std::cout << "  [DOWNLOADED] " << dest.string() << "\n";
    }

    if (ok) {
        std::string json = readFuseJson();
        writeFuseJson(jsonAddDep(json, package));
        std::cout << "  [UPDATED] fuse.json\n"
                  << "[SUCCESS] " << user << "/" << repo << " added.\n"
                  << "  Import: import " << user << "/" << repo << ";\n";
    }
}

static void handleAddLocal(const std::string& srcPath) {
    if (!fs::exists("fuse.json")) {
        std::cerr << "[E_FUSE_404]: No fuse.json found. Run 'fuse init' first.\n";
        return;
    }
    fs::path src(srcPath);
    if (!fs::exists(src)) {
        std::cerr << "[E_FUSE_404]: Path not found: " << srcPath << "\n";
        return;
    }

    // Determine name: use stem (without extension)
    std::string stem = src.stem().string();
    // Store as packages/local/<stem>.sfpp
    fs::path dest = fs::path("packages") / "local" / (stem + ".sfpp");
    try { fs::create_directories(dest.parent_path()); }
    catch (const std::exception& e) {
        std::cerr << "[E_IO_500]: " << e.what() << "\n"; return;
    }

    try {
        fs::copy_file(src, dest, fs::copy_options::overwrite_existing);
    } catch (const std::exception& e) {
        std::cerr << "[E_IO_500]: Copy failed: " << e.what() << "\n";
        return;
    }

    std::string pkgId = "local/" + stem;
    std::string json = readFuseJson();
    writeFuseJson(jsonAddDep(json, pkgId));

    std::cout << "[SUCCESS] Local package installed.\n"
              << "  File:   " << dest.string() << "\n"
              << "  Import: import local/" << stem << ";\n";
}

static void handleRem(const std::string& package) {
    if (!fs::exists("fuse.json")) {
        std::cerr << "[E_FUSE_404]: No fuse.json found.\n";
        return;
    }
    size_t slash = package.find('/');
    if (slash == std::string::npos || slash == 0 || slash == package.size() - 1) {
        std::cerr << "[E_FUSE_400]: Invalid format '" << package << "'. Expected user/repo\n";
        return;
    }
    std::string user = package.substr(0, slash);
    std::string repo = package.substr(slash + 1);
    fs::path dest = packagePath(user, repo);

    std::cout << "Removing @" << user << "/" << repo << "...\n";
    if (fs::exists(dest)) {
        try {
            fs::remove(dest);
            std::cout << "  [DELETED] " << dest.string() << "\n";
            if (fs::is_empty(dest.parent_path())) {
                fs::remove(dest.parent_path());
                std::cout << "  [DELETED] " << dest.parent_path().string() << "/\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "  [WARNING] " << e.what() << "\n";
        }
    } else {
        std::cout << "  [INFO] Package files not found locally.\n";
    }

    std::string json = readFuseJson();
    writeFuseJson(jsonRemDep(json, package));
    std::cout << "  [UPDATED] fuse.json\n"
              << "[SUCCESS] " << user << "/" << repo << " removed.\n";
}

static void handleUpdateOne(const std::string& package) {
    size_t slash = package.find('/');
    // skip local packages
    if (slash == std::string::npos || package.substr(0, slash) == "local") {
        std::cout << "  [SKIP] " << package << " (local, skipping re-download)\n";
        return;
    }
    std::string user = package.substr(0, slash);
    std::string repo = package.substr(slash + 1);
    fs::path dest = packagePath(user, repo);
    try { fs::create_directories(dest.parent_path()); } catch (...) {}

    std::string url = "https://raw.githubusercontent.com/" + user + "/" + repo + "/main/" + repo + ".sfpp";
    std::cout << "  Updating @" << user << "/" << repo << "...\n";
    bool ok = downloadFile(url, dest);
    if (ok) {
        std::cout << "  [UPDATED] " << dest.string() << "\n";
    } else {
        std::cout << "  [WARN] Could not fetch update for @" << user << "/" << repo << "\n";
    }
}

static void handleUpdate(const std::string& specificPkg = "") {
    if (!fs::exists("fuse.json")) {
        std::cerr << "[E_FUSE_404]: No fuse.json found. Run 'fuse init' first.\n";
        return;
    }
    std::string json = readFuseJson();
    if (!specificPkg.empty()) {
        std::cout << "Updating " << specificPkg << "...\n";
        handleUpdateOne(specificPkg);
        std::cout << "[SUCCESS] Update complete.\n";
        return;
    }
    auto deps = jsonGetDeps(json);
    if (deps.empty()) {
        std::cout << "[INFO] No dependencies to update.\n";
        return;
    }
    std::cout << "Updating " << deps.size() << " package(s)...\n";
    for (auto& d : deps) handleUpdateOne(d);
    std::cout << "[SUCCESS] All packages updated.\n";
}

static void handleList() {
    if (!fs::exists("fuse.json")) {
        std::cerr << "[E_FUSE_404]: No fuse.json found. Run 'fuse init' first.\n";
        return;
    }
    std::string json = readFuseJson();
    std::string name = jsonGetStr(json, "name");
    std::string version = jsonGetStr(json, "version");
    std::cout << "Project: " << name << " v" << version << "\n\n";

    auto deps = jsonGetDeps(json);
    if (deps.empty()) {
        std::cout << "No packages installed.\n";
        return;
    }
    std::cout << "Installed packages (" << deps.size() << "):\n";
    for (auto& d : deps) {
        size_t slash = d.find('/');
        std::string user = (slash != std::string::npos) ? d.substr(0, slash) : d;
        std::string repo = (slash != std::string::npos) ? d.substr(slash + 1) : d;
        fs::path p = packagePath(user, repo);
        std::string status = fs::exists(p) ? "[installed]" : "[missing]  ";
        std::cout << "  " << status << " @" << d << "\n";
    }
}

static void handleRun(const std::string& scriptName) {
    if (!fs::exists("fuse.json")) {
        std::cerr << "[E_FUSE_404]: No fuse.json found. Run 'fuse init' first.\n";
        return;
    }
    // Parse scripts section manually
    std::string json = readFuseJson();
    size_t scriptsIdx = json.find("\"scripts\"");
    if (scriptsIdx == std::string::npos) {
        std::cerr << "[E_FUSE_404]: No 'scripts' section in fuse.json.\n";
        return;
    }
    size_t open = json.find('{', scriptsIdx);
    size_t close = json.find('}', open);
    if (open == std::string::npos || close == std::string::npos) {
        std::cerr << "[E_FUSE_400]: Malformed 'scripts' in fuse.json.\n";
        return;
    }
    std::string block = json.substr(open + 1, close - open - 1);
    // Find "scriptName": "command"
    std::string key = "\"" + scriptName + "\"";
    size_t kp = block.find(key);
    if (kp == std::string::npos) {
        std::cerr << "[E_FUSE_404]: Script '" << scriptName << "' not found in fuse.json.\n";
        return;
    }
    size_t cp = block.find(':', kp);
    if (cp == std::string::npos) { std::cerr << "[E_FUSE_400]: Bad scripts format.\n"; return; }
    size_t q1 = block.find('"', cp);
    if (q1 == std::string::npos) { std::cerr << "[E_FUSE_400]: Bad scripts format.\n"; return; }
    size_t q2 = block.find('"', q1 + 1);
    if (q2 == std::string::npos) { std::cerr << "[E_FUSE_400]: Bad scripts format.\n"; return; }
    std::string cmd = block.substr(q1 + 1, q2 - q1 - 1);
    std::cout << "> " << cmd << "\n";
    std::system(cmd.c_str());
}

static void handlePublish() {
    printBanner();
    std::cout << "\nfuse publish -- Package Builder\n\n";

    // Gather info
    std::string pkgName, userName, pkgVersion, pkgDesc;
    std::string defaultDir = fs::current_path().filename().string();

    std::cout << "Package name (" << defaultDir << "): ";
    if (!getInput(pkgName)) return;
    if (isWhitespace(pkgName)) pkgName = defaultDir;
    for (char& c : pkgName) if (c == ' ') c = '-';

    std::cout << "Your GitHub username: ";
    if (!getInput(userName)) return;
    if (isWhitespace(userName)) userName = "anonymous";

    std::cout << "Version (1.0.0): ";
    if (!getInput(pkgVersion)) return;
    if (isWhitespace(pkgVersion)) pkgVersion = "1.0.0";

    std::cout << "Description: ";
    if (!getInput(pkgDesc)) return;
    if (isWhitespace(pkgDesc)) pkgDesc = "A Sulfur++ package";

    std::cout << "\nCreating package template...\n";

    // Create package directory
    try {
        fs::create_directories(pkgName);
        fs::current_path(pkgName);
        std::cout << "  [CREATED] " << pkgName << "/\n";
    } catch (const std::exception& e) {
        std::cerr << "  [ERROR] " << e.what() << "\n";
        return;
    }

    // Main package file: <pkgName>.sfpp
    {
        std::ofstream f(pkgName + ".sfpp");
        f << "// " << userName << "/" << pkgName << " v" << pkgVersion << "\n"
          << "// " << pkgDesc << "\n\n"
          << "export this as " << userName << "/" << pkgName << ";\n\n"
          << "import std/builtin --use=[NOLIBNAME];\n\n"
          << "// Public API\n"
          << "fn hello() {\n"
          << "    Terminal.Out << \"Hello from " << userName << "/" << pkgName << "!\\n\";\n"
          << "}\n";
        std::cout << "  [CREATED] " << pkgName << ".sfpp\n";
    }

    // fuse.json for the package
    {
        std::ofstream f("fuse.json");
        f << "{\n"
          << "  \"name\": \"" << pkgName << "\",\n"
          << "  \"version\": \"" << pkgVersion << "\",\n"
          << "  \"description\": \"" << pkgDesc << "\",\n"
          << "  \"author\": \"" << userName << "\",\n"
          << "  \"main\": \"" << pkgName << ".sfpp\",\n"
          << "  \"repository\": {\n"
          << "    \"type\": \"git\",\n"
          << "    \"url\": \"https://github.com/" << userName << "/" << pkgName << "\"\n"
          << "  },\n"
          << "  \"dependencies\": {}\n"
          << "}\n";
        std::cout << "  [CREATED] fuse.json\n";
    }

    // README.md
    {
        std::ofstream f("README.md");
        f << "# " << userName << "/" << pkgName << "\n\n"
          << pkgDesc << "\n\n"
          << "## Installation\n\n"
          << "```\n"
          << "fuse add " << userName << "/" << pkgName << "\n"
          << "```\n\n"
          << "## Usage\n\n"
          << "```\n"
          << "import " << userName << "/" << pkgName << ";\n\n"
          << pkgName << ".hello();\n"
          << "```\n\n"
          << "## API\n\n"
          << "### `hello()`\n"
          << "Prints a hello message.\n";
        std::cout << "  [CREATED] README.md\n";
    }

    // examples/hello.sfpp
    {
        fs::create_directories("examples");
        std::ofstream f("examples/hello.sfpp");
        f << "import " << userName << "/" << pkgName << ";\n\n"
          << pkgName << ".hello();\n";
        std::cout << "  [CREATED] examples/hello.sfpp\n";
    }

    // .gitignore
    {
        std::ofstream f(".gitignore");
        f << "packages/\nbuild/\nbin/\n*.exe\n";
        std::cout << "  [CREATED] .gitignore\n";
    }

    // git init
    std::cout << "\nInitializing git...\n";
    if (std::system("git init") != 0)
        std::cout << "  [WARNING] git not found on PATH.\n";

    std::cout
        << "\n+--------------------------------------------------+\n"
        << " [SUCCESS] Package '" << userName << "/" << pkgName << "' created!\n"
        << "\n"
        << " Next steps:\n"
        << "   1. Edit " << pkgName << ".sfpp with your package logic\n"
        << "   2. Push to GitHub: https://github.com/" << userName << "/" << pkgName << "\n"
        << "   3. Users install with: fuse add " << userName << "/" << pkgName << "\n"
        << "+--------------------------------------------------+\n";
}

// ─── main ──────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    std::signal(SIGINT, [](int sig) {
        std::cout << "\n[E_KEYBOARD_INT!] Whoops! Keyboard interrupt called!\n" << std::flush;
        std::_Exit(sig);
    });

    if (argc < 2) { printHelp(); return 0; }

    std::string cmd = argv[1];

    if (cmd == "--help" || cmd == "-h") {
        printHelp();
    }
    else if (cmd == "--version" || cmd == "-v") {
        std::cout << "fuse " << __FUSE_VERSION__ << " (Sulfur++ Package Manager)\n";
    }
    else if (cmd == "init") {
        handleInit();
    }
    else if (cmd == "add") {
        if (argc < 3) {
            std::cerr << "[E_FUSE_400]: Usage: fuse add <user>/<package>\n"
                      << "              fuse add local <path>\n";
            return 1;
        }
        std::string arg2 = argv[2];
        if (arg2 == "local") {
            if (argc < 4) {
                std::cerr << "[E_FUSE_400]: Usage: fuse add local <path>\n";
                return 1;
            }
            handleAddLocal(argv[3]);
        } else {
            handleAdd(arg2);
        }
    }
    else if (cmd == "rem" || cmd == "remove" || cmd == "uninstall") {
        if (argc < 3) {
            std::cerr << "[E_FUSE_400]: Usage: fuse rem <user>/<package>\n";
            return 1;
        }
        handleRem(argv[2]);
    }
    else if (cmd == "update" || cmd == "upgrade") {
        if (argc >= 3) {
            handleUpdate(argv[2]);
        } else {
            handleUpdate();
        }
    }
    else if (cmd == "list" || cmd == "ls") {
        handleList();
    }
    else if (cmd == "publish" || cmd == "pack") {
        handlePublish();
    }
    else if (cmd == "run") {
        if (argc < 3) {
            std::cerr << "[E_FUSE_400]: Usage: fuse run <script>\n";
            return 1;
        }
        handleRun(argv[2]);
    }
    else {
        std::cerr << "Unknown command: '" << cmd << "'\n\n";
        printHelp();
        return 1;
    }
    return 0;
}
