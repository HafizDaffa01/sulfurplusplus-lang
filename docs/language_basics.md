# Language Basics

## Variables
```sfpp
let PI = 3.14159;          // constant, immutable
var counter = 0;           // mutable variable
auto dynVar = "text";    // type inferred, can change later

dynVar = 42;               // now holds an integer
```
- `let` – immutable constant.
- `var` – mutable variable.
- `auto` – dynamically typed, type is inferred from the assigned value.

## Control Flow
```sfpp
if (counter > 10) {
    io.Terminal.Out << "big" << "\n";
} else {
    io.Terminal.Out << "small" << "\n";
}

for (i = 0; i < 5; i = i + 1) {
    io.Terminal.Out << i << "\n";
}
```
Supported statements: `if/else`, `while`, `for` (C‑style), `break`, `continue`.

## Functions
```sfpp
fn add(a, b) {
    return a + b;
}

var result = add(3, 4);
io.Terminal.Out << "3+4=" << result << "\n";
```
Functions are first‑class values; they can be passed around and stored in variables.

## Classes & Structs
```sfpp
class Point {
    var x = 0;
    var y = 0;
}

var p = Point();
p.x = 10; p.y = 20;
```
`class` provides reference semantics, while `struct` gives value semantics.

## Error Handling
```sfpp
try {
    throw "Something went wrong";
} catch (e) {
    io.Terminal.Err << "Caught: " << e << "\n";
}
```
Use `try/catch` to capture runtime errors. Errors are ordinary values.

## Tiered JIT Compilation
Sulfur++ features a Tiered JIT Compilation mechanism powered by LLVM. By default, functions execute through the tree-walking interpreter for fast startup. If a function is called 50 times (making it a "hot-spot"), the runtime will automatically JIT-compile it in the background for maximum native performance.
You can also force the engine to JIT compile all functions immediately by passing the `--jit` or `-j` flag to `combust`.

---
Continue to the next tutorial for the standard library I/O module: [stdlib_io.md](stdlib_io.md).
