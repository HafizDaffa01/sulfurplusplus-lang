# FFI & Unsafe Documentation

This document covers the Foreign Function Interface (FFI) for calling C/C++ shared libraries, and the `unsafe` block mechanism for low-level memory and hardware access.

---

## Overview

Sulfur++ provides two mechanisms for system integration:

| Mechanism | Use Case | Safety |
|-----------|----------|--------|
| **`unsafe` blocks** | Direct memory access, GPIO, hardware registers | Manual — programmer responsible |
| **FFI (std/ffi)** | Calling C/C++ shared libraries (`.so`, `.dylib`, `.dll`) | Type-checked at call boundary |

Both require explicit opt-in — no accidental system access.

---

## unsafe Blocks

### Syntax

```sfpp
unsafe {
    // Unsafe operations allowed here
    var ptr = sys_memory_alloc(1024);
    sys_memory_write(ptr, 42);
    var val = sys_memory_read(ptr);
    sys_memory_free(ptr);
}
```

### Rules

1. **Scope-limited** — Only code inside `{ }` can use unsafe operations
2. **No implicit escape** — Unsafe values (raw pointers) can be passed out but originate from unsafe context
3. **Nestable** — `unsafe` blocks can nest
3. **Function calls** — Calling a native function from safe code is allowed; the native function itself may do unsafe things

### Allowed in unsafe

| Operation | Native Function |
|-----------|-----------------|
| Raw memory alloc/free | `sys_memory_alloc`, `sys_memory_free` |
| Raw memory read/write | `sys_memory_read`, `sys_memory_write` |
| GPIO access | `iot_gpio_mode`, `iot_gpio_write`, `iot_gpio_read` |
| FFI calls | `ffi_dlopen`, `ffi_dlsym`, `ffi_call`, etc. |
| Pointer deref | `*ptr`, `&var` (address-of) |
| `new`/`delete` | For raw pointers |

### Example: GPIO Control

```sfpp
import std/hal as hal;

fn blinkLed(pin: int_64, times: int_64) {
    var led = hal.createPin(pin);
    led.mode(hal.OUTPUT);
    
    for (i = 0; i < times; i = i + 1) {
        unsafe {
            led.write(hal.HIGH);
        }
        delay(500);
        unsafe {
            led.write(hal.LOW);
        }
        delay(500);
    }
}

blinkLed(13, 5);
```

### Example: Memory Buffer

```sfpp
fn allocateBuffer(size: int_64) -> ptr {
    unsafe {
        return sys_memory_alloc(size);
    }
}

fn writeBuffer(ptr: ptr, offset: int_64, value: int_64) {
    unsafe {
        sys_memory_write(ptr, offset, value);  // Note: sys_memory_write takes (addr, val)
    }
}

fn readBuffer(ptr: ptr, offset: int_64) -> int_64 {
    unsafe {
        return sys_memory_read(ptr + offset);
    }
}

fn freeBuffer(ptr: ptr) {
    unsafe {
        sys_memory_free(ptr);
    }
}

var buf = allocateBuffer(256);
writeBuffer(buf, 0, 42);
var val = readBuffer(buf, 0);  // 42
freeBuffer(buf);
```

---

## Pointer Types

### Internal Pointers (`ref` / `PtrValue.target`)

- Point to other `Value` objects in the VM
- Used for: `ref` parameters, aliasing variables
- Created via: `makePtr(&existingValue)`
- Type: `ptr` in language

### Raw Pointers (`PtrValue.rawPtr`)

- Point to arbitrary C memory addresses
- Used for: FFI, hardware registers, `malloc`/`free`
- Created via: `makeRawPtr(void*)` or FFI functions
- Type: `ptr` in language (same type, different internal representation)

### Pointer Operations

```sfpp
// Address-of (creates internal pointer)
var x = 42;
var px = &x;  // px is ptr to x

// Dereference (reads internal pointer target)
var y = *px;  // y = 42

// Raw pointer arithmetic (unsafe)
unsafe {
    var raw = sys_memory_alloc(100);
    var p = raw + 8;  // pointer arithmetic on rawPtr
    sys_memory_write(p, 123);
}
```

---

## FFI: Foreign Function Interface

The `std/ffi` module provides a complete interface for loading shared libraries and calling C functions.

### Module Import

```sfpp
import std/ffi as ffi;
```

### Core Concepts

| Concept | Description |
|---------|-------------|
| **Library handle** | Opaque pointer from `dlopen` |
| **Symbol** | Function address from `dlsym` |
| **Call convention** | System V AMD64 ABI (Linux/macOS), Microsoft x64 (Windows) |
| **Argument passing** | First 6 integer args in registers (rdi, rsi, rdx, rcx, r8, r9), first 4 float args in xmm0-xmm3 |
| **Return types** | `void`, `int` (int64_t), `float` (double), `ptr` (void*), `str` (const char*) |

### Library Management

```sfpp
// Load library
var libc = ffi.load("libc.so.6");        // Linux
var libm = ffi.load("libm.so.6");        // Math library
var custom = ffi.load("./mylib.so");     // Relative path

// Get symbol (function pointer)
var strlen = libc.sym("strlen");
var sin = libm.sym("sin");
var myFunc = custom.sym("my_function");

// Close library
libc.close();
```

### Calling Functions

#### High-Level Wrappers

```sfpp
// Returns int_64
var len = ffi.callInt(strlen, ["Hello"]);

// Returns float_64
var result = ffi.callFloat(sin, [1.57]);

// Returns void
ffi.callVoid(puts, ["Hello, FFI!"]);

// Returns string (const char*)
var cstr = ffi.callStr(ctime, [&timestamp]);

// Returns raw pointer
var ptr = ffi.callRaw(malloc, [1024]);
```

#### Low-Level Call

```sfpp
// Full control over return type
var result = ffi.call(funcPtr, argsList, "int");      // int_64
var result = ffi.call(funcPtr, argsList, "float");    // float_64
var result = ffi.call(funcPtr, argsList, "void");     // null
var result = ffi.call(funcPtr, argsList, "ptr");      // ptr
var result = ffi.call(funcPtr, argsList, "str");      // str
```

### Argument Conversion

Arguments in the `argsList` are automatically converted:

| Sulfur++ Type | C Type | Register (AMD64) |
|---------------|--------|------------------|
| `int_64` | `int64_t` | Integer (rdi, rsi, ...) |
| `float_64` | `double` | Float (xmm0, xmm1, ...) |
| `str` | `const char*` | Integer (pointer in rdi, ...) |
| `ptr` (raw) | `void*` | Integer |
| `null` | `NULL` | Integer (0) |
| `bool` | `int64_t` | Integer (0/1) |

**Limits:**
- Max 6 integer/pointer arguments
- Max 4 float arguments
- Mixed calls: integers and floats use separate register sets

### Memory Operations

```sfpp
// Allocate via libc
var malloc = libc.sym("malloc");
var free = libc.sym("free");

var ptr = ffi.callRaw(malloc, [256]);  // void* malloc(size_t)

// Read/write memory
var val = ffi.memRead(ptr, 0);         // Read int64 at offset
ffi.memWrite(ptr, 8, 42);              // Write int64 at offset

// String operations
var cstr = ffi.callStr(strdup, ["Hello"]);  // Get C string
var rustStr = ffi.strRead(cstr);             // Read as Sulfur++ string
ffi.strWrite(ptr, 0, "New content");         // Write string to C memory

// Free
ffi.callVoid(free, [ptr]);
```

### Type Sizes

```sfpp
ffi.sizeof("int")      // 8 (int64_t)
ffi.sizeof("int32")    // 4
ffi.sizeof("int16")    // 2
ffi.sizeof("int8")     // 1
ffi.sizeof("float")    // 8 (double)
ffi.sizeof("float32")  // 4
ffi.sizeof("ptr")      // 8 (pointer)
ffi.sizeof("bool")     // 1
```

### Complete Example: Calling libcurl

```sfpp
import std/ffi as ffi;
import std/io as io;

fn httpGet(url: str) -> str {
    var curl = ffi.load("libcurl.so.4");
    
    var curl_easy_init = curl.sym("curl_easy_init");
    var curl_easy_setopt = curl.sym("curl_easy_setopt");
    var curl_easy_perform = curl.sym("curl_easy_perform");
    var curl_easy_cleanup = curl.sym("curl_easy_cleanup");
    
    // Constants (from curl.h)
    let CURLOPT_URL = 10002;
    let CURLOPT_WRITEFUNCTION = 20011;
    let CURLOPT_WRITEDATA = 10001;
    
    var handle = ffi.callRaw(curl_easy_init, []);
    
    // Set URL
    ffi.callInt(curl_easy_setopt, [handle, CURLOPT_URL, url], "int");
    
    // Buffer for response
    var buffer = ffi.callRaw(ffi.load("libc.so.6").sym("malloc"), [4096]);
    
    // Write callback (simplified - real impl needs proper callback)
    // ... callback setup ...
    
    // Perform
    var result = ffi.callInt(curl_easy_perform, [handle], "int");
    
    // Read response
    var response = ffi.strRead(buffer);
    
    // Cleanup
    ffi.callVoid(curl_easy_cleanup, [handle]);
    curl.close();
    
    return response;
}

var html = httpGet("https://example.com");
io.Terminal.Out << html << "\n";
```

---

## Platform Notes

### Linux
- Libraries: `.so` files
- `dlopen`/`dlsym`/`dlclose` from `<dlfcn.h>`
- Call convention: System V AMD64 ABI

### macOS
- Libraries: `.dylib` files
- Same `dlopen` API
- Call convention: System V AMD64 ABI

### Windows
- Libraries: `.dll` files
- Use `LoadLibrary`/`GetProcAddress`/`FreeLibrary` (not yet implemented)
- Call convention: Microsoft x64 (first 4 args in rcx, rdx, r8, r9)

---

## Safety Guidelines

### Unsafe Blocks
1. **Minimize scope** — Keep unsafe blocks as small as possible
2. **Validate pointers** — Check for null before dereferencing
3. **Pair alloc/free** — Every `alloc` needs a `free`
4. **No aliasing violations** — Don't create multiple mutable pointers to same memory

### FFI
1. **Verify signatures** — Wrong argument count/types = undefined behavior
2. **Manage lifetimes** — C strings returned by FFI may be freed by library
3. **Handle errors** — Check return codes, errno
4. **Thread safety** — Many C libraries aren't thread-safe

```sfpp
// GOOD: Validate and handle errors
var handle = ffi_dlopen("libfoo.so");
if (handle == null) {
    throw "Failed to load libfoo: " + ffi_dlerror();
}

var fn = ffi_dlsym(handle, "foo_func");
if (fn == null) {
    throw "Symbol not found";
}

// BAD: No validation
var fn = ffi_dlsym(ffi_dlopen("libfoo.so"), "foo_func");
ffi.call(fn, [], "int");  // CRASH if any step failed
```

---

## Internal Implementation

### Native Functions (in `interpreter.cpp`)

| Native | Implementation |
|--------|----------------|
| `ffi_dlopen` | `dlopen(path, RTLD_NOW \| RTLD_LOCAL)` |
| `ffi_dlsym` | `dlsym(handle, name)` with `dlerror()` check |
| `ffi_dlclose` | `dlclose(handle)` |
| `ffi_call` | Manual argument marshalling + direct call via function pointer cast |
| `ffi_mem_read` | `*(int64_t*)((char*)base + offset)` |
| `ffi_mem_write` | `*(int64_t*)((char*)base + offset) = val` |
| `ffi_str_read` | `strlen` + copy |
| `ffi_str_write` | `memcpy` with null terminator |
| `ffi_sizeof` | Hardcoded type sizes |

### Argument Marshalling (`ffi_call`)

```cpp
// 1. Separate integer and float args
// 2. Convert each Sulfur++ Value to raw C value
// 3. Cast function pointer to correct signature
// 4. Call via switch on arg count (0-6)
// 5. Convert return value back to Sulfur++ Value
```

**Supported signatures (auto-selected by arg count):**
- `int64_t fn()`
- `int64_t fn(int64_t)`
- `int64_t fn(int64_t, int64_t)`
- ... up to 6 args
- Same for `double` return (max 4 float args)
- `void fn(...)` (max 6 args)
- `void* fn(...)` (max 3 args)
- `const char* fn(...)` (max 2 args)

---

## Debugging FFI

### Common Issues

| Symptom | Cause | Fix |
|---------|-------|-----|
| Segfault on call | Wrong calling convention / arg count | Verify C signature matches |
| Wrong return value | Return type mismatch | Use correct `retType` string |
| Crash on string return | C string freed by library | Copy string immediately |
| `dlsym` returns null | Symbol name mangled / wrong | Check `nm -D lib.so \| grep func` |
| Library not found | Path wrong / dependencies missing | Use absolute path, check `ldd` |

### Debug Tips

```sfpp
// Print function pointer
var fn = libc.sym("strlen");
print("fn ptr: ", fn);  // Shows <ptr> but rawPtr is accessible

// Check dlerror manually
var handle = ffi_dlopen("nonexistent.so");
if (handle.isNull()) {
    // dlerror not directly exposed, but ffi_dlopen throws on failure
}
```

---

## Future Extensions

Planned FFI improvements:
- Struct passing/returning (by value)
- Callback support (C function pointers → Sulfur++ functions)
- Windows support (`LoadLibrary`/`GetProcAddress`)
- Automatic header parsing / bindgen
- Async FFI calls (non-blocking)