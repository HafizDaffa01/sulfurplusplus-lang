#include "semantic/type_checker.hpp"
#include "error.hpp"

TypePtr TypeChecker::inferExpression(const Expr& expr) {
    return std::visit([&](const auto& node) -> TypePtr {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, IntLitExpr>) {
            return makePrimitiveType(TypeKind::Int64);
        }
        else if constexpr (std::is_same_v<T, FloatLitExpr>) {
            return makePrimitiveType(TypeKind::Float64);
        }
        else if constexpr (std::is_same_v<T, BoolLitExpr>) {
            return makePrimitiveType(TypeKind::Bool);
        }
        else if constexpr (std::is_same_v<T, NullLitExpr>) {
            return makePrimitiveType(TypeKind::Null);
        }
        else if constexpr (std::is_same_v<T, StringLitExpr> || std::is_same_v<T, PSStringExpr>) {
            return makePrimitiveType(TypeKind::String);
        }
        else if constexpr (std::is_same_v<T, CharLitExpr>) {
            return makePrimitiveType(TypeKind::Char);
        }
        else if constexpr (std::is_same_v<T, IdentExpr>) {
            Symbol* sym = symTable_.lookup(node.name);
            if (sym && sym->type) {
                return sym->type;
            }
            return makeAnyType();
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
            TypePtr leftType = inferExpression(*node.left);
            TypePtr rightType = inferExpression(*node.right);
            auto result = evaluateBinaryOp(node.op, leftType, rightType, node.line);
            if (!result.valid) {
                throw TypeError(result.errorMessage, node.line, "E_TYPE_406", "Check operand types for operator '" + node.op + "'", 1);
            }
            return result.resultType ? result.resultType : makeAnyType();
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>) {
            TypePtr opType = inferExpression(*node.operand);
            return evaluateUnaryOp(node.op, opType, node.line);
        }
        else if constexpr (std::is_same_v<T, AssignExpr>) {
            TypePtr valType = inferExpression(*node.value);
            return valType;
        }
        else if constexpr (std::is_same_v<T, ListLitExpr>) {
            TypePtr elemType = node.elements.empty() ? makeAnyType() : inferExpression(*node.elements[0]);
            return makeListType(elemType);
        }
        else if constexpr (std::is_same_v<T, DictLitExpr>) {
            return makeDictType(makeStringType(), makeAnyType());
        }
        else if constexpr (std::is_same_v<T, CallExpr>) {
            if (auto* id = std::get_if<IdentExpr>(&node.callee->data)) {
                Symbol* sym = symTable_.lookup(id->name);
                if (sym && sym->type && sym->type->returnType) {
                    return sym->type->returnType;
                }
            }
            return makeAnyType();
        }
        else if constexpr (std::is_same_v<T, NewExpr>) {
            return makeClassType(node.className);
        }
        else if constexpr (std::is_same_v<T, AddrOfExpr>) {
            TypePtr targetType = inferExpression(*node.operand);
            return makePointerType(targetType);
        }
        else if constexpr (std::is_same_v<T, DerefExpr>) {
            TypePtr ptrType = inferExpression(*node.operand);
            if (ptrType->kind == TypeKind::Pointer && ptrType->elementType) {
                return ptrType->elementType;
            }
            return makeAnyType();
        }
        else if constexpr (std::is_same_v<T, TernaryExpr>) {
            TypePtr thenType = inferExpression(*node.thenExpr);
            TypePtr elseType = inferExpression(*node.elseExpr);
            if (thenType->equals(elseType)) return thenType;
            return makeAnyType();
        }
        else if constexpr (std::is_same_v<T, LambdaExpr>) {
            std::vector<TypePtr> paramTypes;
            for (auto& p : node.params) {
                paramTypes.push_back(parseTypeName(p.second));
            }
            TypePtr ret = parseTypeName(node.retType);
            return makeFunctionType(std::move(paramTypes), ret);
        }

        return makeAnyType();
    }, expr.data);
}

bool TypeChecker::checkAssignment(const TypePtr& targetType, const TypePtr& valueType, int line) {
    if (!targetType || !valueType) return true;
    if (targetType->isAny() || valueType->isAny()) return true;
    if (!valueType->isAssignableTo(targetType)) {
        throw TypeError("cannot assign value of type " + valueType->toString() + " to target of type " + targetType->toString(),
                        line, "E_TYPE_406", "Ensure expression evaluates to " + targetType->toString(), 1);
    }
    return true;
}

bool TypeChecker::checkFunctionCall(const Symbol& fnSym, const std::vector<TypePtr>& argTypes, int line) {
    if (!fnSym.type || fnSym.type->kind != TypeKind::Function) return true;
    const auto& params = fnSym.type->paramTypes;
    if (!params.empty() && argTypes.size() != params.size()) {
        // Optional parameters handled at runtime
    }
    for (size_t i = 0; i < std::min(params.size(), argTypes.size()); i++) {
        if (!argTypes[i]->isAssignableTo(params[i])) {
            throw TypeError("argument " + std::to_string(i + 1) + " of function '" + fnSym.name +
                            "' expected " + params[i]->toString() + " (got " + argTypes[i]->toString() + ")",
                            line, "E_TYPE_406", "Pass an argument of type " + params[i]->toString(), 1);
        }
    }
    return true;
}
