#include "../include/value.hpp"
#include <cmath>
#include <sstream>
#include <stdexcept>

// ─── DictValue
// ────────────────────────────────────────────────────────────────

ValuePtr DictValue::get(const std::string &key) const {
  for (auto &p : pairs) {
    if (p.first.isStr() && p.first.asStr() == key)
      return p.second;
  }
  return makeNull();
}

bool DictValue::has(const std::string &key) const {
  for (auto &p : pairs) {
    if (p.first.isStr() && p.first.asStr() == key)
      return true;
  }
  return false;
}

void DictValue::set(const std::string &key, ValuePtr val) {
  for (auto &p : pairs) {
    if (p.first.isStr() && p.first.asStr() == key) {
      p.second = val;
      return;
    }
  }
  pairs.push_back({makeStr(key), val});
}

// ─── Value accessors
// ──────────────────────────────────────────────────────────

bool Value::asBool() const {
  if (isBool())
    return std::get<bool>(data);
  throw std::runtime_error("Value is not a bool");
}

int64_t Value::asInt() const {
  if (isInt())
    return std::get<int64_t>(data);
  if (isFloat())
    return static_cast<int64_t>(std::get<double>(data));
  throw std::runtime_error("Value is not an int");
}

double Value::asFloat() const {
  if (isFloat())
    return std::get<double>(data);
  if (isInt())
    return static_cast<double>(std::get<int64_t>(data));
  throw std::runtime_error("Value is not a float");
}

std::string Value::asStr() const {
  if (isStr()) {
    auto s = std::get<std::shared_ptr<std::string>>(data);
    return s ? *s : "";
  }
  throw std::runtime_error("Value is not a string");
}

char Value::asChar() const {
  if (isChar())
    return std::get<char>(data);
  throw std::runtime_error("Value is not a char");
}

std::shared_ptr<FunctionValue> Value::asFn() const {
  if (isFn())
    return std::get<std::shared_ptr<FunctionValue>>(data);
  throw std::runtime_error("Value is not a function");
}

std::shared_ptr<ClassInstance> Value::asClassInst() const {
  if (isClassInst())
    return std::get<std::shared_ptr<ClassInstance>>(data);
  throw std::runtime_error("Value is not a class instance");
}

std::shared_ptr<StructInstance> Value::asStructInst() const {
  if (isStructInst())
    return std::get<std::shared_ptr<StructInstance>>(data);
  throw std::runtime_error("Value is not a struct instance");
}

std::shared_ptr<ListValue> Value::asList() const {
  if (isList())
    return std::get<std::shared_ptr<ListValue>>(data);
  throw std::runtime_error("Value is not a list");
}

std::shared_ptr<SetValue> Value::asSet() const {
  if (isSet())
    return std::get<std::shared_ptr<SetValue>>(data);
  throw std::runtime_error("Value is not a set");
}

std::shared_ptr<DictValue> Value::asDict() const {
  if (isDict())
    return std::get<std::shared_ptr<DictValue>>(data);
  throw std::runtime_error("Value is not a dict");
}

std::shared_ptr<PtrValue> Value::asPtr() const {
  if (isPtr())
    return std::get<std::shared_ptr<PtrValue>>(data);
  throw std::runtime_error("Value is not a pointer");
}

// ─── Truthy
// ───────────────────────────────────────────────────────────────────

bool Value::truthy() const {
  return std::visit(
      [](const auto &v) -> bool {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>)
          return false;
        if constexpr (std::is_same_v<T, bool>)
          return v;
        if constexpr (std::is_same_v<T, int64_t>)
          return v != 0;
        if constexpr (std::is_same_v<T, double>)
          return v != 0.0 && !std::isnan(v);
        if constexpr (std::is_same_v<T, std::shared_ptr<std::string>>)
          return v && !v->empty();
        if constexpr (std::is_same_v<T, char>)
          return v != '\0';
        if constexpr (std::is_same_v<T, std::shared_ptr<ListValue>>)
          return v && !v->elements.empty();
        if constexpr (std::is_same_v<T, std::shared_ptr<DictValue>>)
          return v && !v->pairs.empty();
        return true;
      },
      data);
}

// ─── toString
// ─────────────────────────────────────────────────────────────────

std::string Value::toString() const {
  return std::visit(
      [](const auto &v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>)
          return "null";
        if constexpr (std::is_same_v<T, bool>)
          return v ? "true" : "false";
        if constexpr (std::is_same_v<T, int64_t>)
          return std::to_string(v);
        if constexpr (std::is_same_v<T, double>) {
          std::ostringstream oss;
          double intpart;
          if (std::modf(v, &intpart) == 0.0 && std::abs(v) < 1e15)
            oss << intpart;
          else
            oss << v;
          return oss.str();
        }
        if constexpr (std::is_same_v<T, std::shared_ptr<std::string>>)
          return v ? *v : "";
        if constexpr (std::is_same_v<T, char>)
          return std::string(1, v);
        if constexpr (std::is_same_v<T, std::shared_ptr<FunctionValue>>)
          return "<fn " + (v ? v->name : "?") + ">";
        if constexpr (std::is_same_v<T, std::shared_ptr<ClassDef>>)
          return "<class " + (v ? v->name : "?") + ">";
        if constexpr (std::is_same_v<T, std::shared_ptr<ClassInstance>>) {
          if (!v || !v->def)
            return "<instance>";
          return "<" + v->def->name + " instance>";
        }
        if constexpr (std::is_same_v<T, std::shared_ptr<StructDef>>)
          return "<struct " + (v ? v->name : "?") + ">";
        if constexpr (std::is_same_v<T, std::shared_ptr<StructInstance>>) {
          if (!v || !v->def)
            return "<struct instance>";
          std::string s = v->def->name + "{";
          bool first = true;
          for (auto &[k, val] : v->fields) {
            if (!first)
              s += ", ";
            s += k + ": " + val.toString();
            first = false;
          }
          return s + "}";
        }
        if constexpr (std::is_same_v<T, std::shared_ptr<ListValue>>) {
          if (!v)
            return "[]";
          std::string s = "[";
          for (size_t i = 0; i < v->elements.size(); i++) {
            if (i)
              s += ", ";
            s += v->elements[i].toString();
          }
          return s + "]";
        }
        if constexpr (std::is_same_v<T, std::shared_ptr<SetValue>>) {
          if (!v)
            return "{}";
          std::string s = "set{";
          for (size_t i = 0; i < v->elements.size(); i++) {
            if (i)
              s += ", ";
            s += v->elements[i].toString();
          }
          return s + "}";
        }
        if constexpr (std::is_same_v<T, std::shared_ptr<DictValue>>) {
          if (!v)
            return "{}";
          std::string s = "{";
          for (size_t i = 0; i < v->pairs.size(); i++) {
            if (i)
              s += ", ";
            s += v->pairs[i].first.toString();
            s += ": ";
            s += v->pairs[i].second.toString();
          }
          return s + "}";
        }
        if constexpr (std::is_same_v<T, std::shared_ptr<PtrValue>>)
          return "<ptr>";
        return "<unknown>";
      },
      data);
}

// ─── typeName
// ─────────────────────────────────────────────────────────────────

std::string Value::typeName() const {
  return std::visit(
      [](const auto &v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>)
          return "null";
        if constexpr (std::is_same_v<T, bool>)
          return "bool";
        if constexpr (std::is_same_v<T, int64_t>)
          return "int_64";
        if constexpr (std::is_same_v<T, double>)
          return "float_64";
        if constexpr (std::is_same_v<T, std::shared_ptr<std::string>>)
          return "str";
        if constexpr (std::is_same_v<T, char>)
          return "char";
        if constexpr (std::is_same_v<T, std::shared_ptr<FunctionValue>>)
          return "fn";
        if constexpr (std::is_same_v<T, std::shared_ptr<ClassDef>>)
          return "class";
        if constexpr (std::is_same_v<T, std::shared_ptr<ClassInstance>>)
          return "instance";
        if constexpr (std::is_same_v<T, std::shared_ptr<StructDef>>)
          return "struct";
        if constexpr (std::is_same_v<T, std::shared_ptr<StructInstance>>)
          return "struct_instance";
        if constexpr (std::is_same_v<T, std::shared_ptr<ListValue>>)
          return "list";
        if constexpr (std::is_same_v<T, std::shared_ptr<SetValue>>)
          return "set";
        if constexpr (std::is_same_v<T, std::shared_ptr<DictValue>>)
          return "dict";
        if constexpr (std::is_same_v<T, std::shared_ptr<PtrValue>>)
          return "ptr";
        return "unknown";
      },
      data);
}

// ─── equals
// ───────────────────────────────────────────────────────────────────

bool Value::equals(const Value &other) const {
  if (isNull() && other.isNull())
    return true;
  if (isBool() && other.isBool())
    return asBool() == other.asBool();
  if (isInt() && other.isInt())
    return asInt() == other.asInt();
  if (isFloat() && other.isFloat())
    return asFloat() == other.asFloat();
  if (isInt() && other.isFloat())
    return (double)asInt() == other.asFloat();
  if (isFloat() && other.isInt())
    return asFloat() == (double)other.asInt();
  if (isStr() && other.isStr())
    return asStr() == other.asStr();
  if (isChar() && other.isChar())
    return asChar() == other.asChar();
  return false;
}
