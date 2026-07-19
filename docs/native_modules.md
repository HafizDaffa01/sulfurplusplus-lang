# Sulfur++ Native Module System

Sulfur++ supports native modules written in C/C++ that can be loaded at runtime, similar to Python's C extension modules.

## Overview

Native modules are shared libraries (`.so` on Linux, `.dylib` on macOS, `.dll` on Windows) that export a `sulfurpp_module_init` function. They are imported using the standard `import` statement.

## Quick Start

### 1. Create a Native Module

```cpp
// mymodule.cpp
#include "interpreter.hpp"
#include "value.hpp"

using ValuePtr = ValuePtr;
using FunctionValue = FunctionValue;

// Native function implementation
static ValuePtr my_add(Interpreter* interp, std::vector<ValuePtr> args) {
    if (args.size() != 2) {
        interp->throwTypeError("my_add requires 2 arguments", -1);
        return makeNull();
    }
    int64_t a = args[0]->isInt() ? args[0]->asInt() : (int64_t)args[0]->asFloat();
    int64_t b = args[1]->isInt() ? args[1]->asInt() : (int64_t)args[1]->asFloat();
    return makeInt(a + b);
}

// Module initialization - called when module is imported
extern "C" {
    ValuePtr sulfurpp_module_init(Interpreter* interp) {
        auto module = std::make_shared<DictValue>();
        
        auto fn = std::make_shared<FunctionValue>();
        fn->name = "my_add";
        fn->isNative = true;
        fn->native = my_add;
        module->set("my_add", makeFn(fn));
        
        return makeDict(module);
    }
}
```

### 2. Build with CMake

```cmake
# In your CMakeLists.txt
add_library(mymodule MODULE mymodule.cpp)
target_include_directories(mymodule PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(mymodule PRIVATE sulfur)
set_target_properties(mymodule PROPERTIES
    PREFIX ""
    SUFFIX ".so"
    OUTPUT_NAME "mymodule"
)
```

### 3. Use in Sulfur++

```sfpp
import mymodule as m;

var result = m.my_add(10, 20);  // 30
```

## Module Loading

The interpreter searches for native modules in this order:

1. Direct path (absolute or relative to CWD)
2. `./module.so` (current directory)
3. `../module.so` (parent directory)
4. `examples/module.so`
5. `build/examples/module.so`
6. `build/std/module.so`
7. `build/module.so`
8. `src/stdlib/module.so`
9. `packages/module.so`

If not found as a native module, it falls back to loading `.sfpp` files.

## API Reference

### Value Type Operations

```cpp
// Type checking
val->isNull()      // bool
val->isBool()      // bool
val->isInt()       // bool
val->isFloat()     // bool
val->isStr()       // bool
val->isList()      // bool
val->isDict()       // bool
val->isFn()        // bool

// Value extraction
val->asBool()      // bool
val->asInt()       // int64_t
val->asFloat()     // double
val->asStr()       // std::string
val->asList()      // std::shared_ptr<ListValue>
val->asDict()      // std::shared_ptr<DictValue>

// Conversions
val->toString()    // std::string
```

### Factory Functions

```cpp
makeNull()                      // ValuePtr
makeBool(bool)                  // ValuePtr
makeInt(int64_t)                // ValuePtr
makeFloat(double)               // ValuePtr
makeStr(const std::string&)     // ValuePtr
makeList(shared_ptr<ListValue>) // ValuePtr
makeDict(shared_ptr<DictValue>) // ValuePtr
makeFn(shared_ptr<FunctionValue>) // ValuePtr
```

### Composite Types

```cpp
// List
auto list = std::make_shared<ListValue>();
list->elements.push_back(makeInt(1));
list->elements.push_back(makeStr("two"));

// Dict
auto dict = std::make_shared<DictValue>();
dict->set("key", makeInt(42));
ValuePtr val = dict->get("key");  // returns null if not found
```

### FunctionValue

```cpp
auto fn = std::make_shared<FunctionValue>();
fn->name = "my_function";
fn->isNative = true;
fn->native = [](Interpreter* interp, std::vector<ValuePtr> args) -> ValuePtr {
    // Your implementation
    return makeInt(42);
};

// Register in module
module->set("my_function", makeFn(fn));
```

### Interpreter Methods for Native Functions

```cpp
// Error throwing
interp->throwRuntimeError(const std::string& msg, int line);
interp->throwTypeError(const std::string& msg, int line);
interp->throwMathError(const std::string& msg, int line);
interp->throwIOError(const std::string& msg, int line);

// Output
interp->print(const std::string& s);
interp->printErr(const std::string& s);

// Input
interp->readLine() -> std::string
```

## Complete Example: Math Module

See `examples/mymath.cpp` for a complete working example with:
- Multiple functions (add, multiply, fibonacci, sum_list, greet)
- Constants (PI, E, VERSION)
- Proper error handling
- CMake integration

### Building the Example

```bash
cmake --build build --config Release
```

This creates `build/mymath.so`.

### Using the Example

```sfpp
// test_mymath.sfpp
import mymath as math;

print("math.add(2, 3) = ", math.add(2, 3));
print("math.multiply(4, 5) = ", math.multiply(4, 5));
print("math.fibonacci(10) = ", math.fibonacci(10));
print("math.sum_list([1,2,3,4,5]) = ", math.sum_list([1, 2, 3, 4, 5]));
print("math.greet() = ", math.greet());
print("math.greet('World') = ", math.greet("World"));
print("math.PI = ", math.PI);
print("math.E = ", math.E);
```

Run:
```bash
./build/combust run test_mymath.sfpp
```

Output:
```
math.add(2, 3) = 5
math.multiply(4, 5) = 20
math.fibonacci(10) = 55
math.sum_list([1, 2, 3, 4, 5]) = 15
math.greet() = Hello, World from native module!
math.greet('World') = Hello, World from native module!
math.PI = 3.141592653589793
math.E = 2.718281828459045
```

## C API (Alternative)

For a more stable C API (similar to Python's C API), include `sulfur_api.h`:

```c
#include "sulfur_api.h"

static ValuePtr my_add(SulfurVM* vm, ValuePtr* args, int nargs) {
    int64_t a = sulfur_to_int(vm, args[0]);
    int64_t b = sulfur_to_int(vm, args[1]);
    return sulfur_new_int(vm, a + b);
}

SULFUR_MODULE_INIT(mymodule) {
    static SulfurMethodDef methods[] = {
        {"add", my_add, 2, "Add two integers"},
        {NULL, NULL, 0, NULL}
    };
    sulfur_module_add_functions(vm, mod, methods);
    return 0;
}
```

This C API provides:
- Reference counting (`sulfur_incref`, `sulfur_decref`)
- Type checking (`sulfur_is_int`, `sulfur_is_str`, etc.)
- Value creation (`sulfur_new_int`, `sulfur_new_string`, etc.)
- Module registration (`sulfur_module_add_functions`, etc.)
- Error handling (`sulfur_set_error`, `sulfur_has_error`)

## Best Practices

1. **Error Handling**: Always check argument count and types, use `interp->throwTypeError()` for invalid arguments.

2. **Memory Management**: The VM manages Value lifetimes via reference counting. Return values from factory functions (`makeInt`, `makeStr`, etc.) are automatically managed.

3. **Thread Safety**: Native functions run in the VM's thread. Avoid blocking operations; use async patterns if needed.

4. **ABI Stability**: Use the C API (`sulfur_api.h`) for maximum compatibility across Sulfur++ versions.

5. **Module Naming**: The `.so` filename should match the import name (e.g., `mymath.so` for `import mymath`).

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `undefined symbol` | Link against `sulfur` library: `target_link_libraries(mymodule sulfur)` |
| `cannot open shared object` | Ensure `.so` is in search path or use absolute path |
| `missing sulfurpp_module_init` | Export `extern "C" ValuePtr sulfurpp_module_init(Interpreter*)` |
| Segfault on call | Check argument count/types; ensure return value is valid `ValuePtr` |
| Symbol conflicts | Use unique names; consider C API for stable ABI |

## Comparison: FFI vs Native Modules

| Feature | FFI (`std/ffi`) | Native Modules |
|---------|----------------|----------------|
| Use case | Call existing C libs | Extend Sulfur++ runtime |
| Performance | Lower (thunk overhead) | Higher (direct calls) |
| Type safety | Manual | Automatic via Value |
| Build complexity | None (runtime) | Requires compilation |
| ABI stability | Runtime only | Stable C API available |