# Sulfur++ Language

Sulfur++ is a lightweight, C++-backed scripting language designed for both desktop applications and embedded/IoT systems. It provides robust system access, memory management, and a rich standard library while remaining easy to use.

## Key Features

- **Dynamic Typing**: Flexible variable handling.
- **Native System Access**: Low-level memory, hardware (GPIO), and OS bindings via an `unsafe` block mechanism.
- **Robust Standard Library**: Includes modules for I/O, system utilities, math, and collections.
- **Modern Syntax**: Supports classes, structs, closures, lambda expressions, and pipeline operators (`|`).
- **Error Handling**: Advanced error management via the `TIO` (Typed I/O) error system.

## Quick Start

### Building
Sulfur++ uses CMake. To build the interpreter:

```bash
mkdir build
cd build
cmake ..
make  # or use your IDE to build the generated solution
```

### Running a Script
Run your `.sfpp` scripts using the `combust` interpreter:

```bash
combust path/to/your/script.sfpp
```

## Package Management
Use `fuse` to manage packages:

```bash
fuse install <package_name>
```

## Example: Fibonacci

```sfpp
fn fib(n) {
    var a = 0;
    var b = 1;
    for (var i = 0; i < n; i = i + 1) {
        var temp = a;
        a = b;
        b = temp + b;
    }
    return a;
}

var start = now();
var result = fib(50);
var end = now();

Terminal.Out << "Fibonacci(50) = " << result << Terminal.EOL;
Terminal.Out << "Time taken: " << (end - start) / 1000000.0 << " seconds" << Terminal.EOL;
```

## Standard Library (`std/`)

- `std/io`: File system and terminal I/O.
- `std/sys`: System hardware, network, and OS process management.
- `std/builtin`: Core language builtins, time, and math utilities.
- `std/collections`: Utilities for lists, dicts, and sets.
- `std/math`: Advanced mathematical operations.
- `std/string`: String manipulation.

## Contributing
Contributions are welcome. Please ensure that all changes include corresponding test cases and run the project's build and test suite before submitting a PR.
