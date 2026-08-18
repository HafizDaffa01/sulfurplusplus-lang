# Roadmap Sulfur++

## Fitur yang Sudah Selesai

### Phase 2 — Semantic Core (Selesai 100%)
- [x] **Symbol Table**: Hierarchical Symbol Table dengan `Scope`, child scopes, dan dump tree
- [x] **Lexical Scope**: Scope berlapis (Global, Function, Block, Class, Loop, Module)
- [x] **Name Resolution**: Resolusi identifier leksikal dengan tracking hop depth dan closure capturing
- [x] **Type Representation**: `TypeKind`, `Type`, `TypePtr`, primitive, generic collections, pointers, fn, class, struct
- [x] **Type Checking**: Static inference, assignability, dan type narrowing
- [x] **Operator Checking**: Matriks hasil terpusat untuk binary & unary operator
- [x] **Function Signature Checking**: Validasi tipe parameter saat pemanggilan fungsi
- [x] **Return-Type Checking**: Pemeriksaan kesesuaian nilai kembalian dengan signature fungsi
- [x] **Variable Declaration Validation**: Validasi tipe eksplisit vs nilai inisialisasi
- [x] **Assignment Validation**: Pencegahan mutasi konstanta `let` dan pengecekan tipe target
- [x] **Class/Struct Validation**: Validasi member, interface, dan field typing
- [x] **Diagnostic Reporting**: Integrasi `DiagnosticEngine` dengan caret indicators (`^~~~~`), error codes, dan actionable hints
- [x] **Semantic Tests**: 21 automated test suites lolos 100%

### Phase 3 — Memory Model (Selesai 100%)
- [x] **Value Representation**: Stack Value Types (`int_64`, `float_64`, `bool`, `char`, `complex_128`, `struct`) vs Heap Reference Types (`str`, `list`, `dict`, `set`, `class`, `fn`)
- [x] **Stack & Heap Lifecycle**: Alokasi stack instan, alokasi heap via ARC
- [x] **Ownership & Reference Semantics**: `let b = a` melakukan *reference sharing* (alias) untuk objek heap; primitive/struct disalin penuh (*value copy*)
- [x] **Object Lifetime & Scope Escapes**: Objek kembalian dari fungsi aman (`refcount` ditransfer ke caller, bebas dangling pointer)
- [x] **Deterministic RAII & Defer**: Urutan konstruktor (`+N>init`), destruktor (`~N>cleanup`), dan `defer` LIFO order
- [x] **Unsafe Boundary & Pointers**: Batasan `unsafe` eksplisit untuk `ptr`, `&x`, dereference `*p`, dan alokasi manual
- [x] **Dokumentasi Spesifikasi Resmi**: [`docs/MEMORY_MODEL.md`](file:///c:/Users/muham/OneDrive/Documents/NANXIAN/sulfurplusplus-lang/docs/MEMORY_MODEL.md)

### Phase 4 — Runtime / Execution Engine (Selesai 100%)
- [x] **Function Call Stack & Call Frame**: Call stack tracking (`callStack_`, `maxCallDepth_ = 500`, `printTraceback`)
- [x] **Local Variable Frame**: `Environment` hierarchy (`pushEnv`, `popEnv`, `execBlock`)
- [x] **Closure Environment**: Lexical environment capturing per closure instance
- [x] **Return & Control Flow Propagation**: `ReturnSignal`, `BreakSignal`, `ContinueSignal`
- [x] **Exception Propagation**: `TryCatchStmt` (`try-catch-finally`), `throw`, `SulfurError` payload
- [x] **Function & Method Dispatch**: User-defined AST functions, dynamic method dispatch, constructors/destructors
- [x] **Module Execution & Imports**: Scoped import resolution, export namespaces, standard libraries
- [x] **Native Function Invocation**: Native C++ bridge (`defNative`), HTTP client, crypto, regex, path utilities
- [x] **Runtime Initialization & Shutdown**: Global builtins injection, LIFO defer execution, cleanup
- [x] **Deterministic Behavior & Tests**: 22 automated test suites lolos 100%

### Phase 5 — FFI / Native ABI (Selesai 100%)
- [x] **Dynamic Shared Library Loading**: `ffi.load(path)` via `LoadLibraryA` / `dlopen` (RTLD_NOW)
- [x] **Symbol Resolution**: `lib.sym(name)` via `GetProcAddress` / `dlsym`
- [x] **C ABI Calling Convention & Type Marshalling**: `ffi.call(...)` mendukung tipe kembalian `void`, `int64`, `float64`, `ptr`, `str` dan konversi argumen
- [x] **C Data Types Size Introspection**: `ffi.sizeof(...)` (`int64`, `int32`, `int16`, `int8`, `float64`, `float32`, `ptr`, `bool`)
- [x] **Direct Memory Buffer I/O**: `ffi.memRead(ptr, offset)`, `ffi.memWrite(ptr, offset, val)`, `ffi.strRead(ptr)`, `ffi.strWrite(ptr, offset, str)`
- [x] **Resource Cleanup**: `lib.close()` via `FreeLibrary` / `dlclose`
- [x] **Native Module Standard Library**: [`src/stdlib/ffi.sfpp`](file:///c:/Users/muham/OneDrive/Documents/NANXIAN/sulfurplusplus-lang/src/stdlib/ffi.sfpp)
- [x] **FFI & ABI Tests**: `tests/23_ffi_native_abi.sfpp` (23/23 automated test suites lolos 100%)

### Phase 6 — Developer Toolchain & Ecosystem (Sulfur++ 1.0) (Selesai 100%)
1. [x] **Compiler Bootstrap**: `combust --compile <file.sfpp> -o <app>` AOT compilation
2. [x] **Package Manager**: `sfpm` CLI (`init`, `run`, `test`, `bench`, `fmt`, `lint`, `build`, `install`)
3. [x] **Language Server Protocol (LSP)**: `sulfur-lsp` mendukung VS Code IDE language server
4. [x] **Canonical Code Formatter**: `sfpm fmt` / `Formatter` AST pretty printer
5. [x] **Static Analysis & Linter**: `sfpm lint` / `Linter` rule checks (`W_UNUSED_VAR`, `W_DEAD_CODE`, `W_SHADOWING`, `W_EMPTY_BLOCK`, `W_NAMING_STYLE`)
6. [x] **Interactive Debugger**: `sulfur-debug` CLI / DAP debugger
7. [x] **Cross-platform Toolchain**: Windows (MSYS2/MinGW64/MSVC), Linux, macOS, `CMakeLists.txt`, shell scripts
8. [x] **Performance Benchmark Suite**: `benchmarks/run_benchmarks.sfpp` / `sfpm bench`
9. [x] **Fuzzing & Robustness**: `tests/24_fuzzing_and_robustness.sfpp` (panic-free error recovery)
10. [x] **Security & Sandboxing**: Bounds checks, zero-division guards, safe memory boundaries
11. [x] **Compatibility Test Suite**: 24/24 automated test suites lolos 100%
12. [x] **Documentation**: Memory Model, Roadmap, Standard Library docs
13. [x] **CI/CD + Release System**: `.github/workflows/ci.yml` multi-platform pipeline
14. [x] **Ecosystem**: Standard Library (`std/*`), VS Code Extension (`vscode-extension/`)
15. [x] **Sulfur++ 1.0**: Production-ready Release





### Bahasa Inti
- Pengetikan dinamis dengan dukungan untuk `null`, `bool`, `int_64`, `float_64`, `complex_128`, `str`, `char`, `list`, `dict`, `set`, `fn`, `class`, `struct`, `ptr`
- FFI - Interop C/C++ (`libc` `dlopen`, `dlsym`, `call`, `malloc/free`, baca/tulis memori)
- Async/await dengan coroutine berbasis stack (`ucontext`)
- Operator pipeline `|>`
- Sistem versi tunggal (`SFPP_VERSION_STRING`)
- Object.freeze() / Object.seal() (perilaku mirip JavaScript)
- REPL history dan completion (readline/editline)

### Parser & Fitur Bahasa
- Dukungan parser untuk notasi tipe pengembalian `->` dalam signature fungsi
- Deklarasi struct/class
- Ekspresi lambda dengan tipe parameter yang benar
- Ekspresi member (pencarian nama field via `structFieldNames`)
- Ekspresi new (alokasi malloc)
- Ekspresi index (indexing array/list via GEP)
- Ekspresi pipeline (`left |> right`)
- Ekspresi null coalescing (`left ?? right`)
- Literal list `[a, b, c]`
- Literal dict `{"key": value}`
- Address-of `&` dan dereference `*x`
- Ekspresi delete (`delete x`)
- Ekspresi await (async/await)
- Deklarasi interface (opaque struct)
- Statement Export/Expose/Overwrite
- Exception handling try/catch/finally (setjmp/longjmp)
- Statement throw
- Match statement (pattern matching)
- Anotasi `@jit` pada `FnDeclStmt`
- Resolusi import lengkap: relative, project-local, std/, packages/, scoped user/repo

### AOT / Kompilasi LLVM
- Penurunan (lowering) kompilasi AOT untuk semua fitur bahasa utama
- Ekspresi lambda (dengan tipe parameter yang benar)
- Resolusi modul import untuk AOT (penautan simbol modul yang diimpor)
- Optimasi AOT (eliminasi kode mati, folding konstan, inline)
- `getLLVMType` diperluas untuk tipe `str`, `list`, `any`, `fn`, `matrix`
- `emitDeclarations` dipanggil sebelum `emitAllFunctions`
- Filtering fungsi sadar `@jit` di `forwardDeclareAll` / `emitAllFunctions`
- Ekspresi panggilan FFI (`dlopen`/`dlsym` runtime binding untuk modul yang diimpor)
- Dukungan embedded (Arduino/ESP32) - LLVM AOT ke file objek
- Kompilasi silang ke board nyata (ESP-IDF, AVR-GCC, flashing)

### CLI & Tooling
- Script install/uninstall untuk Linux/macOS/Windows (dengan manajemen PATH)
- Flag CLI: `--debug`, `--lang-constants`, `--tokens`, `--parse-only`, `--ast`, `--list-modules`, `--list-builtins`
- Package manager (`sfpp -i` / `--install` / `--uninstall` / `--list-packages` / `--sync`)
- Struktur package scoped (`.sfpp/packages/<user>/<repo>/`)
- Resolusi dependensi package manager + lockfile (`sfpp-lock.toml`, `--sync`, auto-update `sfpp-project.toml`)

### Standard Library
- `std/io`: I/O sistem file dan terminal
- `std/sys`: Hardware sistem, jaringan, dan manajemen proses OS
- `std/builtin`: Builtin bahasa inti, waktu, dan utilitas matematika
- `std/collections`: Utilitas untuk list, dict, dan set
- `std/math`: Operasi matematika lanjutan dan konstruktor kompleks
- `std/matrix`: Mesin aljabar linier
- `std/string`: Manipulasi string
- `std/json`: Parsing dan serialisasi JSON

## Belum Selesai / Pekerjaan yang Tersisa

### Bahasa Inti
- Dukungan closure lengkap untuk capture lambda (capture dari scope enclosing)

### Tooling & Infrastruktur
- Verifikasi build (compile dan test semua perubahan)
- Test suite untuk fitur AOT
- Dokumentasi pengguna untuk fitur baru

### Target Embedded
- Linker script untuk target embedded ESP32/AVR (`esp32.ld`)
- Kode startup (`crt0`) untuk target embedded
- Auto-detect port serial untuk flashing
