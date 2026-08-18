<p align="center">
  <img src="assets/logo.png" alt="Sulfur++ Logo" width="480">
</p>

<p align="center">
  <b>A high-performance, modern programming language for systems, data science, and developer toolchains.</b>
</p>

<p align="center">
  <a href="#-automated-test-suite"><img src="https://img.shields.io/badge/tests-24%20passed%20(100%25)-success.svg" alt="Tests"></a>
  <a href="#-performance-benchmarks"><img src="https://img.shields.io/badge/performance-benchmarked-yellow.svg" alt="Benchmarks"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License"></a>
  <a href="https://github.com/HafizDaffa01/sulfurplusplus-lang"><img src="https://img.shields.io/badge/version-1.0.3--A-orange.svg" alt="Version"></a>
</p>

---

## 🚀 Key Features

* **Dynamic & Typed Data Handling**: Standard primitives, scientific data types, collections, and custom objects.
* **Semantic Analysis & Resolver Pass**: Ahead-of-time scope analysis, immutability enforcement (`let`), and control flow verification.
* **Native Math & Science Engine**: High-performance linear algebra (matrices), complex numbers, and trigonometry.
* **Rich Standard Library**: Built-in modules for I/O, Networking/HTTP, Cryptography & Hashing, Regular Expressions, Path handling, JSON, and System utilities.
* **Modern Syntax**: Classes, constructors (`init`), methods, closures, lambdas, pattern matching (`match`), C-style & for-in loops, and pipeline operators.
* **Advanced Error Diagnostics**: Color-coded terminal diagnostics with line contexts, error codes, and actionable hints.
* **Comprehensive Automated Test Suite**: Integrated with CMake CTest and batch runners.

---

## 📦 Data Types

Sulfur++ supports the following core types natively:
* `null`: Represents an empty or undefined value.
* `bool`: Boolean `true` or `false`.
* `int_64`: 64-bit integer.
* `float_64`: 64-bit floating-point number.
* `complex_128`: 128-bit complex number (`real` + `imag` $i$).
* `str`: UTF-8 String.
* `char`: Single ASCII character.
* `list`: Dynamic array (can be nested to form matrices).
* `dict`: Hash map/dictionary with string keys.
* `set`: Unique unordered collection.
* `fn`: Function, closure, or lambda reference.
* `class` / `struct`: Custom object and data structure definitions.
* `ptr`: Raw memory pointer (for use in `unsafe` blocks).

---

## 🛠️ CLI & Execution Engine (`combust`)

The `combust` tool is the primary CLI for executing, watching, and compiling Sulfur++ code.

```bash
# Run a script
combust script.sfpp

# Interactive REPL
combust

# Watch mode (auto re-runs upon file modification)
combust script.sfpp --watch

# Debug mode (detailed trace and execution timing)
combust script.sfpp --debug

# Compile script to standalone binary
combust --compile script.sfpp -o myapp
```

On Windows, use the provided helper:
```powershell
.\combust.cmd script.sfpp
```

---

## 📚 Standard Library (`std/`)

| Module | Description | Key Functions & Classes |
| :--- | :--- | :--- |
| `std/io` | File system & Terminal I/O | `readFile`, `writeFile`, `appendFile`, `deleteFile`, `Terminal` |
| `std/crypto` | Cryptography & Encodings | `sha256`, `base64Encode`, `base64Decode`, `hexEncode`, `hexDecode` |
| `std/regex` | Regular Expressions | `match`, `search`, `replace`, `findAll` |
| `std/path` | Filesystem Path Utilities | `join`, `basename`, `dirname`, `ext`, `exists`, `isAbsolute` |
| `std/http` | HTTP Client | `Client.get()`, `Client.post()`, `Client.put()`, `Client.delete()` |
| `std/json` | JSON Serializer & Parser | `parse`, `stringify`, `pretty` |
| `std/math` | Math & Complex Numbers | `complex`, `real`, `imag`, `sqrt`, `sin`, `cos`, `log`, `pow` |
| `std/matrix` | Linear Algebra Engine | `m_Eye`, `m_Mul`, `m_Det`, `m_Inv`, `m_Transpose` |
| `std/sys` | OS & Environment Info | `sys_exec`, `sys_platform`, `sys_arch`, `sys_env` |
| `std/time` | High-Resolution Timing | `sleep`, `now`, `clock` |

---

## 🧪 Automated Testing

Sulfur++ comes with an automated test suite covering language features:

```powershell
# Run with CTest (Cross-platform)
ctest --test-dir build --output-on-failure

# Run with Windows Batch Runner
.\run_tests.cmd
```

### Test Suites Included:
* `01_arithmetic.sfpp`: Arithmetic operations & operator precedence.
* `02_control_flow.sfpp`: `if/else`, `while`, `for-in`, C-style `for`, `match`.
* `03_functions.sfpp`: Recursion, closures, higher-order functions, `defer`.
* `04_collections.sfpp`: Dynamic lists, maps, matrices.
* `05_oop.sfpp`: Classes, member initializers, `this` scoping.
* `06_exceptions.sfpp`: `try/catch/finally`, `throw`, error dictionaries.
* `07_stdlib.sfpp`: JSON, strings, math constants.
* `08_semantic_rules.sfpp`: Resolver immutability & lexical scopes.
* `09_crypto_and_regex.sfpp`: SHA-256, Base64, Hex, Regex patterns, Path utils.

---

## 💡 Examples & Recipes

Check the [`examples/`](./examples) directory for complete programs:
* [`examples/01_http_rest_api.sfpp`](./examples/01_http_rest_api.sfpp): REST API client and JSON extraction.
* [`examples/02_file_and_crypto.sfpp`](./examples/02_file_and_crypto.sfpp): File I/O, SHA-256 hashing, and Base64 encoding.
* [`examples/03_oop_matrix_math.sfpp`](./examples/03_oop_matrix_math.sfpp): OOP class vectors and native complex arithmetic.
* [`examples/04_regex_text_processing.sfpp`](./examples/04_regex_text_processing.sfpp): Log parsing, IP address extraction, and masking.
* [`examples/05_algorithms_and_closures.sfpp`](./examples/05_algorithms_and_closures.sfpp): Functional mapping, filtering, and closures.

---

## 🔨 Building from Source

### Prerequisites:
* C++17 compliant compiler (`g++`, `clang++`, or `MSVC`)
* CMake 3.20+
* Ninja (recommended) or Make
* libcurl

### Build Steps:
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## 📄 License

Sulfur++ is released under the [MIT License](./LICENSE).
