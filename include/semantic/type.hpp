#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

enum class TypeKind {
    Unknown,
    Any,
    Void,
    Null,
    Bool,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float32,
    Float64,
    Complex128,
    Char,
    String,
    List,
    Dict,
    Set,
    Function,
    Pointer,
    Class,
    Struct
};

struct Type;
using TypePtr = std::shared_ptr<Type>;

struct Type {
    TypeKind kind = TypeKind::Unknown;
    std::string name; // Custom name (e.g. class or struct name)

    // For List<T>, Set<T>, Pointer<T>
    TypePtr elementType = nullptr;

    // For Dict<K, V>
    TypePtr keyType = nullptr;
    TypePtr valueType = nullptr;

    // For Function(params) -> returnType
    std::vector<TypePtr> paramTypes;
    TypePtr returnType = nullptr;

    // For Class / Struct member types
    std::unordered_map<std::string, TypePtr> members;

    explicit Type(TypeKind k = TypeKind::Unknown, const std::string& n = "")
        : kind(k), name(n) {}

    bool isInt() const;
    bool isFloat() const;
    bool isNumeric() const;
    bool isString() const;
    bool isBool() const;
    bool isNull() const;
    bool isComplex() const;
    bool isAny() const;
    bool isVoid() const;

    std::string toString() const;
    bool equals(const TypePtr& other) const;
    bool isAssignableTo(const TypePtr& target) const;
};

// Factory functions
TypePtr makePrimitiveType(TypeKind kind);
TypePtr makeIntType();
TypePtr makeFloatType();
TypePtr makeStringType();
TypePtr makeBoolType();
TypePtr makeNullType();
TypePtr makeComplexType();
TypePtr makeVoidType();
TypePtr makeAnyType();
TypePtr makeUnknownType();

TypePtr makeListType(TypePtr elem);
TypePtr makeDictType(TypePtr key, TypePtr val);
TypePtr makeSetType(TypePtr elem);
TypePtr makePointerType(TypePtr target);
TypePtr makeFunctionType(std::vector<TypePtr> params, TypePtr ret);
TypePtr makeClassType(const std::string& name);
TypePtr makeStructType(const std::string& name);

TypePtr parseTypeName(const std::string& typeStr);

// Central Operator Type Matrix
struct BinaryOpResult {
    bool valid = false;
    TypePtr resultType = nullptr;
    std::string errorMessage;
};

BinaryOpResult evaluateBinaryOp(const std::string& op, const TypePtr& left, const TypePtr& right, int line = 0);
TypePtr evaluateUnaryOp(const std::string& op, const TypePtr& operand, int line = 0);
