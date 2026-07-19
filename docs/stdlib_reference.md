# Sulfur++ Standard Library Reference

The standard library is organized into modules under the `std/` namespace. Each module uses the `export this as std/name;` pattern and exposes native builtins via `expose "native_name" as alias;`.

## Module Import Syntax

```sfpp
import std/io as io;                    // Import with alias
import std/math as math;                // Common pattern
import std/alias --use=[NOLIBNAME];     // Import with flags
```

**Flags:**
- `--use=[NOLIBNAME]` — Expose symbols without module prefix

---

## Module Index

| Module | Description | Key Exports |
|--------|-------------|-------------|
| `std/builtin` | Core language builtins | `delay`, `typeOf`, `len`, `range`, `exit`, `assert`, `Terminal`, `TIO` |
| `std/io` | File I/O & terminal streams | `readFile`, `writeFile`, `Terminal`, `TIO` |
| `std/sys` | System/hardware access | `GPIO`, `WiFiManager`, `exec`, `Pointer` |
| `std/collections` | List/dict/set utilities | `map`, `filter`, `reduce`, `sort`, `keys`, `values` |
| `std/math` | Mathematical functions & constants | `PI`, `sin`, `cos`, `sqrt`, `complex`, `matrix_*` |
| `std/matrix` | Linear algebra | `m_Add`, `m_Mul`, `m_Transpose`, `m_Eye` |
| `std/string` | String manipulation | `trim`, `split`, `join`, `replace`, `slice` |
| `std/json` | JSON parsing/serialization | `parse`, `stringify`, `pretty`, `parseFile` |
| `std/time` | Timing & delays | `delay`, `micros`, `millis`, `seconds` |
| `std/hal` | Hardware abstraction (GPIO) | `Pin`, `createPin`, `OUTPUT`, `HIGH` |
| `std/events` | Async event loop | `setInterval`, `setTimeout`, `run` |
| `std/constants` | Runtime constants | `SECOND`, `MINUTE`, `RUNTIME_VERSION` |
| `std/runtime` | Runtime introspection | `Runtime.uptimeMillis()`, `getEngineName()` |
| `std/science` | Physical constants | `C`, `G`, `H`, `K`, `NA`, `R` |
| `std/alias` | Alias/shortcut system | `Alias.create(pattern, expansion)` |
| `std/ffi` | Foreign Function Interface | `load`, `callInt`, `callFloat`, `memRead` |

---

## std/builtin

**Source:** `src/stdlib/builtin.sfpp`

Core builtins automatically available in global scope.

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `delay` | `delay(ms: float_64)` | Sleep for milliseconds |
| `delayMilliseconds` | `delayMilliseconds(ms: float_64)` | Alias for `delay` |
| `delayMicroseconds` | `delayMicroseconds(us: float_64)` | Sleep for microseconds |
| `typeOf` | `typeOf(value) -> str` | Runtime type name |
| `type` | `type(value) -> str` | Alias for `typeOf` |
| `toStr` | `toStr(value) -> str` | Convert to string |
| `toInt` | `toInt(value) -> int_64` | Convert to integer |
| `toFloat` | `toFloat(value) -> float_64` | Convert to float |
| `toBool` | `toBool(value) -> bool` | Convert to boolean |
| `len` | `len(value) -> int_64` | Length of string/list/dict/set |
| `range` | `range(end)`, `range(start, end)`, `range(start, end, step)` | Generate integer list |
| `exit` | `exit(code: int_64)` | Exit process |
| `assert` | `assert(condition, message?)` | Throw if falsy |
| `ord` | `ord(str) -> int_64` | Character code |
| `chr` | `chr(code: int_64) -> str` | Character from code |
| `read` | `read() -> str` | Read line from stdin |
| `get` | `get(obj, prop?) -> value/dict` | Introspection: `OBJ_NAME`, `GET_ITEM`, `PROPS` |
| `now` | `now() -> int_64` | Microseconds since epoch |

### Constants

| Constant | Value |
|----------|-------|
| `PI` | 3.141592653589793 |
| `E` | 2.718281828459045 |
| `TAU` | 6.283185307179586 |
| `PHI` | 1.618033988749895 |
| `INF` | `float_64` infinity |
| `NEG_INF` | Negative infinity |
| `NAN` | Not-a-Number |

### Namespaces

**Terminal** — Stream output markers:
```sfpp
Terminal.Out << "stdout" << "\n";
Terminal.Err << "stderr" << "\n";
Terminal.Warn << "warning" << "\n";
Terminal.In  // stdin marker
Terminal.Return  // special return marker
Terminal.EOL  // "\n"
```

**TIO** — Structured error/warning builder:
```sfpp
var err = TIO.E("message");              // Error
var fe  = TIO.FE("fatal");               // Fatal error
var warn = TIO.W("warning");             // Warning

// Decorate with context
TIO.withCategory(err, "NETWORK");
TIO.withCode(err, "CONN_001");
TIO.withHint(err, "Check cable");
TIO.withContext(err, {"port": 8080});
```

---

## std/io

**Source:** `src/stdlib/io.sfpp`

File system and terminal I/O.

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `readFile` | `readFile(path: str) -> str` | Read entire file |
| `writeFile` | `writeFile(path: str, content: str)` | Write file (overwrite) |
| `appendFile` | `appendFile(path: str, content: str)` | Append to file |
| `fileExists` | `fileExists(path: str) -> bool` | Check file exists |
| `deleteFile` | `deleteFile(path: str)` | Delete file |
| `readLines` | `readLines(path: str) -> list<str>` | Read file as lines |
| `writeLines` | `writeLines(path: str, lines: list<str>)` | Write lines to file |
| `cwd` | `cwd() -> str` | Current working directory |
| `listDir` | `listDir(path?) -> list<str>` | Directory listing |
| `isDir` | `isDir(path: str) -> bool` | Is directory |
| `isFile` | `isFile(path: str) -> bool` | Is regular file |
| `mkdir` | `mkdir(path: str)` | Create directory (recursive) |

### Re-exports
- `Terminal`, `TIO` from `std/builtin`

### TIO Helpers (local aliases)
```sfpp
let io_e            = TIO.E;
let io_fe           = TIO.FE;
let io_w            = TIO.W;
let io_withCategory = TIO.withCategory;
let io_withCode     = TIO.withCode;
let io_withHint     = TIO.withHint;
let io_withContext  = TIO.withContext;
```

---

## std/sys

**Source:** `src/stdlib/sys.sfpp`

Low-level system, hardware, and OS access.

### Memory Management

| Function | Signature | Description |
|----------|-----------|-------------|
| `nativeMalloc` | `nativeMalloc(size: int_64) -> ptr` | Allocate raw memory |
| `nativeFree` | `nativeFree(ptr: ptr)` | Free raw memory |
| `nativeMemRead` | `nativeMemRead(addr: ptr) -> value` | Read from address |
| `nativeMemWrite` | `nativeMemWrite(addr: ptr, value)` | Write to address |

### Pointer Wrapper

```sfpp
class Pointer {
    address;
    +1>init;
    fn init(addr) { this.address = addr; }
    fn read() { var val = null; unsafe { val = nativeMemRead(this.address); } return val; }
    fn write(val) { unsafe { nativeMemWrite(this.address, val); } }
    fn free() { unsafe { nativeFree(this.address); } }
}
```

### GPIO (IoT/Embedded)

| Function | Signature | Description |
|----------|-----------|-------------|
| `nativeGpioMode` | `nativeGpioMode(pin: int, mode: int)` | Set pin mode (1=OUTPUT, 0=INPUT) |
| `nativeGpioWrite` | `nativeGpioWrite(pin: int, value: int)` | Write pin (1=HIGH, 0=LOW) |
| `nativeGpioRead` | `nativeGpioRead(pin: int) -> int` | Read pin |

```sfpp
class GPIO {
    pin;
    +1>init;
    fn init(pinNum) { this.pin = pinNum; }
    fn setMode(mode) { unsafe { nativeGpioMode(this.pin, mode); } }
    fn write(value) { unsafe { nativeGpioWrite(this.pin, value); } }
    fn read() { var val = 0; unsafe { val = nativeGpioRead(this.pin); } return val; }
}
```

### Network (WiFi)

| Function | Signature | Description |
|--------|-----------|-------------|
| `nativeWifiConnect` | `nativeWifiConnect(ssid: str, password: str) -> bool` | Connect to WiFi |

```sfpp
class WiFiManager {
    connected;
    +1>init;
    fn init() { this.connected = false; }
    fn connect(ssid, password) {
        var result = false;
        unsafe {
            Terminal.Out << "[sys.network] Connecting to " << ssid << "..." << Terminal.EOL;
            result = nativeWifiConnect(ssid, password);
        }
        if (result) {
            this.connected = true;
            return true;
        } else {
            return io_withHint(
                io_withCode(io_withCategory(io_e("Failed to connect to WiFi"), "SYS_NETWORK"), "WIFI_ERR_001"),
                "Ensure the SSID and password are correct, and the adapter is enabled."
            );
        }
    }
}
```

### Process Execution

| Function | Signature | Description |
|--------|-----------|-------------|
| `nativeExec` | `nativeExec(cmd: str) -> str` | Execute shell command |

```sfpp
fn exec(cmd) {
    var result = null;
    unsafe { result = nativeExec(cmd); }
    return result;
}
```

### Mocking Support

```sfpp
fn mockWifiConnect(ssid, pass) {
    Terminal.Out << "[sys.mock] Intercepted WiFi connection to " << ssid << Terminal.EOL;
    return true;
}

fn applyWifiMock() {
    overwrite nativeWifiConnect = mockWifiConnect;
    Terminal.Out << "[sys.mock] Overwritten nativeWifiConnect" << Terminal.EOL;
}
```

---

## std/collections

**Source:** `src/stdlib/collections.sfpp`

Collection utilities backed by native builtins.

### List Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `map` | `map(list, fn) -> list` | Transform each element |
| `filter` | `filter(list, fn) -> list` | Keep elements matching predicate |
| `reduce` | `reduce(list, fn, init?) -> value` | Reduce to single value |
| `any` | `any(list, fn) -> bool` | Any element matches |
| `all` | `all(list, fn) -> bool` | All elements match |
| `find` | `find(list, fn) -> value/null` | First matching element |
| `findIndex` | `findIndex(list, fn) -> int` | Index of first match (-1 if none) |
| `flatten` | `flatten(list) -> list` | Flatten nested lists |
| `unique` | `unique(list) -> list` | Remove duplicates |
| `sort` | `sort(list, fn?) -> list` | Sort (optional comparator) |
| `sortBy` | `sortBy(list, keyFn) -> list` | Sort by key function |
| `groupBy` | `groupBy(list, keyFn) -> dict` | Group elements by key |
| `zip` | `zip(list1, list2) -> list<list>` | Pair elements |
| `take` | `take(list, n) -> list` | First n elements |
| `drop` | `drop(list, n) -> list` | Skip first n elements |
| `count` | `count(list, fn?) -> int` | Count (with optional predicate) |
| `sum` | `sum(list) -> int/float` | Sum of numbers |
| `min` | `min(list) -> value` | Minimum |
| `max` | `max(list) -> value` | Maximum |

### Dict Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `keys` | `keys(dict) -> list` | All keys |
| `values` | `values(dict) -> list` | All values |
| `entries` | `entries(dict) -> list<[k,v]>` | Key-value pairs |
| `merge` | `merge(dict1, dict2) -> dict` | Merge two dicts |
| `hasKey` | `hasKey(dict, key) -> bool` | Key exists |

---

## std/math

**Source:** `src/stdlib/math.sfpp`

Mathematical constants and functions.

### Constants

| Constant | Value |
|----------|-------|
| `PI` | 3.141592653589793 |
| `E` | 2.718281828459045 |
| `TAU` | 6.283185307179586 (2π) |
| `PHI` | 1.618033988749895 (golden ratio) |
| `INF` | Infinity |
| `NEG_INF` | -Infinity |
| `NAN` | Not-a-Number |

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `complex` | `complex(real, imag?) -> complex_128` | Create complex number |
| `real` | `real(complex) -> float_64` | Real part |
| `imag` | `imag(complex) -> float_64` | Imaginary part |
| `abs` | `abs(x) -> float_64` | Absolute value / magnitude |
| `sqrt` | `sqrt(x) -> float_64/complex` | Square root |
| `pow` | `pow(base, exp) -> float_64/complex` | Power |
| `floor` | `floor(x) -> int_64` | Floor |
| `ceil` | `ceil(x) -> int_64` | Ceiling |
| `round` | `round(x) -> int_64` | Round to nearest |
| `max` | `max(...args) -> value` | Maximum |
| `min` | `min(...args) -> value` | Minimum |

### Trigonometry

| Function | Description |
|----------|-------------|
| `sin`, `cos`, `tan` | Basic trig |
| `asin`, `acos`, `atan` | Inverse trig |
| `atan2` | `atan2(y, x)` |
| `sinh`, `cosh`, `tanh` | Hyperbolic |
| `asinh`, `acosh`, `atanh` | Inverse hyperbolic |

### Exponential/Logarithmic

| Function | Description |
|----------|-------------|
| `exp` | e^x |
| `log` | Natural log |
| `log2` | Base-2 log |
| `log10` | Base-10 log |
| `cbrt` | Cube root |
| `hypot` | `hypot(x, y)` = sqrt(x²+y²) |

### Special Functions

| Function | Description |
|----------|-------------|
| `erf` | Error function |
| `erfc` | Complementary error function |
| `tgamma` | Gamma function |
| `lgamma` | Log gamma function |

### Complex Numbers

| Function | Description |
|----------|-------------|
| `complex(r, i)` | Create complex |
| `real(z)` | Real part |
| `imag(z)` | Imaginary part |
| `conj(z)` | Complex conjugate |
| `arg(z)` | Phase angle |

---

## std/matrix

**Source:** `src/stdlib/matrix.sfpp`

Linear algebra operations on `list<list<float_64>>`.

| Function | Alias | Signature | Description |
|----------|-------|-----------|-------------|
| `matrix_add` | `m_Add` | `m_Add(a, b) -> matrix` | Element-wise addition |
| `matrix_mul` | `m_Mul` | `m_Mul(a, b) -> matrix` | Matrix multiplication or scalar mul |
| `matrix_transpose` | `m_Transpose` | `m_Transpose(a) -> matrix` | Transpose |
| `matrix_scale` | `m_Scale` | `m_Scale(a, scalar) -> matrix` | Scale matrix |
| `matrix_eye` | `m_Eye` | `m_Eye(n) -> matrix` | Identity matrix |

```sfpp
import std/matrix as mat;

var A = [[1, 2], [3, 4]];
var B = mat.m_Eye(2);
var C = mat.m_Mul(A, B);  // [[1, 2], [3, 4]]
```

---

## std/string

**Source:** `src/stdlib/string.sfpp`

String manipulation functions.

| Function | Alias | Signature | Description |
|----------|-------|-----------|-------------|
| `str_trim` | `trim` | `trim(s) -> str` | Trim whitespace |
| `str_trimleft` | `trimLeft` | `trimLeft(s) -> str` | Trim left |
| `str_trimright` | `trimRight` | `trimRight(s) -> str` | Trim right |
| `str_upper` | `toUpper` | `toUpper(s) -> str` | Uppercase |
| `str_lower` | `toLower` | `toLower(s) -> str` | Lowercase |
| `str_startswith` | `startsWith` | `startsWith(s, prefix) -> bool` | Prefix check |
| `str_endswith` | `endsWith` | `endsWith(s, suffix) -> bool` | Suffix check |
| `str_contains` | `contains` | `contains(s, sub) -> bool` | Substring check |
| `str_replace` | `replace` | `replace(s, from, to) -> str` | Replace all |
| `str_split` | `split` | `split(s, delim) -> list<str>` | Split string |
| `str_join` | `join` | `join(list, delim) -> str` | Join list |
| `str_repeat` | `repeat` | `repeat(s, n) -> str` | Repeat n times |
| `str_indexof` | `indexOf` | `indexOf(s, sub) -> int` | Find substring (-1 if not found) |
| `str_slice` | `slice` | `slice(s, start, end?) -> str` | Substring |
| `str_padleft` | `padLeft` | `padLeft(s, width, char?) -> str` | Left pad |
| `str_padright` | `padRight` | `padRight(s, width, char?) -> str` | Right pad |
| `str_reverse` | `reverse` | `reverse(s) -> str` | Reverse string |
| `str_isdigit` | `isDigit` | `isDigit(s) -> bool` | All digits |
| `str_isalpha` | `isAlpha` | `isAlpha(s) -> bool` | All letters |
| `str_isalphanum` | `isAlphaNum` | `isAlphaNum(s) -> bool` | All alphanumeric |

---

## std/json

**Source:** `src/stdlib/json.sfpp`

Pure Sulfur++ JSON parser and serializer.

### Class: `_JSONParser`

Internal parser class — use module functions instead.

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `parse` | `parse(str) -> value` | Parse JSON string |
| `parseFile` | `parseFile(path) -> value` | Parse JSON file |
| `stringify` | `stringify(value) -> str` | Compact JSON |
| `pretty` | `pretty(value) -> str` | Pretty-printed JSON (4-space indent) |
| `stringifyFile` | `stringifyFile(path, value)` | Write JSON to file |

### Supported Types

| JSON | Sulfur++ |
|------|----------|
| `null` | `null` |
| `true`/`false` | `true`/`false` |
| Number | `int_64` or `float_64` |
| String | `str` |
| Array | `list` |
| Object | `dict` |

```sfpp
import std/json as json;

var data = json.parse("{\"name\": \"sulfur\", \"version\": 1}");
json.stringify(data);    // {"name":"sulfur","version":1}
json.pretty(data);       // {\n    "name": "sulfur",\n    "version": 1\n}
json.stringifyFile("config.json", data);
var loaded = json.parseFile("config.json");
```

---

## std/time

**Source:** `src/stdlib/time.sfpp`

Timing utilities.

### Functions

| Function | Alias | Signature | Description |
|----------|-------|-----------|-------------|
| `delay` | `delay` | `delay(ms)` | Sleep ms |
| `delayMillis` | `delayMillis` | `delayMillis(ms)` | Alias |
| `delayMicros` | `delayMicros` | `delayMicros(us)` | Sleep μs |
| `micros` | `micros` | `micros() -> int_64` | Microseconds since epoch |

### Convenience

```sfpp
fn millis() { return micros() / 1000; }
fn seconds() { return millis() / 1000; }
```

---

## std/hal

**Source:** `src/stdlib/hal.sfpp`

Hardware Abstraction Layer for GPIO (embedded/IoT).

### Constants

| Constant | Value |
|----------|-------|
| `OUTPUT` | 1 |
| `INPUT` | 0 |
| `HIGH` | 1 |
| `LOW` | 0 |

### Class: `Pin`

```sfpp
class Pin {
    pinNumber;
    +1>init;
    fn init(pin) { this.pinNumber = pin; }
    fn mode(m) { unsafe { iot_gpio_mode(this.pinNumber, m); } return this; }
    fn write(val) { unsafe { iot_gpio_write(this.pinNumber, val); } }
    fn read() { var val = 0; unsafe { val = iot_gpio_read(this.pinNumber); } return val; }
}
```

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `createPin` | `createPin(pin: int) -> Pin` | Create Pin instance |

```sfpp
import std/hal as hal;

var led = hal.createPin(13);
led.mode(hal.OUTPUT);
led.write(hal.HIGH);
hal.delay(500);
led.write(hal.LOW);
```

---

## std/events

**Source:** `src/stdlib/events.sfpp`

Simple async event loop (cooperative multitasking).

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `setInterval` | `setInterval(delayMs, callback)` | Repeating timer |
| `setTimeout` | `setTimeout(delayMs, callback)` | One-shot timer |
| `run` | `run()` | Start event loop |

### Internal

```sfpp
var tasks = [];  // List of {trigger, callback, delay, type}

fn addTask(delay, callback, type) {
    var triggerTime = time.micros() + (delay * 1000);
    tasks.push({"trigger": triggerTime, "callback": callback, "delay": delay, "type": type});
}
```

### Usage

```sfpp
import std/events as events;

events.setInterval(1000, fn() {
    print("Tick!");
});

events.setTimeout(5000, fn() {
    print("Done!");
    // Note: no way to stop loop currently
});

events.run();  // Blocks forever
```

---

## std/constants

**Source:** `src/stdlib/constants.sfpp`

Runtime version and time constants.

### Time Constants

| Constant | Value |
|----------|-------|
| `SECOND` | 1 |
| `MINUTE` | 60 |
| `HOUR` | 3600 |
| `DAY` | 86400 |
| `WEEK` | 604800 |

### Version Constants

| Constant | Description |
|----------|-------------|
| `RUNTIME_VERSION` | Runtime version string |
| `SULFUR_VERSION` | Language version |
| `COMBUST_VERSION` | Combust CLI version |
| `BUILD_MODE` | "debug" or "release" |
| `DEBUG_MODE` | Boolean |

### Property Constants (for `get()`)

| Constant | String Value |
|----------|--------------|
| `OBJ_NAME` | "OBJ_NAME" |
| `GET_ITEM` | "GET_ITEM" |
| `PROPS` | "PROPS" |

---

## std/runtime

**Source:** `src/stdlib/runtime.sfpp`

Runtime introspection.

### Class: `RuntimeAPI`

```sfpp
class RuntimeAPI {
    startTime;
    fn init() { this.startTime = time.micros(); }
    fn uptimeMillis() { return (time.micros() - this.startTime) / 1000; }
    fn uptimeSeconds() { return (time.micros() - this.startTime) / 1000000.0; }
    fn getEngineName() { return "combust"; }
    fn getLanguage() { return "Sulfur++"; }
}
```

### Global Instance

```sfpp
var Runtime = new RuntimeAPI();
```

```sfpp
import std/runtime as rt;

print("Uptime (ms): ", rt.Runtime.uptimeMillis());
print("Engine: ", rt.Runtime.getEngineName());
print("Language: ", rt.Runtime.getLanguage());
```

---

## std/science

**Source:** `src/stdlib/science.sfpp`

Physical constants (SI units).

| Constant | Alias | Value | Unit |
|----------|-------|-------|------|
| `SC_C` | `C` | 299792458.0 | m/s (speed of light) |
| `SC_G` | `G` | 6.67430e-11 | m³/kg/s² (gravitational) |
| `SC_H` | `H` | 6.62607015e-34 | J·s (Planck) |
| `SC_K` | `K` | 1.380649e-23 | J/K (Boltzmann) |
| `SC_NA` | `NA` | 6.02214076e23 | mol⁻¹ (Avogadro) |
| `SC_R` | `R` | 8.314462618 | J/mol/K (gas constant) |

### Convenience Aliases

```sfpp
let speedOfLight = C;
let planckConstant = H;
let gravitationalConstant = G;
let boltzmannConstant = K;
let avogadroNumber = NA;
let gasConstant = R;
```

---

## std/alias

**Source:** `src/stdlib/alias.sfpp`

Alias/shortcut system for custom syntax.

### Native

```sfpp
expose "__alias_register__" as __alias_register__;
```

### Class: `AliasAPI`

```sfpp
class AliasAPI {
    +1>init;
    fn init() {}
    fn create(pattern, expansion) {
        var parenPos = string.indexOf(pattern, "(");
        var name = pattern;
        var argsPart = "";
        if (parenPos != -1) {
            name = string.slice(pattern, 0, parenPos);
            var endParen = string.indexOf(pattern, ")");
            if (endParen != -1) {
                argsPart = string.slice(pattern, parenPos + 1, endParen);
            }
        }
        __alias_register__(name, argsPart, expansion);
    }
}
```

### Global Instance

```sfpp
var Alias = new AliasAPI();
```

### Usage

```sfpp
import std/alias as alias;

alias.Alias.create("log(msg)", "Terminal.Out << msg << Terminal.EOL");
alias.Alias.create("range(n)", "std/builtin.range(0, n)");
```

---

## std/ffi

**Source:** `src/stdlib/ffi.sfpp`

Foreign Function Interface for calling C/C++ shared libraries.

### Class: `Library`

```sfpp
class Library {
    handle;
    +1>init;
    fn init(h) { this.handle = h; }
    fn sym(name) { return ffi_dlsym(this.handle, name); }
    fn call(name, args, retType) {
        var func = ffi_dlsym(this.handle, name);
        return ffi_call(func, args, retType);
    }
    fn close() { ffi_dlclose(this.handle); }
}
```

### Functions

| Function | Signature | Description |
|----------|-----------|-------------|
| `load` | `load(path: str) -> Library` | Load shared library (dlopen) |
| `callInt` | `callInt(fnPtr, args: list) -> int_64` | Call returning int |
| `callFloat` | `callFloat(fnPtr, args: list) -> float_64` | Call returning float |
| `callVoid` | `callVoid(fnPtr, args: list)` | Call returning void |
| `callStr` | `callStr(fnPtr, args: list) -> str` | Call returning C string |
| `callRaw` | `callRaw(fnPtr, args: list) -> ptr` | Call returning pointer |
| `memRead` | `memRead(address: ptr, offset: int_64) -> int_64` | Read memory at offset |
| `memWrite` | `memWrite(address: ptr, offset: int_64, value: int_64)` | Write memory at offset |
| `strRead` | `strRead(address: ptr) -> str` | Read null-terminated C string |
| `strWrite` | `strWrite(address: ptr, offset: int_64, value: str)` | Write C string |
| `sizeof` | `sizeof(typeName: str) -> int_64` | Size of C type |

### Supported Return Types (for `ffi_call`)

| Type String | C Equivalent |
|-------------|--------------|
| `"void"` | `void` |
| `"int"` | `int64_t` |
| `"float"` | `double` |
| `"ptr"` | `void*` |
| `"str"` | `const char*` |

### Supported `sizeof` Types

`int`, `int64`, `int32`, `int16`, `int8`, `float`/`float64`/`double`, `float32`, `ptr`/`pointer`, `bool`

### Example: Calling libc

```sfpp
import std/ffi as ffi;

var libc = ffi.load("libc.so.6");
var strlen = libc.sym("strlen");

// Call via wrapper
var len = ffi.callInt(strlen, ["Hello, FFI!"]);  // Returns 11

// Or call directly
var len2 = ffi.call(strlen, ["Hello"], "int");

// Memory operations
var ptr = ffi.callRaw(libc.sym("malloc"), [256], "ptr");
ffi.memWrite(ptr, 0, 42);
var val = ffi.memRead(ptr, 0);  // 42
ffi.callVoid(libc.sym("free"), [ptr]);
```

---

## Module Dependency Graph

```
std/builtin
  └── (no deps)

std/collections
  └── (no deps)

std/math
  └── (no deps)

std/matrix
  └── std/math (constants)

std/string
  └── (no deps)

std/io
  └── std/builtin (Terminal, TIO)

std/time
  └── std/builtin (delay, now)

std/json
  ├── std/io (readFile, writeFile)
  └── std/collections (keys)

std/hal
  └── std/builtin (unsafe, delay)

std/events
  └── std/time (micros, delay)

std/constants
  └── std/builtin (expose)

std/runtime
  └── std/time (micros)

std/science
  └── std/builtin (expose)

std/sys
  └── std/builtin (Terminal, TIO, overwrite)
  └── std/io (io_e, io_withCategory, etc.)

std/alias
  └── std/string (indexOf, slice)
  └── std/builtin (__alias_register__)

std/ffi
  └── std/builtin (expose native ffi_*)
```

---

## Loading Order

Modules are loaded on-demand when imported. The interpreter registers all native builtins at startup (`Interpreter::registerBuiltins()`), so `expose` statements simply create aliases in the module scope.

Circular imports are not detected — avoid them.