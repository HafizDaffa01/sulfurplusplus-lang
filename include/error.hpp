#pragma once
#include <string>
#include <stdexcept>
#include <memory>

// Error format: <SEVERITY>_<CATEGORY>_<CODE>
// FE = fatal error, E = error, W = warning

struct SulfurError : public std::runtime_error {
    std::string code;
    std::string hint; // optional hint for the user
    int line;

    SulfurError(const std::string& code, const std::string& msg, int line = -1,
                const std::string& hint = "")
        : std::runtime_error(msg), code(code), hint(hint), line(line) {}
};

struct LexError : public SulfurError {
    LexError(const std::string& msg, int line,
             const std::string& code = "E_LEX_400",
             const std::string& hint = "")
        : SulfurError(code, msg, line, hint) {}
};

struct ParseError : public SulfurError {
    ParseError(const std::string& msg, int line,
               const std::string& code = "E_PARSE_400",
               const std::string& hint = "")
        : SulfurError(code, msg, line, hint) {}
};

struct TypeError : public SulfurError {
    TypeError(const std::string& msg, int line = -1,
              const std::string& code = "E_TYPE_406",
              const std::string& hint = "")
        : SulfurError(code, msg, line, hint) {}
};

struct RuntimeError : public SulfurError {
    RuntimeError(const std::string& msg, int line = -1,
                 const std::string& code = "E_RUNTIME_500",
                 const std::string& hint = "")
        : SulfurError(code, msg, line, hint) {}
};

struct ImportError : public SulfurError {
    ImportError(const std::string& msg, int line = -1,
                const std::string& code = "E_IMPORT_409",
                const std::string& hint = "Check that the module path is correct and the file exists.")
        : SulfurError(code, msg, line, hint) {}
};

struct FatalError : public SulfurError {
    FatalError(const std::string& msg, int line = -1,
               const std::string& code = "FE_VM_500",
               const std::string& hint = "")
        : SulfurError(code, msg, line, hint) {}
};

struct NameError : public SulfurError {
    NameError(const std::string& name, int line = -1)
        : SulfurError("E_NAME_404",
                      "Undefined identifier: '" + name + "'",
                      line,
                      "Check for typos, or ensure '" + name + "' is declared before use.") {}
};

struct MathError : public SulfurError {
    MathError(const std::string& msg, int line = -1,
              const std::string& code = "E_MATH_422",
              const std::string& hint = "Check divisor/modulo operand is not zero.")
        : SulfurError(code, msg, line, hint) {}
};

struct IndexError : public SulfurError {
    IndexError(const std::string& msg, int line = -1,
               const std::string& code = "E_INDEX_416",
               const std::string& hint = "Check list bounds with len() before indexing.")
        : SulfurError(code, msg, line, hint) {}
};

struct MemoryError : public SulfurError {
    MemoryError(const std::string& msg, int line = -1,
                const std::string& code = "E_MEMORY_403",
                const std::string& hint = "Check for null pointer dereference or out-of-scope access.")
        : SulfurError(code, msg, line, hint) {}
};

struct IOError : public SulfurError {
    IOError(const std::string& msg, int line = -1,
            const std::string& code = "E_IO_404",
            const std::string& hint = "Check that the file path exists and you have read/write permissions.")
        : SulfurError(code, msg, line, hint) {}
};

#include "value.hpp"

// ... (rest of error types)

// Control flow signals (not real errors)
struct ReturnSignal {
    ValuePtr value;
};
struct BreakSignal  {};
struct ContinueSignal {};
