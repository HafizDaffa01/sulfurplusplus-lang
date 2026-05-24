# Sulfur++ Language

Sulfur++ is a high-performance, C++-backed scripting language designed for both desktop applications and embedded/IoT systems. It provides robust system access, memory management, and a rich standard library tailored for general-purpose programming, hardware interaction, and scientific computing.

## "Write Easy, Perform Fast"

## Key Features

* **Dynamic Typing**: Flexible variable handling supporting standard and scientific data types.
* **Native Math & Science Engine**: High-performance native integration of linear algebra (matrices) and complex numbers.
* **Native System Access**: Low-level memory, hardware (GPIO), and OS bindings via an `unsafe` block mechanism.
* **Robust Standard Library**: Includes modules for I/O, system utilities, math, matrices, and collections.
* **Modern Syntax**: Supports classes, structs, closures, lambda expressions, and pipeline operators.
* **Advanced Error Diagnostics**: Rich, color-coded terminal outputs indicating the exact location of runtime errors, fatal errors, and warnings.

## Data Types

Sulfur++ supports the following base types natively:
* `null`: Represents an empty or undefined value.
* `bool`: Boolean `true` or `false`.
* `int_64`: 64-bit integer.
* `float_64`: 64-bit floating-point number.
* `complex_128`: 128-bit complex number (real + imagi).
* `str`: UTF-8 String.
* `char`: Single ASCII character.
* `list`: Dynamic array (can be nested to form high-performance matrices).
* `dict`: Hash map/dictionary.
* `set`: Unique unordered collection.
* `fn`: Function or closure reference.
* `class` / `struct`: Custom object and data structure definitions.
* `ptr`: Raw memory pointer (for use in `unsafe` blocks).

## Keywords

Sulfur++ features a clean set of reserved keywords:
`var`, `const`, `fn`, `return`, `if`, `else`, `while`, `for`, `in`, `break`, `continue`, `class`, `struct`, `new`, `null`, `true`, `false`, `import`, `as`, `export`, `expose`, `unsafe`, `throw`.

## Syntax Examples

### Variables and Operations
```sfpp
import std/io as io;

var x = 10;
const pi = 3.14159;

// Complex math
import std/math as math;
var c = math.complex(3, 4);
io.Terminal.Out << "Absolute value of c: " << math.abs(c) << "\n";
```

### Functions and Control Flow
```sfpp
fn calculate(a, b) {
    if (a > b) {
        return a - b;
    } else {
        return a + b;
    }
}

for (i = 0; i < 5; i = i + 1) {
    io.Terminal.Out << calculate(i, 3) << "\n";
}
```

### Matrices
```sfpp
import std/matrix as mat;

var A = [
    [1, 2],
    [3, 4]
];
var B = mat.m_Eye(2);

var C = mat.m_Mul(A, B); // Native C++ matrix multiplication
```

## Error Diagnostics System

Sulfur++ features a robust, categorized error and diagnostic system. Uncaught exceptions or thrown errors will display exact file lines and context.

### Error Severities
* **[E] Error**: Standard runtime exceptions. Halts execution. (Color: Red)
* **[FE] Fatal Error**: Critical failures (e.g., VM crashes, bad memory). Halts execution instantly. (Color: Magenta)
* **[W] Warning**: Non-blocking alerts for deprecations or potential logic flaws. Prints to console but allows execution to continue. (Color: Yellow)

### Built-in Error Codes
Sulfur++ uses a structured error code system instead of standard exception class names. When catching or throwing errors natively, they follow the format `[Severity_Category_Code]`:

* `[E_SYNTAX_400]`: Invalid script syntax.
* `[E_TYPE_400]`: Invalid operations between incompatible data types.
* `[E_MATH_500]`: Mathematical operations failure (e.g., division by zero).
* `[E_NATIVE_404]`: Native function or symbol not found in the builtins registry.
* `[E_IO_404]`: File or directory not found.
* `[E_NAME_404]`: Undefined variable or identifier used.
* `[W_DEPRECATED_101]`: Use of an outdated function or feature.

## CLI Tools

### combust (Execution Engine)
Executes Sulfur++ scripts or starts the interactive REPL.
```bash
combust main.sfpp
combust main.sfpp --watch  # Auto-reload on save
combust main.sfpp --debug  # Print memory and trace logs
```

### fuse (Package Manager)
Manages dependencies and Sulfur++ projects.
```bash
fuse init
fuse add <user>/<package>
fuse run start
```

### ignitor (Embedded Deployment)
Flashes scripts onto embedded IoT targets.
```bash
ignitor init
ignitor flash
ignitor flash --bin
```

## Standard Library (std/)

* `std/io`: File system and terminal I/O.
* `std/sys`: System hardware, network, and OS process management.
* `std/builtin`: Core language builtins, time, and math utilities.
* `std/collections`: Utilities for lists, dicts, and sets.
* `std/math`: Advanced mathematical operations and complex constructors.
* `std/matrix`: Linear algebra engine.
* `std/string`: String manipulation.

## Building from Source

Sulfur++ uses CMake. To build the toolchain:

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Contributing
Contributions are welcome. Please ensure that all changes include corresponding test cases and run the project's build and test suite before submitting a PR.
