# Sulfur++ Documentation

Complete documentation for the Sulfur++ language, compiler, and standard library.

---

## Table of Contents

### Language Reference
- [Language Reference](language_reference.md) — Complete syntax, types, keywords, operators, control flow, functions, classes, error handling, modules
- [Parser & AST](parser_ast.md) — Internal parser architecture, AST node definitions, parsing algorithms

### Standard Library
- [Standard Library Reference](stdlib_reference.md) — All 16 modules with complete API documentation
- [Built-in Functions Reference](builtins_reference.md) — All 130+ native functions with signatures and examples

### Runtime Internals
- [Value System & Types](value_system.md) — Internal value representation, type system, equality, truthiness
- [FFI & Unsafe](ffi_unsafe.md) — Foreign Function Interface, unsafe blocks, memory management
- [Native Modules](native_modules.md) — C/C++ extension modules, module initialization, API reference

---

## Quick Start

### Installation
```bash
git clone https://github.com/yourorg/sulfurplusplus-lang.git
cd sulfurplusplus-lang
mkdir build && cd build
cmake .. && cmake --build . --config Release
```

### Hello World
```sfpp
#!/usr/bin/env combust
import std/io as io;

io.Terminal.Out << "Hello, Sulfur++!" << io.Terminal.EOL;
```

### Run
```bash
./combust hello.sfpp
```

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                      Sulfur++ Source (.sfpp)                 │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                         Lexer                                 │
│  (src/core/lexer.cpp) — Tokenizes source into Token stream   │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                         Parser                                │
│  (src/core/parser.cpp) — Recursive descent + Pratt parsing  │
│  Produces: AST (include/ast.hpp)                             │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                      Interpreter                              │
│  (src/core/interpreter.cpp) — Tree-walking execution         │
│  • Registers 130+ native builtins                            │
│  • Manages environments, closures, classes                   │
│  • Tiered JIT (LLVM) for hot functions                       │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                    Value System                               │
│  (include/value.hpp, src/core/value.cpp)                     │
│  Unified tagged-union representation for all runtime values  │
└─────────────────────────────────────────────────────────────┘
```

---

## Key Components

| Component | Files | Description |
|-----------|-------|-------------|
| **Lexer** | `src/core/lexer.cpp`, `include/token.hpp` | Tokenization, keywords, literals |
| **Parser** | `src/core/parser.cpp`, `include/parser.hpp` | Recursive descent, Pratt expressions |
| **AST** | `include/ast.hpp` | Variant-based node hierarchy |
| **Interpreter** | `src/core/interpreter.cpp`, `include/interpreter.hpp` | Execution engine, builtins, JIT |
| **Values** | `include/value.hpp`, `src/core/value.cpp` | Tagged union runtime representation |
| **Environment** | `include/environment.hpp` | Lexical scoping, closures |
| **Standard Library** | `src/stdlib/*.sfpp` | 16 modules, native-backed |

---

## Language Features at a Glance

| Feature | Status |
|---------|--------|
| Dynamic typing (`auto`) | ✅ |
| Static type annotations | ✅ |
| Generics (`list<int_64>`) | ✅ |
| Nullable types (`str?`) | ✅ |
| Classes (ref semantics) | ✅ |
| Structs (value semantics) | ✅ |
| Interfaces | ✅ |
| Lambdas / closures | ✅ |
| Pipeline operator (`->`) | ✅ |
| Match expressions | ✅ |
| PS-strings (templates) | ✅ |
| Try/catch/finally | ✅ |
| Defer statements | ✅ |
| Unsafe blocks | ✅ |
| FFI (dlopen/dlsym) | ✅ |
| Tiered JIT (LLVM) | ✅ |
| REPL | ✅ |
| Watch mode | ✅ |

---

## Standard Library Modules

| Module | Purpose | Key Exports |
|--------|---------|-------------|
| `std/builtin` | Core builtins | `delay`, `len`, `range`, `typeOf`, `Terminal`, `TIO` |
| `std/io` | File I/O | `readFile`, `writeFile`, `listDir`, `mkdir` |
| `std/sys` | System/hardware | `GPIO`, `WiFiManager`, `exec`, `Pointer` |
| `std/collections` | List/dict/set ops | `map`, `filter`, `reduce`, `sort`, `keys` |
| `std/math` | Math & complex | `sin`, `cos`, `sqrt`, `complex`, `matrix_*` |
| `std/matrix` | Linear algebra | `m_Add`, `m_Mul`, `m_Transpose`, `m_Eye` |
| `std/string` | String ops | `trim`, `split`, `join`, `replace`, `slice` |
| `std/json` | JSON | `parse`, `stringify`, `pretty`, `parseFile` |
| `std/time` | Timing | `delay`, `micros`, `millis`, `seconds` |
| `std/hal` | Hardware abstraction | `Pin`, `createPin`, `HIGH/LOW` |
| `std/events` | Async event loop | `setInterval`, `setTimeout`, `run` |
| `std/constants` | Runtime constants | `SECOND`, `MINUTE`, `VERSION` |
| `std/runtime` | Runtime info | `Runtime.uptimeMillis()`, `getEngineName()` |
| `std/science` | Physical constants | `C`, `G`, `H`, `K`, `NA`, `R` |
| `std/alias` | Alias system | `Alias.create(pattern, expansion)` |
| `std/ffi` | Foreign Function Interface | `load`, `callInt`, `memRead`, `sizeof` |

---

## Built-in Constants

```sfpp
// Mathematical
PI, E, TAU, PHI, INF, NEG_INF, NAN

// Scientific
SC_C (299792458), SC_G (6.67430e-11), SC_H (6.62607015e-34)
SC_K (1.380649e-23), SC_NA (6.02214076e23), SC_R (8.314462618)

// Time
SECOND (1), MINUTE (60), HOUR (3600), DAY (86400), WEEK (604800)

// Version
RUNTIME_VERSION, SULFUR_VERSION, COMBUST_VERSION, BUILD_MODE, DEBUG_MODE
```

---

## Error System

```sfpp
// Throw structured errors
throw TIO.E("message")
    |> TIO.withCode("ERR_001")
    |> TIO.withCategory("NETWORK")
    |> TIO.withHint("Check connection")
    |> TIO.withContext({port: 8080});

// Catch
try { risky() }
catch (e) {
    // e is dict with: __type__, severity, message, code, category, hint, context
    io.Terminal.Err << "Caught: " << e["message"] << "\n";
}
```

**Severities:** `E` (Error), `FE` (Fatal Error), `W` (Warning)

---

## Development

### Building
```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --config Debug

# Release build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

### Running Tests
```bash
./combust tests/basic.sfpp
./combust tests/match.sfpp
./combust tests/ffi.sfpp
```

### Debugging
```bash
# Debug mode (traces)
./combust script.sfpp --debug

# Force JIT
./combust script.sfpp --jit

# Watch for changes
./combust script.sfpp --watch
```

---

## Contributing

1. Fork the repository
2. Create feature branch
3. Add tests for new functionality
4. Run full test suite
5. Submit PR with description

### Code Style
- C++17, modern idioms
- `clang-format` for formatting
- No external dependencies (except LLVM for JIT)
- Comprehensive error messages with line/column

---

## License

MIT License — see `LICENSE` file.

---

## Links

- **Repository**: https://github.com/yourorg/sulfurplusplus-lang
- **Issues**: https://github.com/yourorg/sulfurplusplus-lang/issues
- **Discussions**: https://github.com/yourorg/sulfurplusplus-lang/discussions

---

*Generated from source code — see individual .md files for complete details.*