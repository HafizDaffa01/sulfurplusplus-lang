# Contributing to Sulfur++

Thank you for your interest in contributing to Sulfur++! This guide aligns with our [roadmap](roadmap.md) and will help you get started.

## Getting Started

### Prerequisites
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 19.28+)
- CMake 3.20+
- LLVM 15+ (for AOT compilation features)
- Python 3.8+ (for test scripts)

### Building from Source
```bash
git clone https://github.com/HafizDaffa01/sulfurplusplus-lang.git
cd sulfurplusplus-lang
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

### Running Tests
```bash
# Run all tests
./run_tests.sh

# Run specific test suite
./build/tests/test_runner 01_arithmetic
```

## Project Structure

```
src/
├── core/           # Core language implementation
│   ├── lexer.cpp      # Lexical analysis
│   ├── parser.cpp     # Parsing
│   ├── interpreter.cpp # Runtime interpreter
│   ├── resolver.cpp   # Name resolution
│   ├── semantic/      # Semantic analysis & type checking
│   │   ├── analyzer.cpp
│   │   ├── symbol_table.cpp
│   │   ├── type.cpp
│   │   └── type_checker.cpp
│   ├── formatter.cpp  # Code formatter
│   ├── linter.cpp     # Static analysis
│   └── diagnostic.hpp # Error reporting
├── llvm/           # LLVM AOT compilation backend
│   ├── llvm_aot.cpp
│   ├── llvm_ir_builder.cpp
│   └── *.ld         # Linker scripts for embedded
├── stdlib/         # Standard library (`.sfpp` files)
│   ├── io.sfpp
│   ├── sys.sfpp
│   ├── ffi.sfpp
│   ├── crypto.sfpp
│   ├── http.sfpp
│   ├── regex.sfpp
│   ├── path.sfpp
│   ├── time.sfpp
│   ├── math.sfpp
│   ├── matrix.sfpp
│   ├── string.sfpp
│   ├── collections.sfpp
│   ├── json.sfpp
│   └── builtin.sfpp
└── tools/          # CLI tools
    ├── combust.cpp   # Main CLI (REPL, compile, run)
    ├── compiler.cpp  # AOT compiler
    ├── sfpm.cpp      # Package manager
    ├── lsp.cpp       # Language Server Protocol
    └── debugger.cpp  # Interactive debugger
```

## Development Workflow

### Code Style
- Follow existing code style in the codebase
- Use 4 spaces for indentation (no tabs)
- Maximum line length: 100 characters
- Run clang-format before committing:
  ```bash
  clang-format -i src/**/*.cpp src/**/*.hpp include/**/*.hpp
  ```

### Adding Features by Area

#### Core Language (`src/core/`, `include/`)
- **Lexer/Parser**: `lexer.cpp`, `parser.cpp`, `include/parser.hpp`, `include/ast.hpp`
- **Semantic Analysis**: `semantic/analyzer.cpp`, `semantic/symbol_table.cpp`, `semantic/type_checker.cpp`
- **Interpreter**: `interpreter.cpp`, `environment.cpp`, `value.cpp`
- **Resolver**: `resolver.cpp` (name resolution, closure capture)

#### AOT / LLVM Backend (`src/llvm/`)
- Code generation: `llvm_ir_builder.cpp`, `llvm_aot.cpp`
- Type lowering: `getLLVMType()` for `str`, `list`, `any`, `fn`, `matrix`
- Embedded targets: Linker scripts (`esp32.ld`, `avr.ld`), startup code (`crt0.S`)

#### Standard Library (`src/stdlib/`)
- Add `.sfpp` files and register in `CMakeLists.txt`
- Follow existing patterns for native bindings in `src/tools/`

#### CLI Tools (`src/tools/`)
- **combust**: Main entry point, REPL, compilation pipeline
- **sfpm**: Package manager (init, run, test, bench, fmt, lint, build, install)
- **lsp**: Language Server Protocol for VS Code
- **debugger**: Interactive debugging (DAP support)

### Testing
- Every new feature must include tests
- Tests are `.sfpp` files in `tests/` directory (numbered: `01_`, `02_`, etc.)
- Run `./run_tests.sh` to validate changes
- Target: All 24 test suites passing

## Current Roadmap Priorities

Based on [roadmap.md](roadmap.md), the following areas need contribution:

### 🔴 High Priority (Remaining Work)

| Area | Tasks | Files |
|------|-------|-------|
| **Closure Captures** | Full closure support for lambda captures from enclosing scope | `src/core/resolver.cpp`, `src/core/interpreter.cpp`, `include/semantic/*` |
| **AOT Test Suite** | Test coverage for AOT compilation features | `tests/` (new test files) |
| **Build Verification** | CI/CD improvements, compile & test all changes | `.github/workflows/ci.yml` |
| **Embedded Targets** | Linker scripts (`esp32.ld`), startup code (`crt0.S`), serial port auto-detect | `src/llvm/*.ld`, `src/llvm/*crt0.S` |

### 🟡 Medium Priority

| Area | Tasks |
|------|-------|
| **User Documentation** | Document new features (language spec, stdlib, tooling) |
| **LSP Enhancements** | Better completions, hover, go-to-definition |
| **Package Manager** | Dependency resolution improvements, lockfile handling |
| **Debugger** | Breakpoints, watch expressions, call stack inspection |

### 🟢 Good First Issues

- Add missing stdlib modules (see `src/stdlib/`)
- Improve error messages in `src/core/diagnostic.hpp`
- Add linter rules in `src/core/linter.cpp`
- Extend formatter in `src/core/formatter.cpp`
- Write tests for edge cases in `tests/`

## Pull Request Process

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Make changes with clear, focused commits
4. Run tests and ensure they pass (`./run_tests.sh`)
5. Run linter: `./build/sfpm lint` (if available)
6. Update documentation if needed (`docs/`, inline comments)
7. Submit a PR with a clear description referencing roadmap items

### Commit Message Format
```
<type>(<scope>): <description>

Types: feat, fix, docs, test, refactor, perf, chore
Scopes: lexer, parser, semantic, interpreter, llvm, stdlib, combust, sfpm, lsp, debugger
```

## Reporting Issues

- Use GitHub Issues for bug reports and feature requests
- Include minimal reproduction steps
- Specify OS, compiler version, and Sulfur++ version
- Reference roadmap items when applicable

## Code of Conduct

Be respectful and constructive in all interactions. This project follows the [Contributor Covenant](https://www.contributor-covenant.org/).

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (MIT).