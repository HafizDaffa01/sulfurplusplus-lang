# Sulfur++ Roadmap

## Completed Features

### Core Language
- Dynamic typing with support for `null`, `bool`, `int_64`, `float_64`, `complex_128`, `str`, `char`, `list`, `dict`, `set`, `fn`, `class`, `struct`, `ptr`
- FFI - C/C++ interop (`libc` `dlopen`, `dlsym`, `call`, `malloc/free`, memory read/write)
- Async/await with stackful coroutines (`ucontext`)
- Pipeline operator `|>`
- Single version system (`SFPP_VERSION_STRING`)
- Object.freeze() / Object.seal() (JavaScript-like behavior)
- REPL history and completion (readline/editline)

### Parser & Language Features
- Parser support for `->` return type notation in function signatures
- Struct/class declarations
- Lambda expressions with proper parameter types
- Member expressions (field name lookup via `structFieldNames`)
- New expressions (malloc allocation)
- Index expressions (array/list indexing via GEP)
- Pipeline expressions (`left |> right`)
- Null coalescing expressions (`left ?? right`)
- List literals `[a, b, c]`
- Dict literals `{"key": value}`
- Address-of `&` and dereference `*x`
- Delete expressions (`delete x`)
- Await expressions (async/await)
- Interface declarations (opaque struct)
- Export/Expose/Overwrite statements
- Try/catch/finally exception handling (setjmp/longjmp)
- Throw statements
- Match statement (pattern matching)
- `@jit` annotation on `FnDeclStmt`
- Full import resolution: relative, project-local, std/, packages/, scoped user/repo

### AOT / LLVM Compilation
- AOT compilation lowering for all major language features
- Lambda expressions (with proper parameter types)
- Module import resolution for AOT (linking imported module symbols)
- AOT optimizations (dead code elimination, constant folding, inline)
- `getLLVMType` extended for `str`, `list`, `any`, `fn`, `matrix` types
- `emitDeclarations` called before `emitAllFunctions`
- `@jit`-aware function filtering in `forwardDeclareAll` / `emitAllFunctions`
- FFI call expressions (`dlopen`/`dlsym` runtime binding for imported modules)
- Embedded support (Arduino/ESP32) - LLVM AOT to object files
- Cross-compilation to real boards (ESP-IDF, AVR-GCC, flashing)

### CLI & Tooling
- Install/uninstall scripts for Linux/macOS/Windows (with PATH management)
- CLI flags: `--debug`, `--lang-constants`, `--tokens`, `--parse-only`, `--ast`, `--list-modules`, `--list-builtins`
- Package manager (`sfpp -i` / `--install` / `--uninstall` / `--list-packages` / `--sync`)
- Scoped package structure (`.sfpp/packages/<user>/<repo>/`)
- Package manager dependency resolution + lockfile (`sfpp-lock.toml`, `--sync`, auto-update `sfpp-project.toml`)

### Standard Library
- `std/io`: File system and terminal I/O
- `std/sys`: System hardware, network, and OS process management
- `std/builtin`: Core language builtins, time, and math utilities
- `std/collections`: Utilities for lists, dicts, and sets
- `std/math`: Advanced mathematical operations and complex constructors
- `std/matrix`: Linear algebra engine
- `std/string`: String manipulation
- `std/json`: JSON parsing and serialization

## Incomplete / Remaining Work

### Core Language
- Full closure support for lambda captures (captures from enclosing scope)

### Tooling & Infrastructure
- Build verification (compile and test all changes)
- Test suite for AOT features
- User documentation for new features

### Embedded Targets
- Linker scripts for ESP32/AVR embedded targets (`esp32.ld`)
- Startup code (`crt0`) for embedded targets
- Auto-detect serial port for flashing
