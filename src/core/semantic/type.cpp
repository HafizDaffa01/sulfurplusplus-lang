#include "semantic/type.hpp"
#include <sstream>

bool Type::isInt() const {
    switch (kind) {
        case TypeKind::Int8:
        case TypeKind::Int16:
        case TypeKind::Int32:
        case TypeKind::Int64:
        case TypeKind::UInt8:
        case TypeKind::UInt16:
        case TypeKind::UInt32:
        case TypeKind::UInt64:
            return true;
        default:
            return false;
    }
}

bool Type::isFloat() const {
    return kind == TypeKind::Float32 || kind == TypeKind::Float64;
}

bool Type::isNumeric() const {
    return isInt() || isFloat();
}

bool Type::isString() const {
    return kind == TypeKind::String || kind == TypeKind::Char;
}

bool Type::isBool() const {
    return kind == TypeKind::Bool;
}

bool Type::isNull() const {
    return kind == TypeKind::Null;
}

bool Type::isComplex() const {
    return kind == TypeKind::Complex128;
}

bool Type::isAny() const {
    return kind == TypeKind::Any;
}

bool Type::isVoid() const {
    return kind == TypeKind::Void;
}

std::string Type::toString() const {
    if (!name.empty() && (kind == TypeKind::Class || kind == TypeKind::Struct)) {
        return name;
    }
    switch (kind) {
        case TypeKind::Unknown:    return "unknown";
        case TypeKind::Any:        return "any";
        case TypeKind::Void:       return "void";
        case TypeKind::Null:       return "null";
        case TypeKind::Bool:       return "bool";
        case TypeKind::Int8:       return "int_8";
        case TypeKind::Int16:      return "int_16";
        case TypeKind::Int32:      return "int_32";
        case TypeKind::Int64:      return "int_64";
        case TypeKind::UInt8:      return "uint_8";
        case TypeKind::UInt16:     return "uint_16";
        case TypeKind::UInt32:     return "uint_32";
        case TypeKind::UInt64:     return "uint_64";
        case TypeKind::Float32:    return "float_32";
        case TypeKind::Float64:    return "float_64";
        case TypeKind::Complex128: return "complex_128";
        case TypeKind::Char:       return "char";
        case TypeKind::String:     return "str";
        case TypeKind::List: {
            std::string elem = elementType ? elementType->toString() : "any";
            return "list<" + elem + ">";
        }
        case TypeKind::Dict: {
            std::string k = keyType ? keyType->toString() : "any";
            std::string v = valueType ? valueType->toString() : "any";
            return "dict<" + k + ", " + v + ">";
        }
        case TypeKind::Set: {
            std::string elem = elementType ? elementType->toString() : "any";
            return "set<" + elem + ">";
        }
        case TypeKind::Pointer: {
            std::string target = elementType ? elementType->toString() : "any";
            return "ptr<" + target + ">";
        }
        case TypeKind::Function: {
            std::string s = "fn(";
            for (size_t i = 0; i < paramTypes.size(); i++) {
                if (i > 0) s += ", ";
                s += paramTypes[i] ? paramTypes[i]->toString() : "any";
            }
            s += ")";
            if (returnType) s += " -> " + returnType->toString();
            return s;
        }
        case TypeKind::Class:      return "class " + name;
        case TypeKind::Struct:     return "struct " + name;
    }
    return "unknown";
}

bool Type::equals(const TypePtr& other) const {
    if (!other) return false;
    if (this->isAny() || other->isAny()) return true;
    if (this->kind != other->kind) return false;
    if (kind == TypeKind::Class || kind == TypeKind::Struct) {
        return this->name == other->name;
    }
    if (kind == TypeKind::List || kind == TypeKind::Set || kind == TypeKind::Pointer) {
        if (elementType && other->elementType) return elementType->equals(other->elementType);
        return true;
    }
    if (kind == TypeKind::Dict) {
        bool keq = (!keyType || !other->keyType) || keyType->equals(other->keyType);
        bool veq = (!valueType || !other->valueType) || valueType->equals(other->valueType);
        return keq && veq;
    }
    return true;
}

bool Type::isAssignableTo(const TypePtr& target) const {
    if (!target) return true;
    if (target->isAny() || this->isAny()) return true;
    if (this->equals(target)) return true;

    // Numeric widening
    if (this->isInt() && target->isInt()) return true;
    if (this->isFloat() && target->isFloat()) return true;
    if (this->isInt() && target->isFloat()) return true;
    if ((this->isInt() || this->isFloat()) && target->isComplex()) return true;

    // Null is assignable to Pointer, Class, Struct, Dict, List, Any
    if (this->isNull() && (target->kind == TypeKind::Pointer ||
                           target->kind == TypeKind::Class ||
                           target->kind == TypeKind::Struct ||
                           target->kind == TypeKind::List ||
                           target->kind == TypeKind::Dict ||
                           target->kind == TypeKind::Set)) {
        return true;
    }

    return false;
}

TypePtr makePrimitiveType(TypeKind kind) {
    return std::make_shared<Type>(kind);
}

TypePtr makeIntType() {
    return std::make_shared<Type>(TypeKind::Int64);
}

TypePtr makeFloatType() {
    return std::make_shared<Type>(TypeKind::Float64);
}

TypePtr makeStringType() {
    return std::make_shared<Type>(TypeKind::String);
}

TypePtr makeBoolType() {
    return std::make_shared<Type>(TypeKind::Bool);
}

TypePtr makeNullType() {
    return std::make_shared<Type>(TypeKind::Null);
}

TypePtr makeComplexType() {
    return std::make_shared<Type>(TypeKind::Complex128);
}

TypePtr makeVoidType() {
    return std::make_shared<Type>(TypeKind::Void);
}

TypePtr makeAnyType() {
    return std::make_shared<Type>(TypeKind::Any);
}

TypePtr makeUnknownType() {
    return std::make_shared<Type>(TypeKind::Unknown);
}

TypePtr makeListType(TypePtr elem) {
    auto t = std::make_shared<Type>(TypeKind::List);
    t->elementType = elem ? elem : makeAnyType();
    return t;
}

TypePtr makeDictType(TypePtr key, TypePtr val) {
    auto t = std::make_shared<Type>(TypeKind::Dict);
    t->keyType = key ? key : makeAnyType();
    t->valueType = val ? val : makeAnyType();
    return t;
}

TypePtr makeSetType(TypePtr elem) {
    auto t = std::make_shared<Type>(TypeKind::Set);
    t->elementType = elem ? elem : makeAnyType();
    return t;
}

TypePtr makePointerType(TypePtr target) {
    auto t = std::make_shared<Type>(TypeKind::Pointer);
    t->elementType = target ? target : makeAnyType();
    return t;
}

TypePtr makeFunctionType(std::vector<TypePtr> params, TypePtr ret) {
    auto t = std::make_shared<Type>(TypeKind::Function);
    t->paramTypes = std::move(params);
    t->returnType = ret ? ret : makeAnyType();
    return t;
}

TypePtr makeClassType(const std::string& name) {
    return std::make_shared<Type>(TypeKind::Class, name);
}

TypePtr makeStructType(const std::string& name) {
    return std::make_shared<Type>(TypeKind::Struct, name);
}

TypePtr parseTypeName(const std::string& typeStr) {
    if (typeStr.empty() || typeStr == "auto" || typeStr == "any" || typeStr == "var") return makeAnyType();
    if (typeStr == "void") return makeVoidType();
    if (typeStr == "null") return makeNullType();
    if (typeStr == "bool") return makeBoolType();
    if (typeStr == "int" || typeStr == "int_64" || typeStr == "int64") return makePrimitiveType(TypeKind::Int64);
    if (typeStr == "int_8" || typeStr == "int8") return makePrimitiveType(TypeKind::Int8);
    if (typeStr == "int_16" || typeStr == "int16") return makePrimitiveType(TypeKind::Int16);
    if (typeStr == "int_32" || typeStr == "int32") return makePrimitiveType(TypeKind::Int32);
    if (typeStr == "uint_8" || typeStr == "uint8") return makePrimitiveType(TypeKind::UInt8);
    if (typeStr == "uint_16" || typeStr == "uint16") return makePrimitiveType(TypeKind::UInt16);
    if (typeStr == "uint_32" || typeStr == "uint32") return makePrimitiveType(TypeKind::UInt32);
    if (typeStr == "uint_64" || typeStr == "uint64") return makePrimitiveType(TypeKind::UInt64);
    if (typeStr == "float" || typeStr == "float_64" || typeStr == "float64") return makePrimitiveType(TypeKind::Float64);
    if (typeStr == "float_32" || typeStr == "float32") return makePrimitiveType(TypeKind::Float32);
    if (typeStr == "complex" || typeStr == "complex_128" || typeStr == "complex128") return makeComplexType();
    if (typeStr == "char") return makePrimitiveType(TypeKind::Char);
    if (typeStr == "str" || typeStr == "string") return makeStringType();
    if (typeStr == "list") return makeListType(makeAnyType());
    if (typeStr == "dict") return makeDictType(makeAnyType(), makeAnyType());
    if (typeStr == "set") return makeSetType(makeAnyType());
    if (typeStr == "ptr") return makePointerType(makeAnyType());

    // Generic List<T>, Dict<K, V>, Ptr<T>
    if (typeStr.rfind("list<", 0) == 0 && typeStr.back() == '>') {
        std::string inner = typeStr.substr(5, typeStr.size() - 6);
        return makeListType(parseTypeName(inner));
    }
    if (typeStr.rfind("ptr<", 0) == 0 && typeStr.back() == '>') {
        std::string inner = typeStr.substr(4, typeStr.size() - 5);
        return makePointerType(parseTypeName(inner));
    }
    if (typeStr.rfind("set<", 0) == 0 && typeStr.back() == '>') {
        std::string inner = typeStr.substr(4, typeStr.size() - 5);
        return makeSetType(parseTypeName(inner));
    }
    if (typeStr.rfind("dict<", 0) == 0 && typeStr.back() == '>') {
        std::string inner = typeStr.substr(5, typeStr.size() - 6);
        size_t comma = inner.find(',');
        if (comma != std::string::npos) {
            std::string k = inner.substr(0, comma);
            std::string v = inner.substr(comma + 1);
            while (!v.empty() && v.front() == ' ') v.erase(v.begin());
            return makeDictType(parseTypeName(k), parseTypeName(v));
        }
        return makeDictType(makeAnyType(), makeAnyType());
    }

    return std::make_shared<Type>(TypeKind::Class, typeStr);
}

BinaryOpResult evaluateBinaryOp(const std::string& op, const TypePtr& left, const TypePtr& right, int /*line*/) {
    BinaryOpResult res;
    if (!left || !right) {
        res.valid = true;
        res.resultType = makeAnyType();
        return res;
    }

    if (left->isAny() || right->isAny()) {
        res.valid = true;
        res.resultType = makeAnyType();
        return res;
    }

    // --- Addition / Concatenation (+) ---
    if (op == "+") {
        if (left->isInt() && right->isInt()) {
            res.valid = true;
            res.resultType = makeIntType();
            return res;
        }
        if ((left->isInt() && right->isFloat()) || (left->isFloat() && right->isInt()) || (left->isFloat() && right->isFloat())) {
            res.valid = true;
            res.resultType = makeFloatType();
            return res;
        }
        if (left->isComplex() || right->isComplex()) {
            if (left->isNumeric() || left->isComplex()) {
                if (right->isNumeric() || right->isComplex()) {
                    res.valid = true;
                    res.resultType = makeComplexType();
                    return res;
                }
            }
        }
        if (left->isString() && right->isString()) {
            res.valid = true;
            res.resultType = makeStringType();
            return res;
        }
        if (left->kind == TypeKind::List && right->kind == TypeKind::List) {
            res.valid = true;
            res.resultType = left;
            return res;
        }

        // Strict mismatch for invalid combinations (e.g. int + str)
        res.valid = false;
        res.errorMessage = "cannot apply operator '+' to " + left->toString() + " and " + right->toString();
        return res;
    }

    // --- Subtraction, Multiplication, Division, Modulo, Power ---
    if (op == "-" || op == "*" || op == "/" || op == "%" || op == "**") {
        if (op == "*" && left->isString() && right->isInt()) {
            res.valid = true;
            res.resultType = makeStringType();
            return res;
        }
        if (left->isInt() && right->isInt()) {
            res.valid = true;
            res.resultType = (op == "/") ? makeFloatType() : makeIntType();
            return res;
        }
        if ((left->isInt() && right->isFloat()) || (left->isFloat() && right->isInt()) || (left->isFloat() && right->isFloat())) {
            res.valid = true;
            res.resultType = makeFloatType();
            return res;
        }
        if (left->isComplex() || right->isComplex()) {
            if ((left->isNumeric() || left->isComplex()) && (right->isNumeric() || right->isComplex())) {
                res.valid = true;
                res.resultType = makeComplexType();
                return res;
            }
        }

        res.valid = false;
        res.errorMessage = "cannot apply operator '" + op + "' to " + left->toString() + " and " + right->toString();
        return res;
    }

    // --- Bitwise Operators (&, |, ^, <<, >>) ---
    if (op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>") {
        if (left->isInt() && right->isInt()) {
            res.valid = true;
            res.resultType = makeIntType();
            return res;
        }
        res.valid = false;
        res.errorMessage = "bitwise operator '" + op + "' requires integer operands (got " + left->toString() + " and " + right->toString() + ")";
        return res;
    }

    // --- Equality Operators (==, !=) ---
    if (op == "==" || op == "!=") {
        res.valid = true;
        res.resultType = makeBoolType();
        return res;
    }

    // --- Comparison Operators (<, <=, >, >=) ---
    if (op == "<" || op == "<=" || op == ">" || op == ">=") {
        if (left->isNumeric() && right->isNumeric()) {
            res.valid = true;
            res.resultType = makeBoolType();
            return res;
        }
        if (left->isString() && right->isString()) {
            res.valid = true;
            res.resultType = makeBoolType();
            return res;
        }
        res.valid = false;
        res.errorMessage = "cannot compare " + left->toString() + " with " + right->toString();
        return res;
    }

    // --- Logical Operators (&&, ||) ---
    if (op == "&&" || op == "||") {
        res.valid = true;
        res.resultType = makeBoolType();
        return res;
    }

    // --- Null coalescing (??) ---
    if (op == "??") {
        res.valid = true;
        res.resultType = left->isNull() ? right : left;
        return res;
    }

    res.valid = true;
    res.resultType = makeAnyType();
    return res;
}

TypePtr evaluateUnaryOp(const std::string& op, const TypePtr& operand, int /*line*/) {
    if (!operand || operand->isAny()) return makeAnyType();
    if (op == "!") return makeBoolType();
    if (op == "-") {
        if (operand->isInt()) return makeIntType();
        if (operand->isFloat()) return makeFloatType();
        if (operand->isComplex()) return makeComplexType();
    }
    if (op == "~") {
        if (operand->isInt()) return makeIntType();
    }
    return makeAnyType();
}
