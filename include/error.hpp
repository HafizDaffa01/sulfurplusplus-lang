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
    LexError(const std::string& msg, int line, const std::string& code = "E_LEX_400")
        : SulfurError(code, msg, line) {}
};

struct ParseError : public SulfurError {
    ParseError(const std::string& msg, int line, const std::string& code = "E_PARSE_400")
        : SulfurError(code, msg, line) {}
};

struct TypeError : public SulfurError {
    TypeError(const std::string& msg, int line = -1, const std::string& code = "E_TYPE_406")
        : SulfurError(code, msg, line) {}
};

struct RuntimeError : public SulfurError {
    RuntimeError(const std::string& msg, int line = -1, const std::string& code = "E_RUNTIME_500")
        : SulfurError(code, msg, line) {}
};

struct ImportError : public SulfurError {
    ImportError(const std::string& msg, int line = -1, const std::string& code = "E_IMPORT_409")
        : SulfurError(code, msg, line) {}
};

struct FatalError : public SulfurError {
    FatalError(const std::string& msg, int line = -1, const std::string& code = "FE_VM_500")
        : SulfurError(code, msg, line) {}
};

struct NameError : public SulfurError {
    NameError(const std::string& name, int line = -1)
        : SulfurError("E_NAME_404", "Undefined identifier: '" + name + "'", line) {}
};

struct MathError : public SulfurError {
    MathError(const std::string& msg, int line = -1, const std::string& code = "E_MATH_422")
        : SulfurError(code, msg, line) {}
};

struct IndexError : public SulfurError {
    IndexError(const std::string& msg, int line = -1, const std::string& code = "E_INDEX_416")
        : SulfurError(code, msg, line) {}
};

struct MemoryError : public SulfurError {
    MemoryError(const std::string& msg, int line = -1, const std::string& code = "E_MEMORY_403")
        : SulfurError(code, msg, line) {}
};

struct IOError : public SulfurError {
    IOError(const std::string& msg, int line = -1, const std::string& code = "E_IO_404")
        : SulfurError(code, msg, line) {}
};

// Control flow signals (not real errors)
struct ReturnSignal {
    std::shared_ptr<struct Value> value;
};
struct BreakSignal  {};
struct ContinueSignal {};
