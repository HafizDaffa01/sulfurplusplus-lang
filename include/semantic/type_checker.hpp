#pragma once

#include "semantic/type.hpp"
#include "semantic/symbol_table.hpp"
#include "ast.hpp"

class TypeChecker {
public:
    explicit TypeChecker(SymbolTable& symTable) : symTable_(symTable) {}

    TypePtr inferExpression(const Expr& expr);
    bool checkAssignment(const TypePtr& targetType, const TypePtr& valueType, int line = 0);
    bool checkFunctionCall(const Symbol& fnSym, const std::vector<TypePtr>& argTypes, int line = 0);

private:
    SymbolTable& symTable_;
};
