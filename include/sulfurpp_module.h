/**
 * Sulfur++ Native Module C API
 * 
 * Include this header to create native modules for Sulfur++.
 * 
 * A native module is a shared library (.so/.dylib/.dll) that exports
 * a `sulfurpp_module_init` function.
 * 
 * Example:
 * ```c
 * #include "sulfurpp_module.h"
 * 
 * static ValuePtr my_func(Interpreter* interp, std::vector<ValuePtr> args) {
 *     return makeInt(42);
 * }
 * 
 * ValuePtr sulfurpp_module_init(Interpreter* interp) {
 *     auto dict = makeDict(new DictValue());
 *     dict->asDict()->set("my_func", makeFn(new FunctionValue{
 *         .name = "my_func",
 *         .isNative = true,
 *         .native = my_func
 *     }));
 *     return dict;
 * }
 * ```
 */

#ifndef SULFURPP_MODULE_H
#define SULFURPP_MODULE_H

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <variant>
#include <complex>
#include <functional>

// Forward declarations (opaque pointers)
struct Interpreter;
struct Environment;
struct FunctionValue;
struct ClassDef;
struct ClassInstance;
struct StructDef;
struct StructInstance;
struct ListValue;
struct SetValue;
struct DictValue;
struct PtrValue;

// Value type (matches interpreter's Value)
struct Value {
    using Variant = std::variant<
        std::monostate,           // null
        bool,                     // bool
        int64_t,                  // int_64
        double,                   // float_64
        std::complex<double>,     // complex_128
        std::shared_ptr<std::string>,  // str
        char,                     // char
        std::shared_ptr<FunctionValue>, // fn
        std::shared_ptr<ClassDef>,      // class
        std::shared_ptr<ClassInstance>, // instance
        std::shared_ptr<StructDef>,     // struct
        std::shared_ptr<StructInstance>,// struct_instance
        std::shared_ptr<ListValue>,     // list
        std::shared_ptr<SetValue>,      // set
        std::shared_ptr<DictValue>,     // dict
        std::shared_ptr<PtrValue>       // ptr
    >;

    Variant data;

    Value() : data(std::monostate{}) {}
    Value(bool b) : data(b) {}
    Value(int64_t i) : data(i) {}
    Value(double d) : data(d) {}
    Value(std::complex<double> c) : data(c) {}
    Value(const std::string& s) : data(std::make_shared<std::string>(s)) {}
    Value(std::shared_ptr<std::string> s) : data(s) {}
    Value(char c) : data(c) {}
    Value(std::shared_ptr<FunctionValue> f) : data(f) {}
    Value(std::shared_ptr<ClassDef> c) : data(c) {}
    Value(std::shared_ptr<ClassInstance> ci) : data(ci) {}
    Value(std::shared_ptr<StructDef> sd) : data(sd) {}
    Value(std::shared_ptr<StructInstance> si) : data(si) {}
    Value(std::shared_ptr<ListValue> lv) : data(lv) {}
    Value(std::shared_ptr<SetValue> sv) : data(sv) {}
    Value(std::shared_ptr<DictValue> dv) : data(dv) {}
    Value(std::shared_ptr<PtrValue> pv) : data(pv) {}

    // Type checks
    bool isNull() const { return std::holds_alternative<std::monostate>(data); }
    bool isBool() const { return std::holds_alternative<bool>(data); }
    bool isInt() const { return std::holds_alternative<int64_t>(data); }
    bool isFloat() const { return std::holds_alternative<double>(data); }
    bool isComplex() const { return std::holds_alternative<std::complex<double>>(data); }
    bool isStr() const { return std::holds_alternative<std::shared_ptr<std::string>>(data); }
    bool isChar() const { return std::holds_alternative<char>(data); }
    bool isFn() const { return std::holds_alternative<std::shared_ptr<FunctionValue>>(data); }
    bool isClassDef() const { return std::holds_alternative<std::shared_ptr<ClassDef>>(data); }
    bool isClassInst() const { return std::holds_alternative<std::shared_ptr<ClassInstance>>(data); }
    bool isStructDef() const { return std::holds_alternative<std::shared_ptr<StructDef>>(data); }
    bool isStructInst() const { return std::holds_alternative<std::shared_ptr<StructInstance>>(data); }
    bool isList() const { return std::holds_alternative<std::shared_ptr<ListValue>>(data); }
    bool isSet() const { return std::holds_alternative<std::shared_ptr<SetValue>>(data); }
    bool isDict() const { return std::holds_alternative<std::shared_ptr<DictValue>>(data); }
    bool isPtr() const { return std::holds_alternative<std::shared_ptr<PtrValue>>(data); }

    // Value extractors
    bool asBool() const { return std::get<bool>(data); }
    int64_t asInt() const { return std::get<int64_t>(data); }
    double asFloat() const { return std::get<double>(data); }
    std::complex<double> asComplex() const { return std::get<std::complex<double>>(data); }
    std::string asStr() const { 
        auto s = std::get<std::shared_ptr<std::string>>(data); 
        return s ? *s : ""; 
    }
    char asChar() const { return std::get<char>(data); }
    std::shared_ptr<FunctionValue> asFn() const { return std::get<std::shared_ptr<FunctionValue>>(data); }
    std::shared_ptr<ClassInstance> asClassInst() const { return std::get<std::shared_ptr<ClassInstance>>(data); }
    std::shared_ptr<StructInstance> asStructInst() const { return std::get<std::shared_ptr<StructInstance>>(data); }
    std::shared_ptr<ListValue> asList() const { return std::get<std::shared_ptr<ListValue>>(data); }
    std::shared_ptr<SetValue> asSet() const { return std::get<std::shared_ptr<SetValue>>(data); }
    std::shared_ptr<DictValue> asDict() const { return std::get<std::shared_ptr<DictValue>>(data); }
    std::shared_ptr<PtrValue> asPtr() const { return std::get<std::shared_ptr<PtrValue>>(data); }

    std::string toString() const; // implemented in interpreter
    std::string typeName() const;
    bool equals(const Value& other) const;
};

using ValuePtr = Value;

// Composite value structures
struct ListValue {
    std::vector<ValuePtr> elements;
};

struct SetValue {
    std::vector<ValuePtr> elements;
};

struct DictValue {
    std::unordered_map<std::string, ValuePtr> pairs;
    
    ValuePtr get(const std::string& key) const {
        auto it = pairs.find(key);
        return it != pairs.end() ? it->second : ValuePtr();
    }
    
    void set(const std::string& key, ValuePtr val) {
        pairs[key] = val;
    }
    
    bool has(const std::string& key) const {
        return pairs.find(key) != pairs.end();
    }
};

struct PtrValue {
    ValuePtr* target = nullptr;
    void* rawPtr = nullptr;
};

struct FunctionValue {
    std::string name;
    std::vector<std::pair<std::string, std::string>> params;
    std::string retType;
    void* body = nullptr;  // Stmt* (opaque)
    void* decl = nullptr;  // FnDeclStmt* (opaque)
    std::shared_ptr<Environment> closure;
    bool isNative = false;
    int callCount = 0;
    std::function<ValuePtr(Interpreter*, std::vector<ValuePtr>)> native;
    std::string definedInFile;
};

// Factory functions (inline implementations use interpreter internals)
inline ValuePtr makeNull() { return ValuePtr(); }
inline ValuePtr makeBool(bool b) { return ValuePtr(b); }
inline ValuePtr makeInt(int64_t i) { return ValuePtr(i); }
inline ValuePtr makeFloat(double d) { return ValuePtr(d); }
inline ValuePtr makeComplex(std::complex<double> c) { return ValuePtr(c); }
inline ValuePtr makeStr(const std::string& s) { return ValuePtr(s); }
inline ValuePtr makeStr(std::shared_ptr<std::string> s) { return ValuePtr(s); }
inline ValuePtr makeChar(char c) { return ValuePtr(c); }
inline ValuePtr makeList(std::shared_ptr<ListValue> lv) { return ValuePtr(lv); }
inline ValuePtr makeSet(std::shared_ptr<SetValue> sv) { return ValuePtr(sv); }
inline ValuePtr makeDict(std::shared_ptr<DictValue> dv) { return ValuePtr(dv); }
inline ValuePtr makeFn(std::shared_ptr<FunctionValue> f) { return ValuePtr(f); }
inline ValuePtr makePtr(ValuePtr* target) { 
    auto pv = std::make_shared<PtrValue>();
    pv->target = target;
    return ValuePtr(pv); 
}
inline ValuePtr makeRawPtr(void* ptr) { 
    auto pv = std::make_shared<PtrValue>();
    pv->rawPtr = ptr;
    return ValuePtr(pv); 
}

// Interpreter interface for native functions
struct Interpreter {
    // Function calling
    ValuePtr callFunction(std::shared_ptr<FunctionValue> fn,
                          std::vector<ValuePtr> args, int line);
    
    // Error throwing
    void throwRuntimeError(const std::string& msg, int line);
    void throwTypeError(const std::string& msg, int line);
    void throwMathError(const std::string& msg, int line);
    void throwIOError(const std::string& msg, int line);
    
    // Output
    void print(const std::string& s);
    void printErr(const std::string& s);
    std::string readLine();
    
    // Builtin method dispatch
    ValuePtr callBuiltinMethod(ValuePtr obj, const std::string& method,
                               std::vector<ValuePtr> args, int line);
};

// Module init function signature
// Must be exported from the shared library
// extern "C" ValuePtr sulfurpp_module_init(Interpreter* interp);

#define SULFURPP_MODULE_INIT extern "C" ValuePtr sulfurpp_module_init

// Convenience macros for common patterns
#define SULFURPP_NATIVE_FUNC(name) \
    static ValuePtr name(Interpreter* interp, std::vector<ValuePtr> args)

#define SULFURPP_REGISTER_FUNC(dict, name, func) \
    do { \
        auto fv = std::make_shared<FunctionValue>(); \
        fv->name = #name; \
        fv->isNative = true; \
        fv->native = [](Interpreter* i, std::vector<ValuePtr> a) -> ValuePtr { \
            return func(i, a); \
        }; \
        (dict)->asDict()->set(#name, makeFn(fv)); \
    } while(0)

#define SULFURPP_REGISTER_CONST(dict, name, val) \
    do { (dict)->asDict()->set(#name, val); } while(0)

#endif // SULFURPP_MODULE_H