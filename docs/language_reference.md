# Sulfur++ Language Reference

## Overview

Sulfur++ is a dynamically-typed, C++-backed scripting language designed for desktop, embedded, and IoT applications. It features modern syntax, native system access, and a rich standard library.

**Motto**: *Write Easy, Perform Fast*

---

## Lexical Structure

### Comments

```sfpp
// Single-line comment

/*
 * Multi-line comment
 * Can be nested
 */
```

### Identifiers

- Start with letter or underscore: `[a-zA-Z_]`
- Followed by letters, digits, underscores: `[a-zA-Z0-9_]*`
- Case-sensitive
- Cannot be reserved keywords

### Reserved Keywords

| Category | Keywords |
|----------|----------|
| **Variables** | `let`, `var`, `auto` |
| **Functions** | `fn`, `return` |
| **Control Flow** | `if`, `else`, `while`, `for`, `in`, `break`, `continue` |
| **Types** | `class`, `struct`, `interface`, `new`, `delete` |
| **Literals** | `null`, `true`, `false` |
| **Modules** | `import`, `as`, `export`, `expose`, `overwrite` |
| **Special** | `this`, `unsafe`, `defer`, `try`, `catch`, `throw`, `match` |
| **Pointers** | `ptr`, `ref` |
| **Primitive Types** | `int_8`, `int_16`, `int_32`, `int_64`, `uint_8`, `uint_16`, `uint_32`, `uint_64`, `float_32`, `float_64`, `bool`, `char`, `str`, `void`, `list`, `set`, `dict`, `matrix` |

### Literals

| Type | Syntax | Examples |
|------|--------|----------|
| **Integer** | Decimal, hex (`0x`), underscores allowed | `42`, `0xFF`, `1_000_000` |
| **Float** | Decimal, scientific, time suffixes | `3.14`, `1e-5`, `500ms`, `2s`, `1m`, `1h`, `1d` |
| **Bool** | `true`, `false` | `true`, `false` |
| **Null** | `null` | `null` |
| **String** | Double quotes, escape sequences | `"hello"`, `"line1\nline2"` |
| **Char** | Single quotes | `'a'`, `'\n'` |
| **PS-String** | Template string with interpolation | `ps"Value: {x}, Upper: {y:upper}"` |
| **List** | `[elem1, elem2, ...]` | `[1, 2, 3]`, `["a", "b"]` |
| **Dict** | `{key: val, ...}` | `{"name": "test", "age": 25}` |

### Time Suffixes (for numeric literals)

| Suffix | Meaning | Multiplier |
|--------|---------|------------|
| `ms` | milliseconds | 1.0 |
| `us` | microseconds | 0.001 |
| `s` | seconds | 1000.0 |
| `m` | minutes | 60000.0 |
| `h` | hours | 3600000.0 |
| `d` | days | 86400000.0 |

---

## Type System

### Primitive Types

| Type | Description | Size |
|------|-------------|------|
| `null` | Empty/undefined value | - |
| `bool` | Boolean | 1 byte |
| `int_8` / `int_16` / `int_32` / `int_64` | Signed integers | 1/2/4/8 bytes |
| `uint_8` / `uint_16` / `uint_32` / `uint_64` | Unsigned integers | 1/2/4/8 bytes |
| `float_32` / `float_64` | Floating point | 4/8 bytes |
| `complex_128` | Complex number (real + imag) | 16 bytes |
| `char` | Single ASCII character | 1 byte |
| `str` | UTF-8 string | variable |

### Composite Types

| Type | Description |
|------|-------------|
| `list` | Dynamic array, can hold mixed types |
| `dict` | Hash map with string keys |
| `set` | Unique unordered collection |
| `fn` | Function/closure reference |
| `class` | Reference-type object |
| `struct` | Value-type data structure |
| `ptr` | Raw memory pointer (unsafe) |

### Type Annotations

```sfpp
let x: int_64 = 42;
var y: str = "hello";
fn add(a: int_64, b: int_64): int_64 { return a + b; }
```

### Generics

```sfpp
list<int_64>
dict<str, float_64>
list<list<float_64>>  // matrix
```

### Nullable Types

```sfpp
str?        // nullable string
int_64?     // nullable integer
fn()?       // nullable function
```

---

## Variables

### Declaration Keywords

| Keyword | Mutability | Type Inference | Use Case |
|---------|------------|----------------|----------|
| `let` | Immutable | Optional | Constants, immutable bindings |
| `var` | Mutable | Optional | Variables that change |
| `auto` | Mutable | Required | Dynamic typing, type changes allowed |

```sfpp
let PI = 3.14159;              // immutable, inferred float_64
let MAX: int_64 = 100;         // immutable, explicit type
var counter = 0;               // mutable, inferred int_64
auto dynamic = "text";         // mutable, dynamic type
dynamic = 42;                  // now holds int_64
dynamic = [1, 2, 3];           // now holds list
```

### Destructuring

```sfpp
let [a, b, c] = [1, 2, 3];
let {name, age} = {"name": "John", "age": 30};
```

---

## Control Flow

### If / Else

```sfpp
if (condition) {
    // then branch
} else if (otherCondition) {
    // else-if branch
} else {
    // else branch
}
```

**Requirement**: Parentheses and braces are mandatory. No single-line bodies.

### While Loop

```sfpp
while (condition) {
    // body
}
```

### For Loop (C-style)

```sfpp
for (init; condition; increment) {
    // body
}

// Example
for (i = 0; i < 10; i = i + 1) {
    print(i);
}
```

### For-In Loop (Range/Iterable)

```sfpp
for (item in iterable) {
    // body
}

// Range iteration
for (i in range(10)) { ... }
for (i in range(0, 10)) { ... }
for (i in range(0, 10, 2)) { ... }
```

### Break / Continue

```sfpp
for (i in range(100)) {
    if (i == 5) { continue; }
    if (i == 10) { break; }
}
```

### Match Expression

```sfpp
match (value) {
    pattern1 => { body1 }
    pattern2 => { body2 }
    _ => { defaultBody }  // wildcard/default case
}

// Example
match (status) {
    200 => { print("OK"); }
    404 => { print("Not Found"); }
    500 => { print("Server Error"); }
    _ => { print("Unknown"); }
}
```

Patterns can be:
- Literal values: `42`, `"hello"`, `true`
- Variable bindings: `x => { ... }` (captures value)
- Wildcard: `_` (matches anything, no binding)

---

## Functions

### Function Declaration

```sfpp
fn name(param1: type1, param2: type2): returnType {
    // body
    return value;
}
```

### Lambda / Anonymous Functions

```sfpp
// Full syntax
fn (a: int, b: int): int {
    return a + b;
}

// Short syntax (single expression, implicit return)
(a, b) => a + b
```

### Function as Values

```sfpp
var add = fn(a, b) { return a + b; };
var result = add(3, 4);  // 7

// Passing functions
var ops = [add, fn(a,b){return a-b;}];
```

### Pipeline Operator (`->`)

```sfpp
value -> function1 -> function2 -> function3

// Equivalent to:
function3(function2(function1(value)))

// With arguments:
"hello" -> str_upper -> str_reverse
[1,2,3] -> (x) => x * 2 -> col_sum
```

### Method Syntax

```sfpp
// obj.method(args)
list.push(1, 2, 3)
dict.has("key")
str.contains("sub")
```

---

## Classes & Structs

### Class Declaration

```sfpp
class ClassName : Interface1, Interface2 {
    // Fields (use bare name, no var/let)
    fieldName: type = defaultValue;
    
    // Lifecycle order declarations
    +1>init;
    +2>setup;
    ~1>cleanup;
    
    // Methods
    fn methodName(param: type): returnType {
        // body
    }
    
    // Constructor (init method)
    fn init(param: type) {
        this.fieldName = param;
    }
}
```

### Struct Declaration

```sfpp
struct StructName {
    field1: type1;
    field2: type2;
    // No methods, no lifecycle, value semantics
}
```

### Interface Declaration

```sfpp
interface InterfaceName {
    fn methodName(param: type): returnType;
    fn otherMethod(): void;
}
```

### Instantiation

```sfpp
// Class (reference semantics)
var obj = new ClassName(args);

// Struct (value semantics)
var s = StructName { field1: val1, field2: val2 };
```

### Field Access

```sfpp
obj.field           // direct access
obj?.field          // optional chaining (returns null if obj is null)
obj.method()        // method call
obj?.method()       // optional chaining method call
```

### Constructor/Destructor Order

Methods annotated with `+N>` run in ascending order during construction.
Methods annotated with `~N>` run in descending order during destruction.

```sfpp
class Example {
    +1>init;    // runs first
    +2>setup;   // runs second
    ~2>teardown; // runs first on destruction
    ~1>cleanup;  // runs second on destruction
}
```

---

## Error Handling

### Try / Catch / Finally

```sfpp
try {
    // risky code
    riskyOperation();
} catch (e) {
    // handle error
    print("Error: " + e);
} finally {
    // cleanup (optional)
    cleanup();
}
```

### Throw

```sfpp
throw "Something went wrong";
throw TIO.E("Custom error message")
    |> TIO.withCode("ERR_001")
    |> TIO.withCategory("VALIDATION");
```

### Error Object Structure

Errors are dicts with optional fields:
```sfpp
{
    "__type__": "TIO_Error",
    "severity": "E" | "FE" | "W",  // Error, Fatal Error, Warning
    "message": "description",
    "code": "ERR_CODE",             // optional
    "category": "CATEGORY",         // optional
    "hint": "suggestion",           // optional
    "context": any                  // optional
}
```

---

## Modules & Imports

### Import Statement

```sfpp
// Standard library
import std/io as io;
import std/math as math;

// Package import (future)
import @org/package as pkg;

// With flags
import std/alias --use=[NOLIBNAME];
```

### Export Statement

```sfpp
export this as std/mymodule;
```

### Expose Statement (Module → Native Binding)

```sfpp
expose "native_function_name" as aliasName;
```

### Overwrite Statement

```sfpp
overwrite nativeFunction = replacementFunction;
```

---

## Unsafe Blocks

```sfpp
unsafe {
    // Direct memory/hardware access
    var ptr = sys_memory_alloc(1024);
    sys_memory_write(ptr, 42);
    var val = sys_memory_read(ptr);
    sys_memory_free(ptr);
}
```

---

## Defer Statement

```sfpp
fn example() {
    var file = open("data.txt");
    defer { close(file); }  // Runs when function exits (normal or error)
    
    process(file);
    // file automatically closed here
}
```

---

## Operators

### Arithmetic

| Operator | Description |
|----------|-------------|
| `+` | Addition |
| `-` | Subtraction |
| `*` | Multiplication |
| `/` | Division |
| `%` | Modulo |
| `**` | Power |

### Comparison

| Operator | Description |
|----------|-------------|
| `==` | Equal |
| `!=` | Not equal |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less than or equal |
| `>=` | Greater than or equal |

### Logical

| Operator | Description |
|----------|-------------|
| `&&` / `and` | Logical AND |
| `\|\|` / `or` | Logical OR |
| `!` / `not` | Logical NOT |

### Bitwise

| Operator | Description |
|----------|-------------|
| `&` | Bitwise AND |
| `\|` | Bitwise OR |
| `^` | Bitwise XOR |
| `~` | Bitwise NOT |
| `<<` | Left shift |
| `>>` | Right shift |

### Assignment

| Operator | Description |
|----------|-------------|
| `=` | Assign |
| `+=` | Add and assign |
| `-=` | Subtract and assign |
| `*=` | Multiply and assign |
| `/=` | Divide and assign |

### Special

| Operator | Description |
|----------|-------------|
| `->` | Pipeline |
| `=>` | Fat arrow (match cases, lambdas) |
| `??` | Null coalescing |
| `?.` | Optional chaining |
| `?` | Ternary condition |
| `:` | Ternary else / Type annotation |
| `<<` | Stream output |
| `>>` | Stream input |
| `~` | Destructor prefix |
| `+N>` | Constructor order |
| `~N>` | Destructor order |

### Operator Precedence (highest to lowest)

1. Postfix: `()`, `[]`, `.`, `?.`, `::`, `->`
2. Unary: `!`, `-`, `~`, `&`, `*`, `+N>`, `~N>`
3. Power: `**`
4. Multiplicative: `*`, `/`, `%`
5. Additive: `+`, `-`
6. Shift: `<<`, `>>`
7. Comparison: `<`, `>`, `<=`, `>=`
8. Equality: `==`, `!=`
8. Bitwise AND: `&`
9. Bitwise XOR: `^`
10. Bitwise OR: `\|`
11. Logical AND: `&&`
12. Logical OR: `\|\|`
13. Null coalescing: `??`
14. Ternary: `? :`
15. Assignment: `=`, `+=`, `-=`, `*=`, `/=`
16. Pipeline: `->`

---

## PS-Strings (Template Strings)

```sfpp
// Basic interpolation
var name = "World";
ps"Hello, {name}!";  // "Hello, World!"

// Expression interpolation
ps"2 + 2 = {2 + 2}";  // "2 + 2 = 4"

// Format specifiers
ps"Items: {items:join(', ')}";
ps"Repeated: {text:repeat(3)}";
ps"Padded: {num:padLeft(5, '0')}";

// Escape braces
ps"Literal {{braces}}";  // "Literal {braces}"
```

---

## Built-in Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `PI` | 3.141592653589793 | π |
| `E` | 2.718281828459045 | e |
| `TAU` | 6.283185307179586 | 2π |
| `PHI` | 1.618033988749895 | Golden ratio |
| `INF` | ∞ | Positive infinity |
| `NEG_INF` | -∞ | Negative infinity |
| `NAN` | NaN | Not a number |
| `SC_C` | 299792458 | Speed of light (m/s) |
| `SC_G` | 6.67430e-11 | Gravitational constant |
| `SC_H` | 6.62607015e-34 | Planck constant |
| `SC_K` | 1.380649e-23 | Boltzmann constant |
| `SC_NA` | 6.02214076e23 | Avogadro number |
| `SC_R` | 8.314462618 | Gas constant |

---

## CLI Tool: combust

```bash
# Run script
combust script.sfpp

# REPL
combust

# Watch mode (auto-reload)
combust script.sfpp --watch

# Debug mode
combust script.sfpp --debug

# Force JIT
combust script.sfpp --jit

# Compile to executable (future)
combust --compile script.sfpp -o myapp
```

---

## File Extensions

- `.sfpp` - Sulfur++ source files
- `.sfppi` - Sulfur++ interface files (future)

---

## Shebang Support

```sfpp
#!/usr/bin/env combust
import std/io as io;
io.Terminal.Out << "Hello from script!" << "\n";
```