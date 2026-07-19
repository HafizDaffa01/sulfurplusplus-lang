# Parser & AST Documentation

This document describes the internal architecture of the Sulfur++ parser, Abstract Syntax Tree (AST) node definitions, and the parsing pipeline.

---

## Overview

The Sulfur++ parser is a **recursive descent parser** with **Pratt-style expression parsing** for handling operator precedence. It transforms a token stream (from the lexer) into a structured AST.

### Files

- `include/ast.hpp` — AST node definitions (317 lines)
- `include/parser.hpp` — Parser class declaration (67 lines)
- `src/core/parser.cpp` — Parser implementation (1026 lines)

---

## AST Node Definitions

All AST nodes are defined as `struct` types in `include/ast.hpp`. Nodes are wrapped in `std::unique_ptr` with type aliases:
- `ExprPtr = std::unique_ptr<Expr>`
- `StmtPtr = std::unique_ptr<Stmt>`

The `Expr` and `Stmt` structs use `std::variant` to hold any concrete node type, providing type-safe pattern matching via `std::visit`.

---

### Expression Nodes

| Node | Fields | Description |
|------|--------|-------------|
| `IntLitExpr` | `int64_t value`, `int line` | Integer literal |
| `FloatLitExpr` | `double value`, `int line` | Float literal |
| `BoolLitExpr` | `bool value`, `int line` | Boolean literal |
| `NullLitExpr` | `int line` | `null` literal |
| `StringLitExpr` | `std::string value`, `int line` | String literal `"..."` |
| `CharLitExpr` | `char value`, `int line` | Character literal `'c'` |
| `PSStringExpr` | `std::vector<PSSegment> segments`, `int line` | Template string `ps"..."` |
| `IdentExpr` | `std::string name`, `int line` | Identifier reference |
| `BinaryExpr` | `std::string op`, `ExprPtr left`, `ExprPtr right`, `int line` | Binary operation (`a + b`) |
| `UnaryExpr` | `std::string op`, `ExprPtr operand`, `int line` | Unary operation (`-x`, `!x`) |
| `AssignExpr` | `ExprPtr target`, `std::string op`, `ExprPtr value`, `int line` | Assignment (`x = y`, `x += y`) |
| `CallExpr` | `ExprPtr callee`, `std::vector<ExprPtr> args`, `int line` | Function call (`fn(a, b)`) |
| `IndexExpr` | `ExprPtr object`, `ExprPtr index`, `int line` | Index access (`arr[i]`, `dict[key]`) |
| `MemberExpr` | `ExprPtr object`, `std::string member`, `bool safe`, `std::string op`, `int line` | Member access (`obj.field`, `obj?.field`, `obj::field`, `obj->field`) |
| `PipelineExpr` | `ExprPtr left`, `ExprPtr right`, `int line` | Pipeline (`a -> b`) |
| `NullCoalExpr` | `ExprPtr left`, `ExprPtr right`, `int line` | Null coalescing (`a ?? b`) |
| `ListLitExpr` | `std::vector<ExprPtr> elements`, `int line` | List literal `[a, b, c]` |
| `DictLitExpr` | `std::vector<std::pair<ExprPtr, ExprPtr>> pairs`, `int line` | Dict literal `{k: v}` |
| `LambdaExpr` | `std::vector<std::pair<std::string, std::string>> params`, `std::string retType`, `StmtPtr body`, `int line` | Anonymous function |
| `NewExpr` | `std::string className`, `std::vector<ExprPtr> args`, `int line` | Class instantiation `new Class(args)` |
| `TernaryExpr` | `ExprPtr cond`, `ExprPtr thenExpr`, `ExprPtr elseExpr`, `int line` | Ternary `cond ? a : b` |
| `AddrOfExpr` | `ExprPtr operand`, `int line` | Address-of `&x` (unsafe) |
| `DerefExpr` | `ExprPtr operand`, `int line` | Dereference `*x` (unsafe) |
| `DeleteExpr` | `ExprPtr operand`, `int line` | Delete `delete x` (unsafe) |

**PSSegment** (for `ps"..."` strings):
```cpp
struct PSSegment {
    bool isExpr;
    std::string text;      // for plain text
    ExprPtr expr;          // for {expression}
    std::string fmtSpec;   // optional format: :join(','), :repeat(3), etc.
};
```

---

### Statement Nodes

| Node | Fields | Description |
|------|--------|-------------|
| `VarDeclStmt` | `keyword` (let/var/auto), `name`, `type`, `initializer`, `line` | Variable declaration |
| `FnDeclStmt` | `name`, `params` (vector of name,type), `retType`, `body`, `isMethod`, `line` | Function declaration |
| `ReturnStmt` | `ExprPtr value`, `line` | Return statement |
| `BreakStmt` | `line` | Break loop |
| `ContinueStmt` | `line` | Continue loop |
| `BlockStmt` | `std::vector<StmtPtr> stmts`, `line` | `{ ... }` block |
| `IfStmt` | `ExprPtr cond`, `StmtPtr thenBranch`, `StmtPtr elseBranch`, `line` | If/else |
| `WhileStmt` | `ExprPtr cond`, `StmtPtr body`, `line` | While loop |
| `ForStmt` | `std::string var`, `ExprPtr iterable`, `StmtPtr body`, `line` | For-in loop |
| `ClassDeclStmt` | `name`, `interfaces`, `members`, `ctorOrder`, `dtorOrder`, `line` | Class definition |
| `StructDeclStmt` | `name`, `fields` (vector of name,type), `line` | Struct definition |
| `InterfaceDeclStmt` | `name`, `methods` (FnDeclStmt), `line` | Interface definition |
| `ExprStmt` | `ExprPtr expr`, `line` | Expression as statement |
| `StreamOutStmt` | `ExprPtr target`, `ExprPtr value`, `line` | `Terminal.Out << value` |
| `ImportStmt` | `pkg`, `alias`, `flags` (vector), `line` | `import ... as ...` |
| `ExportStmt` | `alias`, `line` | `export this as ...` |
| `ExposeStmt` | `name`, `alias`, `line` | `expose "native" as alias` |
| `OverwriteStmt` | `target`, `ExprPtr value`, `line` | `overwrite native = replacement` |
| `UnsafeStmt` | `StmtPtr body`, `line` | `unsafe { ... }` |
| `DeferStmt` | `StmtPtr body`, `line` | `defer { ... }` |
| `TryCatchStmt` | `tryBody`, `catchVar`, `catchBody`, `finallyBody`, `line` | Try/catch/finally |
| `ThrowStmt` | `ExprPtr value`, `line` | `throw expr` |
| `MatchStmt` | `ExprPtr value`, `std::vector<MatchCase> cases`, `line` | Match expression |
| `MatchCase` | `ExprPtr pattern` (null = wildcard), `StmtPtr body`, `line` | One match case |

---

## Parser Architecture

### Class: `Parser`

```cpp
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::vector<StmtPtr> parse();

private:
    std::vector<Token> tokens_;
    size_t pos_ = 0;

    // Token navigation
    Token& peek(int offset = 0);
    Token& advance();
    bool check(TokenType t) const;
    bool match(TokenType t);
    bool match(std::initializer_list<TokenType> types);
    Token expect(TokenType t, const std::string& msg);
    bool isAtEnd() const;

    // Statement parsing
    StmtPtr parseStmt();
    StmtPtr parseVarDecl(const std::string& keyword);
    StmtPtr parseFnDecl(bool isMethod = false);
    StmtPtr parseClassDecl();
    StmtPtr parseStructDecl();
    StmtPtr parseInterfaceDecl();
    StmtPtr parseBlock();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseFor();
    StmtPtr parseReturn();
    StmtPtr parseThrow();
    StmtPtr parseImport();
    StmtPtr parseExport();
    StmtPtr parseExpose();
    StmtPtr parseOverwrite();
    StmtPtr parseUnsafe();
    StmtPtr parseDefer();
    StmtPtr parseTryCatch();
    StmtPtr parseMatch();
    StmtPtr parseExprStmt();

    // Expression parsing (Pratt precedence climbing)
    ExprPtr parseExpr();           // = parseAssign()
    ExprPtr parseAssign();
    ExprPtr parseTernary();
    ExprPtr parseNullCoal();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseAddSub();
    ExprPtr parseShift();
    ExprPtr parseMulDiv();
    ExprPtr parseUnary();
    ExprPtr parsePipeline();
    ExprPtr parsePostfix();
    ExprPtr parsePrimary();
    ExprPtr parsePSString(const std::string& raw, int line);

    // Helpers
    std::vector<std::pair<std::string,std::string>> parseParamList();
    std::string parseType();
};
```

### Parsing Flow

```
parse()                    // Entry point: parse all statements until EOF
  └─ parseStmt()           // Dispatch based on current token
       ├─ Keywords: let/var/auto -> parseVarDecl()
       ├─ fn              -> parseFnDecl()
       ├─ class           -> parseClassDecl()
       ├─ struct          -> parseStructDecl()
       ├─ interface       -> parseInterfaceDecl()
       ├─ {               -> parseBlock()
       ├─ if              -> parseIf()
       ├─ while           -> parseWhile()
       ├─ for             -> parseFor()
       ├─ return          -> parseReturn()
       ├─ throw           -> parseThrow()
       ├─ import          -> parseImport()
       ├─ export          -> parseExport()
       ├─ expose          -> parseExpose()
       ├─ overwrite       -> parseOverwrite()
       ├─ unsafe          -> parseUnsafe()
       ├─ defer           -> parseDefer()
       ├─ try             -> parseTryCatch()
       ├─ match           -> parseMatch()
       ├─ break/continue  -> consume, expect ';', return Break/Continue
       └─ default         -> parseExprStmt()

parseExprStmt()
  └─ parseExpr() -> parseAssign() -> ... -> parsePrimary()
```

### Expression Precedence (Highest to Lowest)

The parser implements precedence climbing via separate functions:

```
parseAssign()          // = += -= *= /= (right-associative)
  └─ parseTernary()    // ? :
      └─ parseNullCoal()  // ??
          └─ parseOr()    // ||
              └─ parseAnd()   // &&
                  └─ parseEquality()  // == !=
                      └─ parseComparison() // < > <= >=
                          └─ parseShift()    // << >>
                              └─ parseAddSub() // + -
                                  └─ parseMulDiv() // * / % **
                                      └─ parseUnary() // ! - ~ & * +N> ~N>
                                          └─ parsePipeline() // ->
                                              └─ parsePostfix() // () [] . ?. :: ->
                                                  └─ parsePrimary() // literals, identifiers, grouping, lambdas, new, delete, list/dict literals
```

### Key Parsing Details

#### Type Parsing (`parseType()`)
Handles:
- Primitive types: `int_64`, `float_64`, `bool`, `str`, `void`, etc.
- Generic types: `list<int_32>`, `dict<str, int_64>`
- Nullable: `str?`
- `auto` for type inference

#### Parameter Lists (`parseParamList()`)
```
expect '('
while not ')' and not EOF:
    name = expect IDENT
    type = if match ':' then parseType() else ""
    params.push({name, type})
    if not match ',' break
expect ')'
```

#### Postfix Parsing (`parsePostfix()`)
Handles chained operations in a loop:
```
expr = parsePrimary()
while true:
    if match '.' or '?.' or '::' or '->':
        safe = matched '?.'
        op = matched token string
        member = expect IDENT
        if match '(':
            args = parse arglist
            expr = CallExpr(MemberExpr(expr, member, safe, op), args)
        else:
            expr = MemberExpr(expr, member, safe, op)
    else if match '[':
        index = parseExpr()
        expect ']'
        expr = IndexExpr(expr, index)
    else if match '(':
        args = parse arglist
        expr = CallExpr(expr, args)
    else:
        break
return expr
```

#### Pipeline Parsing (`parsePipeline()`)
```
left = parsePostfix()
while match '->':
    right = parsePostfix()
    left = PipelineExpr(left, right)
return left
```

#### PS-String Parsing (`parsePSString()`)
Raw content from lexer (including `{...}` unparsed) → parsed into segments:
- Plain text → `PSSegment{isExpr=false, text=...}`
- `{expression}` → lex+parse inner expression → `PSSegment{isExpr=true, expr=..., fmtSpec=...}`
- Format spec detected by `:` followed by known keywords: `join`, `repeat`, `pad`, `upper`, `lower`

#### Match Statement (`parseMatch()`)
```
expect 'match' '(' expr ')' '{'
while not '}':
    caseLine = current line
    if ident '_' -> advance, pattern = null
    else pattern = parseExpr()
    expect '=>'
    body = parseBlock()
    cases.push({pattern, body, caseLine})
expect '}'
```

---

## Token Types Used by Parser

From `include/token.hpp`, the parser consumes these token types:

### Keywords (statement starters)
`LET`, `VAR`, `AUTO`, `FN`, `CLASS`, `STRUCT`, `INTERFACE`, `IF`, `ELSE`, `WHILE`, `FOR`, `IN`, `BREAK`, `CONTINUE`, `RETURN`, `THROW`, `IMPORT`, `EXPORT`, `EXPOSE`, `OVERWRITE`, `UNSAFE`, `DEFER`, `TRY`, `CATCH`, `MATCH`

### Literals
`INT_LIT`, `FLOAT_LIT`, `STRING_LIT`, `CHAR_LIT`, `PSSTRING_LIT`, `BOOL_LIT` (`TRUE_KW`/`FALSE_KW`), `NULL_LIT` (`NULL_KW`)

### Operators
`PLUS`, `MINUS`, `STAR`, `SLASH`, `PERCENT`, `POWER`,
`EQ`, `NEQ`, `LT`, `GT`, `LTE`, `GTE`,
`AND`, `OR`, `NOT`, `BANG`,
`BIT_AND`, `BIT_OR`, `BIT_XOR`, `BIT_NOT`, `LSHIFT`, `RSHIFT`,
`ASSIGN`, `PLUS_ASSIGN`, `MINUS_ASSIGN`, `STAR_ASSIGN`, `SLASH_ASSIGN`,
`ARROW` (`->`), `FAT_ARROW` (`=>`), `NULL_COAL` (`??`), `OPT_CHAIN` (`?.`), `QUESTION` (`?`),
`LSHIFT_OUT` (`<<`), `RSHIFT_IN` (`>>`), `TILDE` (`~`), `PLUS_NUM` (`+N>`), `CONSTRUCTOR_ORDER`, `DESTRUCTOR_ORDER`

### Delimiters
`LPAREN`, `RPAREN`, `LBRACE`, `RBRACE`, `LBRACKET`, `RBRACKET`,
`SEMICOLON`, `COLON`, `DOUBLE_COLON`, `COMMA`, `DOT`, `AT`

### Special
`IDENT`, `THIS_KW`, `AS`, `HASH` (`#`), `EOF_T`, `INVALID`

---

## Error Handling

- All parse errors throw `ParseError` (defined in `include/error.hpp`)
- Errors include message and line number
- `expect()` throws if token doesn't match
- Parser continues after error in some contexts (recovery not implemented)

---

## Integration with Interpreter

The interpreter (`Interpreter::execStmt`, `Interpreter::evalExpr`) uses `std::visit` on the variant:

```cpp
void Interpreter::execStmt(const Stmt& s) {
    std::visit([this](const auto& stmt) { execStmt(stmt); }, s.data);
}

ValuePtr Interpreter::evalExpr(const Expr& e) {
    return std::visit([this](const auto& expr) { return evalExpr(expr); }, e.data);
}
```

Each concrete node type has a corresponding `execStmt(NodeType&)` / `evalExpr(NodeType&)` method.

---

## Extending the Parser

To add new syntax:

1. **Add token type** in `include/token.hpp` (`TokenType` enum)
2. **Add keyword mapping** in `src/core/lexer.cpp` (`KEYWORDS` map)
3. **Add AST node** in `include/ast.hpp` (both Expr and/or Stmt variant)
4. **Add parsing method** in `include/parser.hpp` + `src/core/parser.cpp`
5. **Wire into `parseStmt()` or expression chain**
6. **Add interpreter execution** in `src/core/interpreter.cpp`

---

## Performance Notes

- Tokens are owned by `Parser` (moved in constructor)
- AST nodes use `unique_ptr` — single allocation per node
- No backtracking in expression parsing (Pratt-style)
- `parsePSString` creates sub-lexer/parser for interpolation
- Block statements allocate new vectors for statements