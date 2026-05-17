#pragma once
#include <string>
#include <stdexcept>
#include <memory>

// Error format: <SEVERITY>_<CATEGORY>_<CODE>
// FE = fatal error, E = error, W = warning

struct SulfurError : public std::runtime_error {
    std::string code;
    int line;

    SulfurError(const std::string& code, const std::string& msg, int line = -1)
        : std::runtime_error(msg), code(code), line(line) {}
};

struct LexError   : public SulfurError {
    LexError(const std::string& msg, int line)
        : SulfurError("E_LEX_001", msg, line) {}
};

struct ParseError : public SulfurError {
    ParseError(const std::string& msg, int line)
        : SulfurError("E_PARSE_001", msg, line) {}
};

struct TypeError : public SulfurError {
    TypeError(const std::string& msg, int line = -1)
        : SulfurError("E_TYPE_001", msg, line) {}
};

struct RuntimeError : public SulfurError {
    RuntimeError(const std::string& msg, int line = -1)
        : SulfurError("E_RUNTIME_001", msg, line) {}
};

struct ImportError : public SulfurError {
    ImportError(const std::string& msg, int line = -1)
        : SulfurError("E_IMPORT_409", msg, line) {}
};

struct FatalError : public SulfurError {
    FatalError(const std::string& msg, int line = -1)
        : SulfurError("FE_VM_001", msg, line) {}
};

struct NameError : public SulfurError {
    NameError(const std::string& name, int line = -1)
        : SulfurError("E_NAME_001", "Undefined identifier: '" + name + "'", line) {}
};

// Control flow signals (not real errors)
struct ReturnSignal {
    std::shared_ptr<struct Value> value;
};
struct BreakSignal  {};
struct ContinueSignal {};
