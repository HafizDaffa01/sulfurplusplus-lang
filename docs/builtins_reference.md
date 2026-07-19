# Built-in Functions Reference

Complete reference of all native functions registered in `Interpreter::registerBuiltins()` (`src/core/interpreter.cpp`).

All builtins are available globally without import (injected via `injectBuiltinsIntoGlobal()`).

---

## Input / Output

### `input(prompt?) -> str`
```sfpp
var name = input("Enter name: ");  // prints prompt, reads line
var line = input();                 // reads line without prompt
```
**Native:** `input`  
**Params:** `args[0]?` — prompt string (printed before reading)  
**Returns:** Input line as string (without trailing newline)

### `read() -> str`
```sfpp
var line = read();  // alias for input()
```
**Native:** `read`  
**Returns:** Line from stdin

---

## Time / Delays

### `delay(ms) -> null`
```sfpp
delay(1000);        // sleep 1 second
delay(500.5);       // fractional milliseconds
```
**Native:** `delay`  
**Params:** `ms` (float_64 or int_64)  
**Returns:** `null`

### `delayMilliseconds(ms) -> null`
```sfpp
delayMilliseconds(100);
```
**Native:** `delayMilliseconds`  
**Alias for:** `delay`

### `delayMicroseconds(us) -> null`
```sfpp
delayMicroseconds(500);  // 0.5ms
```
**Native:** `delayMicroseconds`  
**Params:** `us` (float_64 or int_64)

### `now() -> int_64`
```sfpp
var start = now();
// ... work ...
var elapsed = now() - start;  // microseconds
```
**Native:** `now`  
**Returns:** Microseconds since epoch (high-resolution clock)

---

## Type Introspection & Conversion

### `typeOf(value) -> str`
```sfpp
typeOf(42)           // "int_64"
typeOf(3.14)         // "float_64"
typeOf("hello")      // "str"
typeOf([1,2,3])      // "list"
typeOf({a:1})        // "dict"
typeOf(fn(){})       // "fn"
typeOf(null)         // "null"
typeOf(true)         // "bool"
```
**Native:** `typeOf`  
**Returns:** Type name string

### `type(value) -> str`
```sfpp
type(42)  // "int_64"
```
**Native:** `type`  
**Alias for:** `typeOf`

### `toStr(value) -> str`
```sfpp
toStr(42)           // "42"
toStr(3.14)         // "3.14"
toStr(true)         // "true"
toStr([1,2,3])      // "[1, 2, 3]"
toStr({a:1})        // "{a: 1}"
toStr(null)         // "null"
```
**Native:** `toStr`  
**Returns:** String representation

### `toInt(value) -> int_64`
```sfpp
toInt(3.14)         // 3
toInt("42")         // 42
toInt("abc")        // 0 (parse failure)
toInt(true)         // 1
toInt(false)        // 0
```
**Native:** `toInt`  
**Returns:** Integer conversion (0 on parse failure)

### `toFloat(value) -> float_64`
```sfpp
toFloat(42)         // 42.0
toFloat("3.14")     // 3.14
toFloat("abc")      // 0.0
toFloat(true)       // 1.0
```
**Native:** `toFloat`  
**Returns:** Float conversion (0.0 on parse failure)

### `toBool(value) -> bool`
```sfpp
toBool(0)           // false
toBool(1)           // true
toBool("")          // false
toBool("text")      // true
toBool([])          // false
toBool([1])         // true
toBool(null)        // false
```
**Native:** `toBool`  
**Returns:** Truthiness as boolean

---

## Collection Operations

### `len(value) -> int_64`
```sfpp
len("hello")        // 5
len([1,2,3])        // 3
len({a:1, b:2})     // 2
len(set{1,2,3})     // 3
len(42)             // 0 (non-collection)
```
**Native:** `len`  
**Returns:** Length/size or 0

### `range(end) -> list`
```sfpp
range(5)            // [0, 1, 2, 3, 4]
```
**Native:** `range`  
**Params:** `end` (exclusive)

### `range(start, end) -> list`
```sfpp
range(2, 6)         // [2, 3, 4, 5]
```

### `range(start, end, step) -> list`
```sfpp
range(0, 10, 2)     // [0, 2, 4, 6, 8]
range(10, 0, -1)    // [10, 9, 8, 7, 6, 5, 4, 3, 2, 1]
```
**Native:** `range`  
**Params:** `start`, `end` (exclusive), `step` (default 1, cannot be 0)

---

## Process Control

### `exit(code?) -> null`
```sfpp
exit(0);      // success
exit(1);      // error
exit();       // exit(0)
```
**Native:** `exit`  
**Params:** `code` (int_64, default 0)  
**Effect:** Immediate process termination via `std::exit()`

### `assert(condition, message?) -> null`
```sfpp
assert(x > 0);
assert(x > 0, "x must be positive");
```
**Native:** `assert`  
**Throws:** `RuntimeError` with message if condition is falsy

---

## Character Encoding

### `ord(str) -> int_64`
```sfpp
ord("A")      // 65
ord("😀")     // 128512 (first char only)
ord("")       // 0
```
**Native:** `ord`  
**Returns:** Unicode code point of first character

### `chr(code) -> str`
```sfpp
chr(65)       // "A"
chr(128512)   // "😀"
chr(-1)       // "" (invalid)
```
**Native:** `chr`  
**Returns:** Single-character string

---

## Math Functions

### Constants (pre-defined in builtins registry)
| Name | Value |
|------|-------|
| `PI` | 3.141592653589793 |
| `E` | 2.718281828459045 |
| `TAU` | 6.283185307179586 |
| `PHI` | 1.618033988749895 |
| `INF` | ∞ |
| `NEG_INF` | -∞ |
| `NAN` | NaN |

### `abs(x) -> number`
```sfpp
abs(-5)         // 5 (int_64)
abs(-3.14)      // 3.14 (float_64)
abs(complex(3,4)) // 5.0 (float_64 - magnitude)
```
**Native:** `abs`  
**Supports:** int, float, complex (returns magnitude as float)

### `sqrt(x) -> number`
```sfpp
sqrt(16)        // 4.0
sqrt(-4)        // complex(0, 2)  (returns complex for negative)
sqrt(complex(3,4)) // complex(2, 1)
```
**Native:** `sqrt`  
**Returns:** float for non-negative, complex for negative

### `pow(base, exp) -> number`
```sfpp
pow(2, 3)           // 8.0
pow(2, 0.5)         // 1.414...
pow(complex(1,1), 2) // complex(0, 2)
```
**Native:** `pow`  
**Supports:** int/float/complex for both args

### `floor(x) -> int_64`
```sfpp
floor(3.7)    // 3
floor(-3.7)   // -4
```
**Native:** `floor`

### `ceil(x) -> int_64`
```sfpp
ceil(3.2)     // 4
ceil(-3.2)    // -3
```
**Native:** `ceil`

### `round(x) -> int_64`
```sfpp
round(3.5)    // 4
round(3.4)    // 3
round(-3.5)   // -3 (away from zero)
```
**Native:** `round`

### `max(...values) -> value`
```sfpp
max(1, 5, 3)      // 5
max(1.5, 2.7)     // 2.7
max()             // null
```
**Native:** `max`  
**Returns:** Maximum value (compares as float)

### `min(...values) -> value`
```sfpp
min(1, 5, 3)      // 1
```
**Native:** `min`  
**Returns:** Minimum value

---

## Trigonometry

All trig functions accept `int_64`, `float_64`, or `complex_128`. Returns `float_64` for real input, `complex_128` for complex input.

| Function | Description |
|----------|-------------|
| `sin(x)` | Sine |
| `cos(x)` | Cosine |
| `tan(x)` | Tangent |
| `asin(x)` | Arc sine |
| `acos(x)` | Arc cosine |
| `atan(x)` | Arc tangent |
| `atan2(y, x)` | Arc tangent of y/x (quadrant-aware) |

```sfpp
sin(PI/2)       // 1.0
cos(0)          // 1.0
atan2(1, 1)     // 0.785... (PI/4)
```

---

## Logarithms

| Function | Description |
|----------|-------------|
| `log(x)` | Natural log (ln) |
| `log2(x)` | Base-2 log |
| `log10(x)` | Base-10 log |

```sfpp
log(E)        // 1.0
log2(8)       // 3.0
log10(1000)   // 3.0
log(-1)       // complex(0, PI)  (complex result for negative)
```

---

## Hyperbolic

| Function | Description |
|----------|-------------|
| `sinh(x)` | Hyperbolic sine |
| `cosh(x)` | Hyperbolic cosine |
| `tanh(x)` | Hyperbolic tangent |
| `asinh(x)` | Inverse hyperbolic sine |
| `acosh(x)` | Inverse hyperbolic cosine (x ≥ 1) |
| `atanh(x)` | Inverse hyperbolic tangent (|x| < 1) |

---

## Exponential & Power

| Function | Description |
|----------|-------------|
| `exp(x)` | e^x |
| `cbrt(x)` | Cube root |
| `hypot(x, y)` | √(x² + y²) |

```sfpp
exp(1)        // E
cbrt(27)      // 3.0
hypot(3, 4)   // 5.0
```

---

## Special Functions

| Function | Description |
|----------|-------------|
| `erf(x)` | Error function |
| `erfc(x)` | Complementary error function (1 - erf) |
| `tgamma(x)` | Gamma function (Γ(x)) |
| `lgamma(x)` | Log gamma function |

---

## Complex Numbers

### `complex(real, imag?) -> complex_128`
```sfpp
complex(3, 4)      // 3+4i
complex(5)         // 5+0i
```
**Native:** `complex`  
**Params:** `real` (default 0), `imag` (default 0)

### `real(c) -> float_64`
```sfpp
real(3+4i)    // 3.0
real(5.0)     // 5.0
```
**Native:** `real`

### `imag(c) -> float_64`
```sfpp
imag(3+4i)    // 4.0
imag(5.0)     // 0.0
```
**Native:** `imag`

### `conj(c) -> complex_128`
```sfpp
conj(3+4i)    // 3-4i
conj(5.0)     // 5.0
```
**Native:** `conj`  
**Returns:** Complex conjugate

### `arg(c) -> float_64`
```sfpp
arg(1+1i)     // 0.785... (PI/4)
arg(-1)       // 3.14159... (PI)
```
**Native:** `arg`  
**Returns:** Phase angle (radians)

---

## Matrix Operations

### `matrix_add(a, b) -> matrix`
```sfpp
var A = [[1,2],[3,4]];
var B = [[5,6],[7,8]];
matrix_add(A, B)  // [[6,8],[10,12]]
```
**Native:** `matrix_add`  
**Requires:** Same dimensions, 2D lists of numbers

### `matrix_mul(a, b) -> matrix`
```sfpp
// Matrix multiplication
matrix_mul([[1,2],[3,4]], [[5,6],[7,8]])  // [[19,22],[43,50]]

// Scalar multiplication
matrix_mul(2, [[1,2],[3,4]])  // [[2,4],[6,8]]
matrix_mul([[1,2],[3,4]], 2)  // [[2,4],[6,8]]
```
**Native:** `matrix_mul`  
**Supports:** Matrix×Matrix, Scalar×Matrix, Matrix×Scalar

### `matrix_transpose(m) -> matrix`
```sfpp
matrix_transpose([[1,2,3],[4,5,6]])  // [[1,4],[2,5],[3,6]]
```
**Native:** `matrix_transpose`

### `matrix_scale(m, scalar) -> matrix`
```sfpp
matrix_scale([[1,2],[3,4]], 2)  // [[2,4],[6,8]]
```
**Native:** `matrix_scale`

### `matrix_eye(n) -> matrix`
```sfpp
matrix_eye(3)  // [[1,0,0],[0,1,0],[0,0,1]]
```
**Native:** `matrix_eye`  
**Returns:** n×n identity matrix

---

## String Native Functions

These are also exposed via `std/string` module.

| Native | Module Alias | Description |
|--------|--------------|-------------|
| `str_trim` | `trim` | Trim whitespace |
| `str_trimleft` | `trimLeft` | Trim left |
| `str_trimright` | `trimRight` | Trim right |
| `str_upper` | `toUpper` | Uppercase |
| `str_lower` | `toLower` | Lowercase |
| `str_startswith` | `startsWith` | Prefix check |
| `str_endswith` | `endsWith` | Suffix check |
| `str_contains` | `contains` | Substring check |
| `str_replace` | `replace` | Replace all |
| `str_split` | `split` | Split by delimiter |
| `str_join` | `join` | Join list with delimiter |
| `str_repeat` | `repeat` | Repeat string N times |
| `str_indexof` | `indexOf` | Find substring |
| `str_slice` | `slice` | Substring |
| `str_padleft` | `padLeft` | Pad left |
| `str_padright` | `padRight` | Pad right |
| `str_reverse` | `reverse` | Reverse string |
| `str_isdigit` | `isDigit` | All digits? |
| `str_isalpha` | `isAlpha` | All letters? |
| `str_isalphanum` | `isAlphaNum` | All alphanumeric? |

```sfpp
// All take string as first arg (or operate on method-style)
str_upper("hello")        // "HELLO"
str_split("a,b,c", ",")   // ["a", "b", "c"]
str_join(["a","b"], "-")  // "a-b"
str_slice("hello", 1, 4)  // "ell"
```

---

## File I/O Native Functions

Exposed via `std/io` module.

| Native | Module Alias | Description |
|--------|--------------|-------------|
| `io_readfile` | `readFile` | Read entire file |
| `io_writefile` | `writeFile` | Write file (overwrite) |
| `io_appendfile` | `appendFile` | Append to file |
| `io_fileexists` | `fileExists` | Check existence |
| `io_deletefile` | `deleteFile` | Delete file |
| `io_readlines` | `readLines` | Read lines as list |
| `io_writelines` | `writeLines` | Write list as lines |
| `io_cwd` | `cwd` | Current working directory |
| `io_listdir` | `listDir` | List directory entries |
| `io_isdir` | `isDir` | Is directory? |
| `io_isfile` | `isFile` | Is regular file? |
| `io_mkdir` | `mkdir` | Create directory |

```sfpp
io_readfile("config.json")        // string content
io_writefile("out.txt", "hello")  // null
io_fileexists("data.txt")         // bool
io_listdir(".")                   // list of names
io_mkdir("newdir")                // null
```

---

## Collections Native Functions

Exposed via `std/collections` module.

| Native | Module Alias | Description |
|--------|--------------|-------------|
| `col_map` | `map` | Transform list |
| `col_filter` | `filter` | Filter list |
| `col_reduce` | `reduce` | Reduce list |
| `col_any` | `any` | Any match? |
| `col_all` | `all` | All match? |
| `col_find` | `find` | First match |
| `col_findindex` | `findIndex` | Index of match |
| `col_flatten` | `flatten` | Flatten nested |
| `col_unique` | `unique` | Remove duplicates |
| `col_sort` | `sort` | Sort list |
| `col_sortby` | `sortBy` | Sort by key fn |
| `col_groupby` | `groupBy` | Group by key fn |
| `col_zip` | `zip` | Zip two lists |
| `col_take` | `take` | First N elements |
| `col_drop` | `drop` | Skip first N |
| `col_count` | `count` | Count matches |
| `col_sum` | `sum` | Sum numbers |
| `col_min` | `min` | Minimum |
| `col_max` | `max` | Maximum |
| `col_keys` | `keys` | Dict keys |
| `col_values` | `values` | Dict values |
| `col_entries` | `entries` | Dict [k,v] pairs |
| `col_merge` | `merge` | Merge dicts |
| `col_haskey` | `hasKey` | Dict has key? |

```sfpp
col_map([1,2,3], fn(x){return x*2;})      // [2,4,6]
col_filter([1,2,3,4], fn(x){return x>2;}) // [3,4]
col_reduce([1,2,3], fn(a,b){return a+b;}, 0) // 6
col_sort([3,1,2])                          // [1,2,3]
col_keys({a:1, b:2})                       // ["a", "b"]
```

---

## Introspection: `get(obj, prop?)`

### `get(obj) -> dict`
```sfpp
get(obj)  // {OBJ_NAME: "...", GET_ITEM: [...], PROPS: [...]}
```

### `get(obj, propName) -> value`
```sfpp
get(obj, "OBJ_NAME")  // string
get(obj, "GET_ITEM")  // list of items
get(obj, "PROPS")     // list of property names
```

### `get(obj, propList) -> dict`
```sfpp
get(obj, ["OBJ_NAME", "PROPS"])  // {OBJ_NAME: "...", PROPS: [...]}
```

**Property Values:**

| Property | Class Instance | Class Def | Struct Instance | Struct Def | Function | String | List | Dict | Set |
|----------|---------------|-----------|----------------|------------|----------|--------|------|------|-----|
| `OBJ_NAME` | class name | class name | struct name | struct name | fn name | "str" | "list" | "dict" | "set" |
| `GET_ITEM` | field list | method list | field list | field list | — | chars | elements | [k,v] pairs | elements |
| `PROPS` | field names | method names | field names | field names | — | methods | methods | keys | — |

---

## Terminal / TIO

### `Terminal` (dict)
```sfpp
Terminal.Out    // "<Terminal.Out>" (stream marker)
Terminal.Warn   // "<Terminal.Warn>"
Terminal.Err    // "<Terminal.Err>"
Terminal.In     // {__stream__: "<Terminal.In>"}
Terminal.EOL    // "\n"
Terminal.Return // "<Terminal.Return>"
```

Used with stream operator:
```sfpp
Terminal.Out << "Hello" << Terminal.EOL;
Terminal.Err << "Error!" << Terminal.EOL;
```

### `TIO` (dict) — Error/Warning Builders

| Function | Signature | Returns |
|----------|-----------|---------|
| `TIO.E` | `E(msg?) -> errorDict` | `{__type__:"TIO_Error", severity:"E", message:...}` |
| `TIO.FE` | `FE(msg?) -> errorDict` | Fatal error dict |
| `TIO.W` | `W(msg?) -> warningDict` | Warning dict |

**Builder Functions** (chainable):
```sfpp
TIO.withCategory(errDict, "NETWORK")    // adds category
TIO.withCode(errDict, "ERR_001")        // adds code
TIO.withHint(errDict, "Check connection") // adds hint
TIO.withContext(errDict, {port: 8080})  // adds context
```

```sfpp
throw TIO.E("Connection failed")
    |> TIO.withCategory("NETWORK")
    |> TIO.withCode("CONN_001")
    |> TIO.withHint("Verify server is running");
```

---

## Scientific Constants (Pre-defined)

| Name | Value | Description |
|------|-------|-------------|
| `SC_C` | 299792458.0 | Speed of light (m/s) |
| `SC_G` | 6.67430e-11 | Gravitational constant |
| `SC_H` | 6.62607015e-34 | Planck constant (J·s) |
| `SC_K` | 1.380649e-23 | Boltzmann constant (J/K) |
| `SC_NA` | 6.02214076e23 | Avogadro number (mol⁻¹) |
| `SC_R` | 8.314462618 | Gas constant (J/mol·K) |

---

## Version Constants

| Name | Type | Example |
|------|------|---------|
| `RUNTIME_VERSION` | str | "0.1.0" |
| `SULFUR_VERSION` | str | "0.1.0" |
| `COMBUST_VERSION` | str | "0.1.0" |
| `BUILD_MODE` | str | "debug" / "release" |
| `DEBUG_MODE` | bool | false |

---

## Property Constants (String Tokens)

For use with `get()`:
- `PROP_FROZEN`, `PROP_READONLY`, `PROP_IMMUTABLE`
- `PROP_REACTIVE`, `PROP_SERIALIZABLE`, `PROP_UNSAFE`
- `PROP_VOLATILE`, `PROP_DEBUG`, `PROP_HIDDEN`
- `PROP_PRIVATE`, `PROP_PUBLIC`, `PROP_FINAL`
- `PROP_CONST`, `PROP_STATIC`, `PROP_SYNC`
- `PROP_ASYNC`, `PROP_LAZY`, `PROP_CACHED`
- `PROP_TEMP`, `PROP_NATIVE`, `PROP_PROTECTED`
- `PROP_INTERNAL`, `PROP_EXPERIMENTAL`, `PROP_DEPRECATED`
- `PROP_LOCKED`, `PROP_OBSERVABLE`
- `OBJ_NAME`, `GET_ITEM`, `PROPS`

---

## FFI Native Functions

Available when `std/ffi` is imported (or called directly):

| Native | Description |
|--------|-------------|
| `ffi_dlopen(path)` | Load shared library → raw pointer |
| `ffi_dlsym(handle, name)` | Get symbol address → raw pointer |
| `ffi_dlclose(handle)` | Close library |
| `ffi_call(fnPtr, argsList, retType)` | Call C function |
| `ffi_mem_read(ptr, offset)` | Read int64 at offset |
| `ffi_mem_write(ptr, offset, value)` | Write int64 at offset |
| `ffi_str_read(ptr)` | Read C string |
| `ffi_str_write(ptr, offset, str)` | Write C string |
| `ffi_sizeof(typeName)` | Size of C type |

**RetType strings for `ffi_call`:** `"void"`, `"int"`, `"float"`, `"ptr"`, `"str"`

---

## Hardware / Sys Native Functions

Available via `std/sys` or `std/hal`:

| Native | Module | Description |
|--------|--------|-------------|
| `iot_gpio_mode` | hal/sys | Set GPIO mode |
| `iot_gpio_write` | hal/sys | Write GPIO |
| `iot_gpio_read` | hal/sys | Read GPIO |
| `sys_memory_alloc` | sys | Allocate memory |
| `sys_memory_free` | sys | Free memory |
| `sys_memory_read` | sys | Read memory |
| `sys_memory_write` | sys | Write memory |
| `sys_wifi_connect` | sys | Connect WiFi |
| `sys_process_exec` | sys | Execute shell command |

---

## Error Codes Reference

| Code | Severity | Category | Meaning |
|------|----------|----------|---------|
| `E_SYNTAX_400` | Error | Syntax | Parse error |
| `E_TYPE_400` | Error | Type | Type mismatch |
| `E_MATH_500` | Error | Math | Math error (div by zero, etc) |
| `E_NATIVE_404` | Error | Native | Builtin not found |
| `E_IO_404` | Error | IO | File not found |
| `E_NAME_404` | Error | Name | Undefined identifier |
| `W_DEPRECATED_101` | Warning | Deprecated | Feature deprecated |
| `FE_VM_500` | Fatal | VM | Internal VM error |

---

## Function Count Summary

| Category | Count |
|----------|-------|
| Math (trig, log, hyperbolic, special) | 30+ |
| Matrix | 5 |
| Complex | 5 |
| String | 18 |
| File I/O | 12 |
| Collections | 22 |
| Type/Conversion | 6 |
| Time/Delay | 4 |
| Process | 2 |
| Introspection | 1 (`get`) |
| Terminal/TIO | 8 |
| FFI | 9 |
| Hardware/Sys | 8 |
| **Total** | **~130+** |