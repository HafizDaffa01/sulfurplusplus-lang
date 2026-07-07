#include "../include/parser.hpp"
#include "../include/error.hpp"
#include "../include/lexer.hpp"
#include <algorithm>

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

Token &Parser::peek(int offset) {
  size_t i = pos_ + offset;
  if (i >= tokens_.size())
    return tokens_.back(); // EOF
  return tokens_[i];
}

Token &Parser::advance() {
  if (!isAtEnd())
    pos_++;
  return tokens_[pos_ - 1];
}

bool Parser::check(TokenType t) const {
  if (pos_ >= tokens_.size())
    return false;
  return tokens_[pos_].type == t;
}

bool Parser::match(TokenType t) {
  if (check(t)) {
    advance();
    return true;
  }
  return false;
}

bool Parser::match(std::initializer_list<TokenType> types) {
  for (auto t : types) {
    if (check(t)) {
      advance();
      return true;
    }
  }
  return false;
}

Token Parser::expect(TokenType t, const std::string &msg) {
  if (check(t))
    return advance();
  throw ParseError(msg + " (got '" + peek().value + "')", peek().line);
}

bool Parser::isAtEnd() const {
  return pos_ >= tokens_.size() || tokens_[pos_].type == TokenType::EOF_T;
}

std::vector<StmtPtr> Parser::parse() {
  std::vector<StmtPtr> stmts;
  while (!isAtEnd()) {
    stmts.push_back(parseStmt());
  }
  return stmts;
}

// --- parseType
// ----------------------------------------------------------------

std::string Parser::parseType() {
  static const std::vector<TokenType> typeTokens = {
      TokenType::TYPE_INT8,    TokenType::TYPE_INT16,  TokenType::TYPE_INT32,
      TokenType::TYPE_INT64,   TokenType::TYPE_UINT8,  TokenType::TYPE_UINT16,
      TokenType::TYPE_UINT32,  TokenType::TYPE_UINT64, TokenType::TYPE_FLOAT32,
      TokenType::TYPE_FLOAT64, TokenType::TYPE_BOOL,   TokenType::TYPE_CHAR,
      TokenType::TYPE_STR,     TokenType::TYPE_VOID,   TokenType::TYPE_LIST,
      TokenType::TYPE_SET,     TokenType::TYPE_DICT,   TokenType::TYPE_MATRIX,
      TokenType::AUTO,        TokenType::IDENT,
  };

  std::string type;
  bool found = false;
  for (auto tt : typeTokens) {
    if (check(tt)) {
      type = advance().value;
      found = true;
      break;
    }
  }
  if (!found)
    throw ParseError("Expected type", peek().line);

  // Generic: list<int_32>, dict<str, int_32>
  if (match(TokenType::LT)) {
    type += "<";
    type += parseType();
    if (match(TokenType::COMMA)) {
      type += ", ";
      type += parseType();
    }
    expect(TokenType::GT, "Expected '>' after generic type");
    type += ">";
  }

  // Nullable: str?
  if (match(TokenType::QUESTION))
    type += "?";

  return type;
}

// --- parseParamList
// -----------------------------------------------------------

std::vector<std::pair<std::string, std::string>> Parser::parseParamList() {
  std::vector<std::pair<std::string, std::string>> params;
  expect(TokenType::LPAREN, "Expected '(' in parameter list");
  while (!check(TokenType::RPAREN) && !isAtEnd()) {
    std::string name =
        expect(TokenType::IDENT, "Expected parameter name").value;
    std::string type;
    if (match(TokenType::COLON))
      type = parseType();
    params.push_back({name, type});
    if (!match(TokenType::COMMA))
      break;
  }
  expect(TokenType::RPAREN, "Expected ')' after parameters");
  return params;
}

// --- parseStmt
// ----------------------------------------------------------------

StmtPtr Parser::parseStmt() {
  int line = peek().line;


  if (check(TokenType::LET))
    return parseVarDecl("let");
  if (check(TokenType::VAR))
    return parseVarDecl("var");
  if (check(TokenType::AUTO))
    return parseVarDecl("auto");

  if (check(TokenType::FN))
    return parseFnDecl();
  if (check(TokenType::CLASS))
    return parseClassDecl();
  if (check(TokenType::STRUCT))
    return parseStructDecl();
  if (check(TokenType::INTERFACE))
    return parseInterfaceDecl();
  if (check(TokenType::LBRACE))
    return parseBlock();
  if (check(TokenType::IF))
    return parseIf();
  if (check(TokenType::WHILE))
    return parseWhile();
  if (check(TokenType::FOR))
    return parseFor();
  if (check(TokenType::RETURN))
    return parseReturn();
  if (check(TokenType::THROW))
    return parseThrow();
  if (check(TokenType::IMPORT))
    return parseImport();
  if (check(TokenType::EXPORT))
    return parseExport();
  if (check(TokenType::EXPOSE))
    return parseExpose();
  if (check(TokenType::OVERWRITE))
    return parseOverwrite();
  if (check(TokenType::UNSAFE))
    return parseUnsafe();
  if (check(TokenType::DEFER))
    return parseDefer();
  if (check(TokenType::TRY))
    return parseTryCatch();

  if (check(TokenType::BREAK)) {
    advance();
    expect(TokenType::SEMICOLON, "Expected ';' after break statement");
    return std::make_unique<Stmt>(BreakStmt{line});
  }
  if (check(TokenType::CONTINUE)) {
    advance();
    expect(TokenType::SEMICOLON, "Expected ';' after continue statement");
    return std::make_unique<Stmt>(ContinueStmt{line});
  }

  return parseExprStmt();
}

StmtPtr Parser::parseVarDecl(const std::string &keyword) {
  int line = peek().line;
  advance(); // consume keyword

  std::string name = expect(TokenType::IDENT, "Expected variable name").value;
  std::string type;
  ExprPtr init;

  if (match(TokenType::COLON))
    type = parseType();
  if (match(TokenType::ASSIGN))
    init = parseExpr();

  expect(TokenType::SEMICOLON, "Expected ';' after variable declaration");
  return std::make_unique<Stmt>(
      VarDeclStmt{keyword, name, type, std::move(init), line});
}

StmtPtr Parser::parseFnDecl(bool isMethod) {
  int line = peek().line;
  advance(); // consume 'fn'

  std::string name;
  if (check(TokenType::IDENT))
    name = advance().value;

  auto params = parseParamList();

  std::string retType;
  if (match(TokenType::COLON))
    retType = parseType();
  else if (check(TokenType::IDENT) && peek().value == "->") {
    // fn name() -> type  syntax alternative
    advance();
    retType = parseType();
  }

  auto body = parseBlock();
  return std::make_unique<Stmt>(
      FnDeclStmt{name, params, retType, std::move(body), isMethod, line});
}

StmtPtr Parser::parseClassDecl() {
  int line = peek().line;
  advance(); // 'class'
  std::string name = expect(TokenType::IDENT, "Expected class name").value;

  std::vector<std::string> interfaces;
  if (match(TokenType::COLON)) {
    do {
      interfaces.push_back(
          expect(TokenType::IDENT, "Expected interface name").value);
    } while (match(TokenType::COMMA));
  }

  expect(TokenType::LBRACE, "Expected '{' in class body");
  std::vector<StmtPtr> members;
  std::vector<std::pair<int, std::string>> ctorOrder, dtorOrder;

  while (!check(TokenType::RBRACE) && !isAtEnd()) {
    if (check(TokenType::CONSTRUCTOR_ORDER)) {
      int n = std::stoi(advance().value);
      std::string methodName =
          expect(TokenType::IDENT, "Expected method name").value;
      expect(TokenType::SEMICOLON, "Expected ';' after constructor order declaration");
      ctorOrder.push_back({n, methodName});
      continue;
    }
    if (check(TokenType::DESTRUCTOR_ORDER)) {
      int n = std::stoi(advance().value);
      std::string methodName =
          expect(TokenType::IDENT, "Expected method name").value;
      expect(TokenType::SEMICOLON, "Expected ';' after destructor order declaration");
      dtorOrder.push_back({n, methodName});
      continue;
    }
    if (check(TokenType::FN)) {
      members.push_back(parseFnDecl(true));
      continue;
    }
    // Field declarations: name: type;
    if (check(TokenType::IDENT)) {
      std::string fieldName = advance().value;
      std::string fieldType;
      if (match(TokenType::COLON))
        fieldType = parseType();
      ExprPtr init;
      if (match(TokenType::ASSIGN))
        init = parseExpr();
      expect(TokenType::SEMICOLON, "Expected ';' after field declaration");
      // Store as var decl
      members.push_back(std::make_unique<Stmt>(
          VarDeclStmt{"var", fieldName, fieldType, std::move(init), line}));
      continue;
    }
    // Variable declarations inside class
    if (check(TokenType::LET) || check(TokenType::VAR) ||
        check(TokenType::AUTO)) {
      std::string kw = advance().value;
      members.push_back(parseVarDecl(kw)); // This re-reads... fix
      continue;
    }
    // Skip unknown tokens
    advance();
  }
  expect(TokenType::RBRACE, "Expected '}' after class body");

  // Sort ctor/dtor order
  std::sort(ctorOrder.begin(), ctorOrder.end());
  std::sort(dtorOrder.begin(), dtorOrder.end());

  return std::make_unique<Stmt>(ClassDeclStmt{
      name, interfaces, std::move(members), ctorOrder, dtorOrder, line});
}

StmtPtr Parser::parseStructDecl() {
  int line = peek().line;
  advance(); // 'struct'
  std::string name = expect(TokenType::IDENT, "Expected struct name").value;
  expect(TokenType::LBRACE, "Expected '{' in struct body");

  std::vector<std::pair<std::string, std::string>> fields;
  while (!check(TokenType::RBRACE) && !isAtEnd()) {
    std::string fname = expect(TokenType::IDENT, "Expected field name").value;
    expect(TokenType::COLON, "Expected ':' after field name");
    std::string ftype = parseType();
    fields.push_back({fname, ftype});
    match(TokenType::COMMA);
    expect(TokenType::SEMICOLON, "Expected ';' after struct field declaration");
  }
  expect(TokenType::RBRACE, "Expected '}' after struct body");
  return std::make_unique<Stmt>(StructDeclStmt{name, fields, line});
}

StmtPtr Parser::parseInterfaceDecl() {
  int line = peek().line;
  advance(); // 'interface'
  std::string name = expect(TokenType::IDENT, "Expected interface name").value;
  expect(TokenType::LBRACE, "Expected '{' in interface body");

  InterfaceDeclStmt iface;
  iface.name = name;
  iface.line = line;

  while (!check(TokenType::RBRACE) && !isAtEnd()) {
    if (check(TokenType::FN)) {
      advance();
      std::string mname =
          expect(TokenType::IDENT, "Expected method name").value;
      auto params = parseParamList();
      std::string retType;
      if (match(TokenType::COLON))
        retType = parseType();
      expect(TokenType::SEMICOLON, "Expected ';' after interface method declaration");
      iface.methods.push_back(
          FnDeclStmt{mname, params, retType, nullptr, true, line});
      continue;
    }
    advance();
  }
  expect(TokenType::RBRACE, "Expected '}' after interface body");
  return std::make_unique<Stmt>(std::move(iface));
}

StmtPtr Parser::parseBlock() {
  int line = peek().line;
  expect(TokenType::LBRACE, "Expected '{'");
  std::vector<StmtPtr> stmts;
  while (!check(TokenType::RBRACE) && !isAtEnd()) {
    stmts.push_back(parseStmt());
  }
  expect(TokenType::RBRACE, "Expected '}'");
  return std::make_unique<Stmt>(BlockStmt{std::move(stmts), line});
}

StmtPtr Parser::parseIf() {
  int line = peek().line;
  advance(); // 'if'
  expect(TokenType::LPAREN, "Expected '(' after 'if'");
  auto cond = parseExpr();
  expect(TokenType::RPAREN, "Expected ')' after if condition");
  auto then = parseBlock();
  StmtPtr els;
  if (match(TokenType::ELSE)) {
    if (check(TokenType::IF))
      els = parseIf();
    else
      els = parseBlock();
  }
  return std::make_unique<Stmt>(
      IfStmt{std::move(cond), std::move(then), std::move(els), line});
}

StmtPtr Parser::parseWhile() {
  int line = peek().line;
  advance(); // 'while'
  expect(TokenType::LPAREN, "Expected '(' after 'while'");
  auto cond = parseExpr();
  expect(TokenType::RPAREN, "Expected ')' after while condition");
  auto body = parseBlock();
  return std::make_unique<Stmt>(
      WhileStmt{std::move(cond), std::move(body), line});
}

StmtPtr Parser::parseFor() {
  int line = peek().line;
  advance(); // 'for'
  expect(TokenType::LPAREN, "Expected '(' after 'for'");
  std::string var = expect(TokenType::IDENT, "Expected loop variable").value;
  expect(TokenType::IN, "Expected 'in' in for loop");
  auto iterable = parseExpr();
  expect(TokenType::RPAREN, "Expected ')' after for iterable");
  auto body = parseBlock();
  return std::make_unique<Stmt>(
      ForStmt{var, std::move(iterable), std::move(body), line});
}

StmtPtr Parser::parseReturn() {
  int line = peek().line;
  advance(); // 'return'
  ExprPtr val;
  if (!check(TokenType::SEMICOLON) && !check(TokenType::RBRACE) && !isAtEnd())
    val = parseExpr();
  expect(TokenType::SEMICOLON, "Expected ';' after return statement");
  return std::make_unique<Stmt>(ReturnStmt{std::move(val), line});
}

StmtPtr Parser::parseThrow() {
  int line = peek().line;
  advance(); // 'throw'
  auto val = parseExpr();
  expect(TokenType::SEMICOLON, "Expected ';' after throw statement");
  return std::make_unique<Stmt>(ThrowStmt{std::move(val), line});
}



StmtPtr Parser::parseImport() {
  int line = peek().line;
  advance(); // 'import'

  std::string pkg;
  while (!check(TokenType::AS) && !check(TokenType::SEMICOLON) &&
         !isAtEnd() && !check(TokenType::MINUS)) {
    pkg += advance().value;
  }

  std::string alias;
  if (match(TokenType::AS))
    alias = expect(TokenType::IDENT, "Expected alias").value;

  std::vector<std::string> flags;
  // --use=[FLAG1, FLAG2]
  while (check(TokenType::MINUS) && peek(1).value == "-") {
    advance();
    advance(); // --
    if (peek().value == "use") {
      advance();
      if (match(TokenType::ASSIGN)) {
        expect(TokenType::LBRACKET, "Expected '[' after --use=");
        while (!check(TokenType::RBRACKET) && !isAtEnd()) {
          if (check(TokenType::IDENT))
            flags.push_back(advance().value);
          match(TokenType::COMMA);
        }
        expect(TokenType::RBRACKET, "Expected ']' after flags");
      }
    }
  }

  expect(TokenType::SEMICOLON, "Expected ';' after import statement");
  return std::make_unique<Stmt>(ImportStmt{pkg, alias, flags, line});
}

StmtPtr Parser::parseExport() {
  int line = peek().line;
  advance(); // 'export'
  
  expect(TokenType::THIS_KW, "Expected 'this' after 'export'");
  expect(TokenType::AS, "Expected 'as' after 'this'");
  
  std::string alias;
  while (!check(TokenType::SEMICOLON) && !isAtEnd()) {
    alias += advance().value;
  }
  
  expect(TokenType::SEMICOLON, "Expected ';' after export statement");
  return std::make_unique<Stmt>(ExportStmt{alias, line});
}

StmtPtr Parser::parseExpose() {
  int line = peek().line;
  advance(); // 'expose'
  
  std::string name = expect(TokenType::STRING_LIT, "Expected string literal after 'expose'").value;
  expect(TokenType::AS, "Expected 'as' after expose name");
  std::string alias = expect(TokenType::IDENT, "Expected alias identifier").value;
  
  expect(TokenType::SEMICOLON, "Expected ';' after expose statement");
  return std::make_unique<Stmt>(ExposeStmt{name, alias, line});
}

StmtPtr Parser::parseOverwrite() {
  int line = peek().line;
  advance(); // 'overwrite'
  
  std::string target;
  while (!isAtEnd()) {
    if (check(TokenType::IDENT) && peek().value == "with") {
      advance(); // consume 'with'
      break;
    }
    if (check(TokenType::ASSIGN)) {
      advance(); // consume '='
      break;
    }
    target += advance().value;
  }
  
  auto value = parseExpr();
  expect(TokenType::SEMICOLON, "Expected ';' after overwrite statement");
  return std::make_unique<Stmt>(OverwriteStmt{target, std::move(value), line});
}
StmtPtr Parser::parseUnsafe() {
  int line = peek().line;
  advance();
  auto body = parseBlock();
  return std::make_unique<Stmt>(UnsafeStmt{std::move(body), line});
}

StmtPtr Parser::parseDefer() {
  int line = peek().line;
  advance();
  auto body = parseBlock();
  return std::make_unique<Stmt>(DeferStmt{std::move(body), line});
}

StmtPtr Parser::parseTryCatch() {
  int line = peek().line;
  advance(); // consume 'try'
  auto tryBody = parseBlock();

  std::string catchVar = "e";
  StmtPtr catchBody;
  if (check(TokenType::CATCH)) {
    advance(); // consume 'catch'
    expect(TokenType::LPAREN, "Expected '(' after 'catch'");
    catchVar = expect(TokenType::IDENT, "Expected catch variable name").value;
    expect(TokenType::RPAREN, "Expected ')' after catch variable");
    catchBody = parseBlock();
  }

  StmtPtr finallyBody;
  // check for optional 'finally' (parsed as IDENT since it's not a keyword yet)
  if (check(TokenType::IDENT) && peek().value == "finally") {
    advance();
    finallyBody = parseBlock();
  }

  return std::make_unique<Stmt>(TryCatchStmt{
      std::move(tryBody), catchVar, std::move(catchBody),
      std::move(finallyBody), line});
}

StmtPtr Parser::parseExprStmt() {
  int line = peek().line;

  // Check for stream-out: expr << expr
  // We parse as a normal expression and handle << at expression level,
  // but Terminal.Out << "..." is a special stream statement
  auto expr = parseExpr();
  expect(TokenType::SEMICOLON, "Expected ';' after expression statement");
  return std::make_unique<Stmt>(ExprStmt{std::move(expr), line});
}

// --- Expressions -------------------------------------------------------------

ExprPtr Parser::parseExpr() { return parseAssign(); }

ExprPtr Parser::parseAssign() {
  int line = peek().line;
  auto left = parseTernary();

  if (check(TokenType::ASSIGN) || check(TokenType::PLUS_ASSIGN) ||
      check(TokenType::MINUS_ASSIGN) || check(TokenType::STAR_ASSIGN) ||
      check(TokenType::SLASH_ASSIGN)) {
    std::string op = advance().value;
    auto right = parseAssign();
    return std::make_unique<Expr>(
        AssignExpr{std::move(left), op, std::move(right), line});
  }
  return left;
}

ExprPtr Parser::parseTernary() {
  int line = peek().line;
  auto cond = parseNullCoal();
  if (match(TokenType::QUESTION)) {
    auto then = parseExpr();
    expect(TokenType::COLON, "Expected ':' in ternary expression");
    auto els = parseExpr();
    return std::make_unique<Expr>(
        TernaryExpr{std::move(cond), std::move(then), std::move(els), line});
  }
  return cond;
}

ExprPtr Parser::parseNullCoal() {
  int line = peek().line;
  auto left = parseOr();
  while (check(TokenType::NULL_COAL)) {
    advance();
    auto right = parseOr();
    left = std::make_unique<Expr>(
        NullCoalExpr{std::move(left), std::move(right), line});
  }
  return left;
}

ExprPtr Parser::parseOr() {
  int line = peek().line;
  auto left = parseAnd();
  while (check(TokenType::OR)) {
    std::string op = advance().value;
    auto right = parseAnd();
    left = std::make_unique<Expr>(
        BinaryExpr{op, std::move(left), std::move(right), line});
  }
  return left;
}

ExprPtr Parser::parseAnd() {
  int line = peek().line;
  auto left = parseEquality();
  while (check(TokenType::AND)) {
    std::string op = advance().value;
    auto right = parseEquality();
    left = std::make_unique<Expr>(
        BinaryExpr{op, std::move(left), std::move(right), line});
  }
  return left;
}

ExprPtr Parser::parseEquality() {
  int line = peek().line;
  auto left = parseComparison();
  while (check(TokenType::EQ) || check(TokenType::NEQ)) {
    std::string op = advance().value;
    auto right = parseComparison();
    left = std::make_unique<Expr>(
        BinaryExpr{op, std::move(left), std::move(right), line});
  }
  return left;
}

ExprPtr Parser::parseComparison() {
  int line = peek().line;
  auto left = parseShift();
  while (check(TokenType::LT) || check(TokenType::GT) ||
         check(TokenType::LTE) || check(TokenType::GTE)) {
    std::string op = advance().value;
    auto right = parseShift();
    left = std::make_unique<Expr>(
        BinaryExpr{op, std::move(left), std::move(right), line});
  }
  return left;
}

ExprPtr Parser::parseAddSub() {
  int line = peek().line;
  auto left = parseMulDiv();
  while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
    std::string op = advance().value;
    auto right = parseMulDiv();
    left = std::make_unique<Expr>(
        BinaryExpr{op, std::move(left), std::move(right), line});
  }
  return left;
}

ExprPtr Parser::parseShift() {
  int line = peek().line;
  auto left = parseAddSub();
  while (check(TokenType::LSHIFT_OUT) || check(TokenType::RSHIFT_IN)) {
    std::string op = advance().value;
    auto right = parseAddSub();
    left = std::make_unique<Expr>(
        BinaryExpr{op, std::move(left), std::move(right), line});
  }
  return left;
}

ExprPtr Parser::parseMulDiv() {
  int line = peek().line;
  auto left = parseUnary();
  while (check(TokenType::STAR) || check(TokenType::SLASH) ||
         check(TokenType::PERCENT) || check(TokenType::POWER)) {
    std::string op = advance().value;
    auto right = parseUnary();
    left = std::make_unique<Expr>(
        BinaryExpr{op, std::move(left), std::move(right), line});
  }
  return left;
}

ExprPtr Parser::parseUnary() {
  int line = peek().line;
  if (check(TokenType::BANG) || check(TokenType::MINUS) ||
      check(TokenType::TILDE)) {
    std::string op = advance().value;
    auto operand = parseUnary();
    return std::make_unique<Expr>(UnaryExpr{op, std::move(operand), line});
  }
  if (check(TokenType::BIT_AND)) {
    advance();
    auto operand = parseUnary();
    return std::make_unique<Expr>(AddrOfExpr{std::move(operand), line});
  }
  if (check(TokenType::STAR)) {
    advance();
    auto operand = parseUnary();
    return std::make_unique<Expr>(DerefExpr{std::move(operand), line});
  }
  return parsePipeline();
}

ExprPtr Parser::parsePipeline() {
  int line = peek().line;
  auto left = parsePostfix();
  while (check(TokenType::ARROW)) {
    advance();
    // Right side: a function call or identifier to call
    auto right = parsePostfix();
    left = std::make_unique<Expr>(
        PipelineExpr{std::move(left), std::move(right), line});
  }
  return left;
}

ExprPtr Parser::parsePostfix() {
  int line = peek().line;
  auto expr = parsePrimary();

  while (true) {
    if (check(TokenType::DOT) || check(TokenType::OPT_CHAIN) || check(TokenType::DOUBLE_COLON) || check(TokenType::ARROW)) {
      TokenType opType = peek().type;
      bool safe = (opType == TokenType::OPT_CHAIN);
      std::string opStr = ".";
      if (opType == TokenType::DOUBLE_COLON) opStr = "::";
      else if (opType == TokenType::ARROW) opStr = "->";
      else if (opType == TokenType::OPT_CHAIN) opStr = "?.";

      advance();
      std::string member =
          expect(TokenType::IDENT, "Expected member name").value;
      // If followed by (, it's a method call
      if (check(TokenType::LPAREN)) {
        advance();
        std::vector<ExprPtr> args;
        while (!check(TokenType::RPAREN) && !isAtEnd()) {
          args.push_back(parseExpr());
          if (!match(TokenType::COMMA))
            break;
        }
        expect(TokenType::RPAREN, "Expected ')' after arguments");
        // Build as CallExpr on MemberExpr
        auto mem = std::make_unique<Expr>(
            MemberExpr{std::move(expr), member, safe, opStr, line});
        expr = std::make_unique<Expr>(
            CallExpr{std::move(mem), std::move(args), line});
      } else {
        expr = std::make_unique<Expr>(
            MemberExpr{std::move(expr), member, safe, opStr, line});
      }
    } else if (check(TokenType::LBRACKET)) {
      advance();
      auto idx = parseExpr();
      expect(TokenType::RBRACKET, "Expected ']'");
      expr = std::make_unique<Expr>(
          IndexExpr{std::move(expr), std::move(idx), line});
    } else if (check(TokenType::LPAREN)) {
      advance();
      std::vector<ExprPtr> args;
      while (!check(TokenType::RPAREN) && !isAtEnd()) {
        args.push_back(parseExpr());
        if (!match(TokenType::COMMA))
          break;
      }
      expect(TokenType::RPAREN, "Expected ')' after arguments");
      expr = std::make_unique<Expr>(
          CallExpr{std::move(expr), std::move(args), line});
    } else {
      break;
    }
  }

  return expr;
}

ExprPtr Parser::parsePrimary() {
  int line = peek().line;

  if (check(TokenType::FN)) {
    advance(); // consume 'fn'
    std::string name;
    if (check(TokenType::IDENT)) {
      name = advance().value;
    }
    auto params = parseParamList();
    std::string retType;
    if (match(TokenType::COLON)) {
      retType = parseType();
    } else if (check(TokenType::IDENT) && peek().value == "->") {
      advance(); // consume ->
      retType = parseType();
    }
    auto body = parseBlock();
    return std::make_unique<Expr>(LambdaExpr{std::move(params), std::move(retType), std::move(body), line});
  }

  if (check(TokenType::INT_LIT)) {
    auto val = std::stoll(advance().value);
    return std::make_unique<Expr>(IntLitExpr{val, line});
  }
  if (check(TokenType::FLOAT_LIT)) {
    auto val = std::stod(advance().value);
    return std::make_unique<Expr>(FloatLitExpr{val, line});
  }
  if (check(TokenType::TRUE_KW)) {
    advance();
    return std::make_unique<Expr>(BoolLitExpr{true, line});
  }
  if (check(TokenType::FALSE_KW)) {
    advance();
    return std::make_unique<Expr>(BoolLitExpr{false, line});
  }
  if (check(TokenType::NULL_KW)) {
    advance();
    return std::make_unique<Expr>(NullLitExpr{line});
  }
  if (check(TokenType::STRING_LIT)) {
    auto val = advance().value;
    return std::make_unique<Expr>(StringLitExpr{val, line});
  }
  if (check(TokenType::CHAR_LIT)) {
    auto val = advance().value;
    return std::make_unique<Expr>(CharLitExpr{val[0], line});
  }
  if (check(TokenType::PSSTRING_LIT)) {
    auto raw = advance().value;
    return parsePSString(raw, line);
  }

  // new ClassName(args)
  if (check(TokenType::NEW)) {
    advance();
    std::string className =
        expect(TokenType::IDENT, "Expected class name").value;
    expect(TokenType::LPAREN, "Expected '(' after class name");
    std::vector<ExprPtr> args;
    while (!check(TokenType::RPAREN) && !isAtEnd()) {
      args.push_back(parseExpr());
      if (!match(TokenType::COMMA))
        break;
    }
    expect(TokenType::RPAREN, "Expected ')' after arguments");
    return std::make_unique<Expr>(NewExpr{className, std::move(args), line});
  }

  // delete operand
  if (check(TokenType::DELETE)) {
    advance();
    auto operand = parsePrimary();
    return std::make_unique<Expr>(DeleteExpr{std::move(operand), line});
  }

  // List literal [a, b, c]
  if (check(TokenType::LBRACKET)) {
    advance();
    std::vector<ExprPtr> elems;
    while (!check(TokenType::RBRACKET) && !isAtEnd()) {
      elems.push_back(parseExpr());
      if (!match(TokenType::COMMA))
        break;
    }
    expect(TokenType::RBRACKET, "Expected ']'");
    return std::make_unique<Expr>(ListLitExpr{std::move(elems), line});
  }

  // Dict literal {key: val, ...}
  if (check(TokenType::LBRACE)) {
    advance();
    std::vector<std::pair<ExprPtr, ExprPtr>> pairs;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
      auto key = parseExpr();
      expect(TokenType::COLON, "Expected ':' in dict literal");
      auto val = parseExpr();
      pairs.push_back({std::move(key), std::move(val)});
      if (!match(TokenType::COMMA))
        break;
    }
    expect(TokenType::RBRACE, "Expected '}'");
    return std::make_unique<Expr>(DictLitExpr{std::move(pairs), line});
  }

  // Grouped expression
  if (check(TokenType::LPAREN)) {
    advance();
    auto expr = parseExpr();
    expect(TokenType::RPAREN, "Expected ')'");
    return expr;
  }

  // Identifier
  if (check(TokenType::IDENT) || check(TokenType::THIS_KW)) {
    auto name = advance().value;
    return std::make_unique<Expr>(IdentExpr{name, line});
  }

  throw ParseError("Unexpected token '" + peek().value + "'", peek().line);
}

// --- parsePSString
// ------------------------------------------------------------

ExprPtr Parser::parsePSString(const std::string &raw, int line) {
  // Parse ps"..." raw content into segments
  // Segments: plain text or {expression with optional :format}
  std::vector<PSSegment> segments;
  size_t i = 0;
  std::string text;

  auto flush = [&]() {
    if (!text.empty()) {
      segments.push_back(PSSegment{false, text, nullptr, ""});
      text.clear();
    }
  };

  while (i < raw.size()) {
    if (raw[i] == '{') {
      flush();
      i++; // skip {
      // Read until matching }
      int depth = 1;
      std::string inner;
      while (i < raw.size() && depth > 0) {
        if (raw[i] == '{')
          depth++;
        if (raw[i] == '}') {
          depth--;
          if (depth == 0) {
            i++;
            break;
          }
        }
        inner += raw[i++];
      }
      // Check for format spec: ":join(', ')" etc.
      std::string fmtSpec;
      size_t colonPos = inner.rfind(':');
      if (colonPos != std::string::npos) {
        // Heuristic: if after colon there's a known format keyword
        std::string potFmt = inner.substr(colonPos + 1);
        if (potFmt.find("join") != std::string::npos ||
            potFmt.find("repeat") != std::string::npos ||
            potFmt.find("pad") != std::string::npos ||
            potFmt.find("upper") != std::string::npos ||
            potFmt.find("lower") != std::string::npos) {
          fmtSpec = potFmt;
          inner = inner.substr(0, colonPos);
        }
      }
      // Parse inner as expression
      try {
        Lexer subLex(inner);
        auto subToks = subLex.tokenize();
        Parser subPar(std::move(subToks));
        auto expr = subPar.parseExpr();
        segments.push_back(PSSegment{true, "", std::move(expr), fmtSpec});
      } catch (...) {
        // Fallback: treat as literal
        segments.push_back(PSSegment{false, "{" + inner + "}", nullptr, ""});
      }
    } else if (raw[i] == '\\') {
      i++;
      if (i < raw.size()) {
        switch (raw[i]) {
        case 'n':
          text += '\n';
          break;
        case 't':
          text += '\t';
          break;
        default:
          text += raw[i];
        }
        i++;
      }
    } else {
      text += raw[i++];
    }
  }
  flush();
  return std::make_unique<Expr>(PSStringExpr{std::move(segments), line});
}
