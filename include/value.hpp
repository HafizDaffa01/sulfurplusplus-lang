#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <memory>
#include <variant>
#include <functional>
#include <stdexcept>
#include <complex>

struct Value;
using ValuePtr = Value;

struct Environment;

struct FunctionValue {
    std::string name;
    std::vector<std::pair<std::string,std::string>> params;
    std::string retType;
    void* body; // Stmt* (opaque to avoid circular include)
    std::shared_ptr<Environment> closure;
    bool isNative = false;
    std::function<ValuePtr(std::vector<ValuePtr>)> native;
    std::string definedInFile;
};

struct ClassDef {
    std::string name;
    std::vector<std::string> interfaces;
    std::shared_ptr<Environment> methods;
    std::vector<std::pair<std::string,std::string>> fields;
    std::vector<std::pair<int,std::string>> ctorOrder;
    std::vector<std::pair<int,std::string>> dtorOrder;
};

struct ClassInstance {
    std::shared_ptr<ClassDef> def;
    std::shared_ptr<Environment> members;
};

struct StructDef {
    std::string name;
    std::vector<std::pair<std::string,std::string>> fields;
};

struct StructInstance {
    std::shared_ptr<StructDef> def;
    std::map<std::string, ValuePtr> fields;
};

struct ListValue {
    std::vector<ValuePtr> elements;
};

struct SetValue {
    std::vector<ValuePtr> elements; // ordered for simplicity
};

struct DictValue {
    std::unordered_map<std::string, ValuePtr> pairs;

    ValuePtr get(const std::string& key) const;
    void set(const std::string& key, ValuePtr val);
    bool has(const std::string& key) const;
};

// Pointer value (for unsafe blocks)
struct PtrValue {
    ValuePtr* target;
};

using ValueVariant = std::variant<
    std::monostate,       // null
    bool,
    int64_t,
    double,
    std::complex<double>,
    std::shared_ptr<std::string>,
    char,
    std::shared_ptr<FunctionValue>,
    std::shared_ptr<ClassDef>,
    std::shared_ptr<ClassInstance>,
    std::shared_ptr<StructDef>,
    std::shared_ptr<StructInstance>,
    std::shared_ptr<ListValue>,
    std::shared_ptr<SetValue>,
    std::shared_ptr<DictValue>,
    std::shared_ptr<PtrValue>
>;

struct Value {
    ValueVariant data;

    Value() : data(std::monostate{}) {}
    explicit Value(bool b) : data(b) {}
    explicit Value(int64_t i) : data(i) {}
    explicit Value(double d) : data(d) {}
    explicit Value(std::complex<double> c) : data(c) {}
    explicit Value(const std::string& s) : data(std::make_shared<std::string>(s)) {}
    explicit Value(std::shared_ptr<std::string> s) : data(s) {}
    explicit Value(char c) : data(c) {}
    explicit Value(std::shared_ptr<FunctionValue> f) : data(f) {}
    explicit Value(std::shared_ptr<ClassDef> c) : data(c) {}
    explicit Value(std::shared_ptr<ClassInstance> ci) : data(ci) {}
    explicit Value(std::shared_ptr<StructDef> sd) : data(sd) {}
    explicit Value(std::shared_ptr<StructInstance> si) : data(si) {}
    explicit Value(std::shared_ptr<ListValue> lv) : data(lv) {}
    explicit Value(std::shared_ptr<SetValue> sv) : data(sv) {}
    explicit Value(std::shared_ptr<DictValue> dv) : data(dv) {}
    explicit Value(std::shared_ptr<PtrValue> pv) : data(pv) {}

    Value* operator->() { return this; }
    const Value* operator->() const { return this; }
    bool isNull() const { return std::holds_alternative<std::monostate>(data); }

    bool isBool() const { return std::holds_alternative<bool>(data); }
    bool isInt() const  { return std::holds_alternative<int64_t>(data); }
    bool isFloat() const{ return std::holds_alternative<double>(data); }
    bool isComplex() const { return std::holds_alternative<std::complex<double>>(data); }
    bool isStr() const  { return std::holds_alternative<std::shared_ptr<std::string>>(data); }
    bool isChar() const { return std::holds_alternative<char>(data); }
    bool isList() const { return std::holds_alternative<std::shared_ptr<ListValue>>(data); }
    bool isDict() const { return std::holds_alternative<std::shared_ptr<DictValue>>(data); }
    bool isSet()  const { return std::holds_alternative<std::shared_ptr<SetValue>>(data); }
    bool isFn()   const { return std::holds_alternative<std::shared_ptr<FunctionValue>>(data); }
    bool isClassDef() const { return std::holds_alternative<std::shared_ptr<ClassDef>>(data); }
    bool isClassInst() const{ return std::holds_alternative<std::shared_ptr<ClassInstance>>(data); }
    bool isStructDef() const{ return std::holds_alternative<std::shared_ptr<StructDef>>(data); }
    bool isStructInst() const{ return std::holds_alternative<std::shared_ptr<StructInstance>>(data); }

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
    std::shared_ptr<SetValue>  asSet()  const;
    std::shared_ptr<DictValue> asDict() const;
    bool isPtr() const { return std::holds_alternative<std::shared_ptr<PtrValue>>(data); }
    std::shared_ptr<PtrValue> asPtr() const;

    bool truthy() const;
    std::string toString() const;
    std::string typeName() const;
    bool equals(const Value& other) const;
};

inline ValuePtr makeNull();
inline ValuePtr makeBool(bool b);
inline ValuePtr makeInt(int64_t i);
inline ValuePtr makeFloat(double d)   { return Value(d); }
inline ValuePtr makeComplex(std::complex<double> c) { return Value(c); }
inline ValuePtr makeStr(const std::string& s) { return Value(s); }
inline ValuePtr makeChar(char c)      { return Value(c); }
inline ValuePtr makeList(std::shared_ptr<ListValue> lv) { return Value(lv); }
inline ValuePtr makeDict(std::shared_ptr<DictValue> dv) { return Value(dv); }
inline ValuePtr makeSet(std::shared_ptr<SetValue> sv)   { return Value(sv); }
inline ValuePtr makeFn(std::shared_ptr<FunctionValue> f){ return Value(f); }
inline ValuePtr makeClassDef(std::shared_ptr<ClassDef> c) { return Value(c); }
inline ValuePtr makeClassInst(std::shared_ptr<ClassInstance> ci) { return Value(ci); }
inline ValuePtr makeStructDef(std::shared_ptr<StructDef> sd) { return Value(sd); }
inline ValuePtr makeStructInst(std::shared_ptr<StructInstance> si) { return Value(si); }
inline ValuePtr makePtr(ValuePtr *target) {
  auto pv = std::make_shared<PtrValue>();
  pv->target = target;
  return Value(pv);
}

// Optimization: Pre-allocate common values
struct ValueCache {
    Value nullVal;
    Value trueVal;
    Value falseVal;
    std::vector<Value> smallInts;

    static ValueCache& get() {
        static ValueCache instance;
        return instance;
    }

private:
    ValueCache() : nullVal(), trueVal(true), falseVal(false) {
        smallInts.reserve(257);
        for (int i = 0; i <= 256; i++) {
            smallInts.push_back(Value((int64_t)i));
        }
    }
};

inline ValuePtr makeNull()  { return ValueCache::get().nullVal; }
inline ValuePtr makeBool(bool b)      { return b ? ValueCache::get().trueVal : ValueCache::get().falseVal; }
inline ValuePtr makeInt(int64_t i)    {
    if (i >= 0 && i <= 256) return ValueCache::get().smallInts[i];
    return Value(i);
}
