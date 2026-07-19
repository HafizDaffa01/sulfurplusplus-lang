# Value System & Type Documentation

This document describes the internal value representation, type system, and runtime semantics of Sulfur++.

---

## Overview

The `Value` class (`include/value.hpp`, `src/core/value.cpp`) is the unified representation for all Sulfur++ values at runtime. It uses a `std::variant` (tagged union) to store different types efficiently.

---

## Type Hierarchy

### Variant Alternatives (`ValueVariant`)

```cpp
using ValueVariant = std::variant<
    std::monostate,                    // null
    bool,                              // bool
    int64_t,                           // int_64
    double,                            // float_64
    std::complex<double>,              // complex_128
    std::shared_ptr<std::string>,      // str
    char,                              // char
    std::shared_ptr<FunctionValue>,    // fn
    std::shared_ptr<ClassDef>,         // class
    std::shared_ptr<ClassInstance>,    // instance
    std::shared_ptr<StructDef>,        // struct
    std::shared_ptr<StructInstance>,   // struct_instance
    std::shared_ptr<ListValue>,        // list
    std::shared_ptr<SetValue>,         // set
    std::shared_ptr<DictValue>,        // dict
    std::shared_ptr<PtrValue>          // ptr (unsafe)
>;
```

---

## Type Mapping: Language ↔ Internal

| Sulfur++ Type | Internal Variant | `typeName()` |
|---------------|------------------|--------------|
| `null` | `std::monostate` | `"null"` |
| `bool` | `bool` | `"bool"` |
| `int_8`...`int_64` | `int64_t` | `"int_64"` |
| `uint_8`...`uint_64` | `int64_t` | `"int_64"` |
| `float_32`, `float_64` | `double` | `"float_64"` |
| `complex_128` | `std::complex<double>` | `"complex_128"` |
| `str` | `shared_ptr<string>` | `"str"` |
| `char` | `char` | `"char"` |
| `fn` | `shared_ptr<FunctionValue>` | `"fn"` |
| `class` | `shared_ptr<ClassDef>` | `"class"` |
| `instance` | `shared_ptr<ClassInstance>` | `"instance"` |
| `struct` | `shared_ptr<StructDef>` | `"struct"` |
| `struct_instance` | `shared_ptr<StructInstance>` | `"struct_instance"` |
| `list` | `shared_ptr<ListValue>` | `"list"` |
| `set` | `shared_ptr<SetValue>` | `"set"` |
| `dict` | `shared_ptr<DictValue>` | `"dict"` |
| `ptr` | `shared_ptr<PtrValue>` | `"ptr"` |

**Note**: All integer types collapse to `int64_t` at runtime. All floats collapse to `double`.

---

## Value Class API

### Construction

```cpp
// Primitive
Value()                          // null
Value(bool)                      // bool
Value(int64_t)                   // int (uses cache for 0-256)
Value(double)                    // float
Value(std::complex<double>)      // complex
Value(const std::string&)        // str (allocates shared_ptr)
Value(char)                      // char

// Objects
Value(std::shared_ptr<FunctionValue>)
Value(std::shared_ptr<ClassDef>)
Value(std::shared_ptr<ClassInstance>)
Value(std::shared_ptr<StructDef>)
Value(std::shared_ptr<StructInstance>)
Value(std::shared_ptr<ListValue>)
Value(std::shared_ptr<SetValue>)
Value(std::shared_ptr<DictValue>)
Value(std::shared_ptr<PtrValue>)
```

### Type Predicates

```cpp
bool isNull() const;
bool isBool() const;
bool isInt() const;
bool isFloat() const;
bool isComplex() const;
bool isStr() const;
bool isChar() const;
bool isList() const;
bool isDict() const;
bool isSet() const;
bool isFn() const;
bool isClassDef() const;
bool isClassInst() const;
bool isStructDef() const;
bool isStructInst() const;
bool isPtr() const;      // PtrValue (internal or raw)
```

### Value Extractors (throw on type mismatch)

```cpp
bool asBool() const;
int64_t asInt() const;
double asFloat() const;
std::complex<double> asComplex() const;
std::string asStr() const;
char asChar() const;
std::shared_ptr<FunctionValue> asFn() const;
std::shared_ptr<ClassInstance> asClassInst() const;
std::shared_ptr<StructInstance> asStructInst() const;
std::shared_ptr<ListValue> asList() const;
std::shared_ptr<SetValue> asSet() const;
std::shared_ptr<DictValue> asDict() const;
std::shared_ptr<PtrValue> asPtr() const;
```

### Utility Methods

```cpp
bool truthy() const;              // JavaScript-style truthiness
std::string toString() const;     // Human-readable representation
std::string typeName() const;     // Type name for debugging
bool equals(const Value& other) const;  // Structural equality
```

---

## Truthiness Rules

| Type | Falsy Values | Truthy Values |
|------|-------------|---------------|
| `null` | always | never |
| `bool` | `false` | `true` |
| `int` | `0` | non-zero |
| `float` | `0.0`, `NaN` | non-zero, non-NaN |
| `complex` | `0+0i` | any non-zero component |
| `str` | `""` (empty) | non-empty |
| `char` | `'\0'` | non-null char |
| `list` | `[]` (empty) | non-empty |
| `dict` | `{}` (empty) | non-empty |
| `set` | `set{}` (empty) | non-empty |
| `fn` | never | always |
| `class/struct/instance` | never | always |
| `ptr` | never | always |

---

## Factory Functions (`value.hpp`)

```cpp
// Singletons (cached)
ValuePtr makeNull();                    // static cached
ValuePtr makeBool(bool);                // static cached true/false
ValuePtr makeInt(int64_t);              // cached for 0-256

// New allocations
ValuePtr makeFloat(double);
ValuePtr makeComplex(std::complex<double>);
ValuePtr makeStr(const std::string&);
ValuePtr makeChar(char);
ValuePtr makeList(std::shared_ptr<ListValue>);
ValuePtr makeDict(std::shared_ptr<DictValue>);
ValuePtr makeSet(std::shared_ptr<SetValue>);
ValuePtr makeFn(std::shared_ptr<FunctionValue>);
ValuePtr makeClassDef(std::shared_ptr<ClassDef>);
ValuePtr makeClassInst(std::shared_ptr<ClassInstance>);
ValuePtr makeStructDef(std::shared_ptr<StructDef>);
ValuePtr makeStructInst(std::shared_ptr<StructInstance>);

// Pointers
ValuePtr makePtr(ValuePtr* target);     // Internal pointer (ref to existing Value)
ValuePtr makeRawPtr(void* ptr);         // Raw C pointer (for FFI)
```

---

## Composite Value Structures

### ListValue
```cpp
struct ListValue {
    std::vector<ValuePtr> elements;
};
```
- Dynamic array, heterogeneous elements
- Used for both lists and matrices (list of lists)

### DictValue
```cpp
struct DictValue {
    std::unordered_map<std::string, ValuePtr> pairs;

    ValuePtr get(const std::string& key) const;  // returns null if missing
    void set(const std::string& key, ValuePtr val);
    bool has(const std::string& key) const;
};
```
- String keys only
- `get()` returns `null` for missing keys (not exception)

### SetValue
```cpp
struct SetValue {
    std::vector<ValuePtr> elements;  // ordered for simplicity
};
```
- Unique elements (enforced by builtin methods)
- Iteration order = insertion order

### FunctionValue
```cpp
struct FunctionValue {
    std::string name;
    std::vector<std::pair<std::string,std::string>> params; // name, type
    std::string retType;
    void* body;                    // Stmt* (opaque)
    void* decl;                    // const FnDeclStmt* (opaque)
    std::shared_ptr<Environment> closure;
    bool isNative = false;
    int callCount = 0;
    std::function<ValuePtr(std::vector<ValuePtr>)> native;
    std::string definedInFile;
};
```
- **User functions**: `isNative=false`, `body` points to AST, `closure` captures env
- **Native functions**: `isNative=true`, `native` holds `std::function` callback

### ClassDef
```cpp
struct ClassDef {
    std::string name;
    std::vector<std::string> interfaces;
    std::shared_ptr<Environment> methods;  // method name -> FunctionValue
    std::vector<std::pair<std::string,std::string>> fields; // name, type
    std::vector<std::pair<int,std::string>> ctorOrder;     // +N>method
    std::vector<std::pair<int,std::string>> dtorOrder;     // ~N>method
};
```

### ClassInstance
```cpp
struct ClassInstance {
    std::shared_ptr<ClassDef> def;
    std::shared_ptr<Environment> members;  // instance fields + bound methods
};
```
- Reference semantics (shared_ptr)
- `members` env inherits from `def->methods` for method lookup

### StructDef
```cpp
struct StructDef {
    std::string name;
    std::vector<std::pair<std::string,std::string>> fields; // name, type
};
```
- Value semantics (copied on assignment)

### StructInstance
```cpp
struct StructInstance {
    std::shared_ptr<StructDef> def;
    std::map<std::string, ValuePtr> fields;  // ordered map
};
```

### PtrValue (Unsafe)
```cpp
struct PtrValue {
    ValuePtr* target;    // Internal: pointer to another Value in VM
    void* rawPtr = nullptr;  // External: raw C pointer (for FFI)
};
```
- **Internal pointer** (`target != nullptr`): Used for `ref` parameters, aliasing
- **Raw pointer** (`rawPtr != nullptr`): Used for FFI, `unsafe` blocks
- Only one is active at a time

---

## Equality Semantics

`Value::equals(const Value& other)` implements structural equality:

```cpp
bool equals(const Value& other) const {
    if (isNull() && other.isNull()) return true;
    if (isBool() && other.isBool()) return asBool() == other.asBool();
    if (isInt() && other.isInt()) return asInt() == other.asInt();
    if (isFloat() && other.isFloat()) return asFloat() == other.asFloat();
    if (isInt() && other.isFloat()) return (double)asInt() == other.asFloat();
    if (isFloat() && other.isInt()) return asFloat() == (double)other.asInt();
    if (isComplex() || other.isComplex()) { /* complex comparison */ }
    if (isStr() && other.isStr()) return asStr() == other.asStr();
    if (isChar() && other.isChar()) return asChar() == other.asChar();
    return false;  // Objects: identity not compared
}
```

**Note**: Functions, classes, instances, lists, dicts, sets, pointers are **not** structurally compared — `equals` returns `false` for non-primitive types.

---

## String Representation (`toString()`)

| Type | Format |
|------|--------|
| `null` | `"null"` |
| `bool` | `"true"` / `"false"` |
| `int` | `"42"` |
| `float` | `"3.14"` (no trailing `.0` for whole numbers < 1e15) |
| `complex` | `"(3+4i)"` |
| `str` | `"hello"` (no quotes) |
| `char` | `"a"` |
| `fn` | `"<fn name>"` |
| `class` | `"<class Name>"` |
| `instance` | `"<Name instance>"` |
| `struct` | `"<struct Name>"` |
| `struct_instance` | `"Name{field: val, ...}"` |
| `list` | `"[1, 2, 3]"` |
| `set` | `"set{1, 2, 3}"` |
| `dict` | `"{key: val, ...}"` |
| `ptr` | `"<ptr>"` |

---

## ValueCache (Optimization)

```cpp
struct ValueCache {
    Value nullVal;
    Value trueVal;
    Value falseVal;
    std::vector<Value> smallInts;  // 0-256 pre-allocated

    static ValueCache& get() { static ValueCache instance; return instance; }
private:
    ValueCache() : nullVal(), trueVal(true), falseVal(false) {
        smallInts.reserve(257);
        for (int i = 0; i <= 256; ++i) smallInts.push_back(Value((int64_t)i));
    }
};
```

- `makeNull()` → `ValueCache::get().nullVal`
- `makeBool(true)` → `ValueCache::get().trueVal`
- `makeBool(false)` → `ValueCache::get().falseVal`
- `makeInt(0..256)` → `ValueCache::get().smallInts[i]`
- Avoids allocation for common values

---

## Memory Management

- All composite objects use `std::shared_ptr` — reference counted
- `Value` itself is a value type (contains variant, not pointer)
- `ValuePtr = Value` (typedef, not a pointer!)
- Cycles possible via: `ClassInstance.members` → `ClassInstance` (via `this` in methods)
- Environment uses `shared_ptr` for parent chain

---

## Type Coercion in Operations

### Arithmetic (`+`, `-`, `*`, `/`, `%`, `**`)

| Left \ Right | int | float | complex | str | list |
|--------------|-----|-------|---------|-----|------|
| **int** | int | float | complex | concat* | repeat* |
| **float** | float | float | complex | error | error |
| **complex** | complex | complex | complex | error | error |
| **str** | concat* | error | error | concat | error |
| **list** | repeat* | error | error | error | concat |

\* `str + int` → not supported directly (use `toStr(int)`)
\* `list * int` → repeat list

### Comparison (`==`, `!=`, `<`, `>`, `<=`, `>=`)

- Numeric types: cross-type comparison allowed (int vs float)
- String: lexicographic
- Complex: compares magnitude? No — only `==`/`!=` for complex, ordering throws
- Objects: not comparable (returns false)

---

## Built-in Methods (via `callBuiltinMethod`)

Methods are dispatched based on `obj->typeName()`:

### List Methods
```sfpp
len() -> int
push(val) -> null
add(val) -> null (alias)
pop() -> val
shift() -> val
unshift(val) -> null
contains(val) -> bool
includes(val) -> bool (alias)
reverse() -> null (in-place)
join(delim) -> str
slice(start, end?) -> list
map(fn) -> list
filter(fn) -> list
reduce(fn, init?) -> val
first() -> val/null
last() -> val/null
clear() -> null
toString() -> str
```

### Dict Methods
```sfpp
keys() -> list
values() -> list
has(key) / contains(key) -> bool
remove(key) -> null
size() / length() / len() -> int
```

### String Methods (via builtin functions)
See `std/string` module — `trim`, `split`, `replace`, etc.

---

## Environment & Variable Storage

```cpp
struct Environment {
    std::shared_ptr<Environment> parent;
    std::unordered_map<std::string, Variable> vars;

    void define(const std::string& name, ValuePtr val, bool isConst, std::string type);
    Variable* get(const std::string& name);  // searches parent chain
    Variable* getLocal(const std::string& name);
    void assign(const std::string& name, ValuePtr val);  // searches parent chain
}
```

```cpp
struct Variable {
    ValuePtr value;
    bool isConst;
    std::string type;  // optional type annotation
};
```

- Lexical scoping via parent chain
- `let` → `isConst=true`
- `var` / `auto` → `isConst=false`

---

## Interpreter Value Operations

Key methods in `Interpreter`:

```cpp
// Binary operations
ValuePtr applyBinaryArith(const std::string& op, ValuePtr l, ValuePtr r, int line);
ValuePtr applyBinaryCompare(const std::string& op, ValuePtr l, ValuePtr r, int line);

// Function calls
ValuePtr callFunction(std::shared_ptr<FunctionValue> fn, std::vector<ValuePtr> args, int line);
ValuePtr callMethod(std::shared_ptr<ClassInstance> inst, const std::string& name, std::vector<ValuePtr> args, int line);

// Built-in method dispatch
ValuePtr callBuiltinMethod(ValuePtr obj, const std::string& method, std::vector<ValuePtr> args, int line);
```

---

## Extending the Value System

To add a new runtime type:

1. **Add variant alternative** in `ValueVariant` (include/value.hpp)
2. **Add type predicate** (`isNewType()`)
3. **Add extractor** (`asNewType()`)
4. **Update `truthy()`, `toString()`, `typeName()`, `equals()`**
5. **Add factory** (`makeNewType()`)
6. **Handle in interpreter** (`callBuiltinMethod`, binary ops, etc.)
7. **Add serialization** if needed

---

## Common Patterns

### Creating Values in Native Code

```cpp
// In registerBuiltins() lambda:
defNative("myFunc", [](std::vector<ValuePtr> args) -> ValuePtr {
    // Args are already ValuePtr (Value objects)
    int64_t n = args[0]->asInt();
    return makeInt(n * 2);
});
```

### Working with Lists

```cpp
auto lv = std::make_shared<ListValue>();
lv->elements.push_back(makeInt(1));
lv->elements.push_back(makeStr("two"));
return makeList(lv);
```

### Working with Dicts

```cpp
auto dv = std::make_shared<DictValue>();
dv->set("key", makeInt(42));
return makeDict(dv);
```

### Throwing Errors

```cpp
throw RuntimeError("message", line);
throw TypeError("expected int, got " + args[0]->typeName());
throw MathError("division by zero");
throw IOError("file not found: " + path);
```

---

## Performance Notes

- `Value` is **not** a pointer — it's a value type containing a variant
- Copying `Value` copies the variant (cheap for primitives, ref-count for shared_ptr)
- Small int caching (0-256) avoids allocation
- `std::string` uses `shared_ptr` — copy-on-write semantics
- List/Dict/Set modifications mutate in-place (reference semantics)
- Function calls allocate new `Environment` for local scope

---

## Debugging Tips

```cpp
// Print value for debugging
std::cerr << val.toString() << " (type: " << val.typeName() << ")\n";

// Check type
if (val.isList()) {
    auto list = val.asList();
    for (auto& elem : list->elements) { ... }
}
```