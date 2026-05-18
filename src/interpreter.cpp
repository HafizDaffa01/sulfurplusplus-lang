#include "../include/interpreter.hpp"
#include "../include/error.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

// ─── Constructor
// ──────────────────────────────────────────────────────────────

Interpreter::Interpreter(bool debugMode)
    : debugMode_(debugMode), stdout_(&std::cout), stderr_(&std::cerr),
      stdin_(&std::cin) {
  globalEnv_ = std::make_shared<Environment>();
  currentEnv_ = globalEnv_;
  registerBuiltins();
}

// ─── run
// ──────────────────────────────────────────────────────────────────────

void Interpreter::run(const std::vector<StmtPtr> &stmts) {
  deferStack_.push_back({});
  try {
    for (auto &s : stmts)
      execStmt(*s);
  } catch (ReturnSignal &) {
    // top-level return is ok
  }
  // Execute deferred blocks in LIFO order
  for (auto it = deferStack_.back().rbegin(); it != deferStack_.back().rend();
       ++it)
    execStmt(**it);
  deferStack_.pop_back();
}

// ─── Environment helpers
// ──────────────────────────────────────────────────────

void Interpreter::pushEnv() {
  currentEnv_ = std::make_shared<Environment>(currentEnv_);
}

void Interpreter::popEnv() {
  if (currentEnv_->parent())
    currentEnv_ = currentEnv_->parent();
}

void Interpreter::trace(const std::string &msg) {
  if (debugMode_)
    *stderr_ << "[TRACE] " << msg << "\n";
}

// ─── Output helpers
// ───────────────────────────────────────────────────────────

void Interpreter::print(const std::string &s) { *stdout_ << s; }
void Interpreter::printErr(const std::string &s) { *stderr_ << s; }
std::string Interpreter::readLine() {
  std::string line;
  std::getline(*stdin_, line);
  return line;
}

// ─── Reactive notification
// ────────────────────────────────────────────────────

void Interpreter::notifyWatchers(const std::string &varName, ValuePtr /*newVal*/) {
  // Walk up environment chain looking for watchers on this var
  auto *env = currentEnv_.get();
  while (env) {
    for (auto &w : env->watches()) {
      if (w.varName != varName)
        continue;
      // If condition, evaluate it
      if (w.condExpr) {
        auto savedEnv = currentEnv_;
        currentEnv_ = w.scope;
        bool fired = false;
        try {
          auto condVal = evalExpr(*static_cast<Expr *>(w.condExpr));
          fired = condVal->truthy();
        } catch (...) {
        }
        currentEnv_ = savedEnv;
        if (!fired)
          continue;
      }
      // Execute the watch body
      auto savedEnv = currentEnv_;
      currentEnv_ = w.scope;
      try {
        execStmt(*static_cast<Stmt *>(w.body));
      } catch (ReturnSignal &) {
      }
      currentEnv_ = savedEnv;
    }
    env = env->parent().get();
  }
}

// ─── execStmt
// ─────────────────────────────────────────────────────────────────

void Interpreter::execStmt(const Stmt &s) {
  std::visit(
      [&](const auto &node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, VarDeclStmt>)
          execVarDecl(node);
        else if constexpr (std::is_same_v<T, FnDeclStmt>)
          execFnDecl(node);
        else if constexpr (std::is_same_v<T, ClassDeclStmt>)
          execClassDecl(node);
        else if constexpr (std::is_same_v<T, StructDeclStmt>)
          execStructDecl(node);
        else if constexpr (std::is_same_v<T, InterfaceDeclStmt>)
          execInterfaceDecl(node);
        else if constexpr (std::is_same_v<T, BlockStmt>)
          execBlock(node);
        else if constexpr (std::is_same_v<T, IfStmt>)
          execIf(node);
        else if constexpr (std::is_same_v<T, WhileStmt>)
          execWhile(node);
        else if constexpr (std::is_same_v<T, ForStmt>)
          execFor(node);
        else if constexpr (std::is_same_v<T, ReturnStmt>)
          execReturn(node);
        else if constexpr (std::is_same_v<T, BreakStmt>)
          throw BreakSignal{};
        else if constexpr (std::is_same_v<T, ContinueStmt>)
          throw ContinueSignal{};
        else if constexpr (std::is_same_v<T, ExprStmt>)
          evalExpr(*node.expr);
        else if constexpr (std::is_same_v<T, StreamOutStmt>)
          execStreamOut(node);
        else if constexpr (std::is_same_v<T, WatchStmt>)
          execWatch(node);
        else if constexpr (std::is_same_v<T, SignalDeclStmt>)
          execSignalDecl(node);
        else if constexpr (std::is_same_v<T, EmitStmt>)
          execEmit(node);
        else if constexpr (std::is_same_v<T, OnStmt>)
          execOn(node);
        else if constexpr (std::is_same_v<T, ImportStmt>)
          execImport(node);
        else if constexpr (std::is_same_v<T, UnsafeStmt>)
          execUnsafe(node);
        else if constexpr (std::is_same_v<T, DeferStmt>)
          execDefer(node);
      },
      s.data);
}

void Interpreter::execBlock(const BlockStmt &b,
                            std::shared_ptr<Environment> env) {
  auto savedEnv = currentEnv_;
  currentEnv_ = env ? env : std::make_shared<Environment>(currentEnv_);
  deferStack_.push_back({});
  try {
    for (auto &stmt : b.stmts)
      execStmt(*stmt);
  } catch (...) {
    // Run deferred blocks even on exceptions
    for (auto it = deferStack_.back().rbegin(); it != deferStack_.back().rend();
         ++it)
      try {
        execStmt(**it);
      } catch (...) {
      }
    deferStack_.pop_back();
    currentEnv_ = savedEnv;
    throw;
  }
  for (auto it = deferStack_.back().rbegin(); it != deferStack_.back().rend();
       ++it)
    try {
      execStmt(**it);
    } catch (...) {
    }
  deferStack_.pop_back();
  currentEnv_ = savedEnv;
}

void Interpreter::execVarDecl(const VarDeclStmt &s) {
  trace("VarDecl: " + s.keyword + " " + s.name);
  ValuePtr val = makeNull();
  if (s.initializer)
    val = evalExpr(*s.initializer);

  bool mutable_ = (s.keyword != "let");
  bool reactive = (s.keyword == "reactive");
  currentEnv_->define(s.name, val, mutable_, reactive);
}

void Interpreter::execFnDecl(const FnDeclStmt &s) {
  trace("FnDecl: " + s.name);
  auto fn = std::make_shared<FunctionValue>();
  fn->name = s.name;
  fn->params = s.params;
  fn->retType = s.retType;
  fn->body = s.body.get();
  fn->closure = currentEnv_;
  currentEnv_->define(s.name, makeFn(fn), true);
}

void Interpreter::execClassDecl(const ClassDeclStmt &s) {
  trace("ClassDecl: " + s.name);
  auto def = std::make_shared<ClassDef>();
  def->name = s.name;
  def->interfaces = s.interfaces;
  def->ctorOrder = s.ctorOrder;
  def->dtorOrder = s.dtorOrder;
  def->methods = std::make_shared<Environment>(currentEnv_);

  for (auto &m : s.members) {
    if (auto *fn = std::get_if<FnDeclStmt>(&m->data)) {
      auto fv = std::make_shared<FunctionValue>();
      fv->name = fn->name;
      fv->params = fn->params;
      fv->retType = fn->retType;
      fv->body = fn->body.get();
      fv->closure = def->methods;
      def->methods->define(fn->name, makeFn(fv), true);
    } else if (auto *vd = std::get_if<VarDeclStmt>(&m->data)) {
      ValuePtr init = makeNull();
      if (vd->initializer)
        init = evalExpr(*vd->initializer);
      def->fields.push_back(std::make_pair(vd->name, vd->type));
      def->methods->define(vd->name, init, true);
    }
  }

  currentEnv_->define(s.name, makeClassDef(def), true);
}

void Interpreter::execStructDecl(const StructDeclStmt &s) {
  trace("StructDecl: " + s.name);
  auto def = std::make_shared<StructDef>();
  def->name = s.name;
  def->fields = s.fields;
  currentEnv_->define(s.name, makeStructDef(def), true);
}

void Interpreter::execInterfaceDecl(const InterfaceDeclStmt &s) {
  trace("InterfaceDecl: " + s.name);
  // Interfaces are just declarations; no runtime action needed
  currentEnv_->define(s.name, makeNull(), true);
}

void Interpreter::execIf(const IfStmt &s) {
  auto cond = evalExpr(*s.cond);
  if (cond->truthy())
    execStmt(*s.thenBranch);
  else if (s.elseBranch)
    execStmt(*s.elseBranch);
}

void Interpreter::execWhile(const WhileStmt &s) {
  while (true) {
    auto cond = evalExpr(*s.cond);
    if (!cond->truthy())
      break;
    try {
      execStmt(*s.body);
    } catch (BreakSignal &) {
      break;
    } catch (ContinueSignal &) {
      continue;
    }
  }
}

void Interpreter::execFor(const ForStmt &s) {
  auto iterable = evalExpr(*s.iterable);
  auto doBody = [&](ValuePtr item) {
    pushEnv();
    currentEnv_->define(s.var, item, true);
    try {
      execStmt(*s.body);
    } catch (BreakSignal &) {
      popEnv();
      return false;
    } catch (ContinueSignal &) {
    }
    popEnv();
    return true;
  };

  if (iterable->isList()) {
    for (auto &elem : iterable->asList()->elements)
      if (!doBody(elem))
        break;
  } else if (iterable->isStr()) {
    for (char c : iterable->asStr())
      if (!doBody(makeStr(std::string(1, c))))
        break;
  } else if (iterable->isDict()) {
    for (auto &[k, v] : iterable->asDict()->pairs)
      if (!doBody(k))
        break;
  } else if (iterable->isSet()) {
    for (auto &elem : iterable->asSet()->elements)
      if (!doBody(elem))
        break;
  } else {
    throw RuntimeError(
        "Value of type '" + iterable->typeName() + "' is not iterable", s.line);
  }
}

void Interpreter::execReturn(const ReturnStmt &s) {
  ValuePtr val = makeNull();
  if (s.value)
    val = evalExpr(*s.value);
  throw ReturnSignal{val};
}

void Interpreter::execStreamOut(const StreamOutStmt &s) {
  auto val = evalExpr(*s.value);
  print(val->toString());
}

void Interpreter::execWatch(const WatchStmt &s) {
  WatchEntry entry;
  entry.varName = s.varName;
  entry.condExpr = s.cond ? (void *)s.cond.get() : nullptr;
  entry.body = (void *)s.body.get();
  entry.scope = currentEnv_;
  currentEnv_->addWatch(std::move(entry));
}

void Interpreter::execSignalDecl(const SignalDeclStmt &s) {
  currentEnv_->declareSignal(s.name);
}

void Interpreter::execEmit(const EmitStmt &s) {
  auto *sig = currentEnv_->findSignal(s.name);
  if (!sig)
    throw RuntimeError("Signal '" + s.name + "' is not declared", s.line);
  for (auto &[body, scope] : sig->handlers) {
    auto savedEnv = currentEnv_;
    currentEnv_ = scope;
    try {
      execStmt(*static_cast<Stmt *>(body));
    } catch (ReturnSignal &) {
    }
    currentEnv_ = savedEnv;
  }
}

void Interpreter::execOn(const OnStmt &s) {
  currentEnv_->addSignalHandler(s.signal, (void *)s.body.get(), currentEnv_);
}

void Interpreter::execImport(const ImportStmt &s) {
  // Basic built-in package handling
  // In a real implementation, this would load .sfpp files from package paths
  trace("Import: " + s.pkg + " as " + s.alias);

  // Register a namespace object (dict) for the import alias
  auto ns = std::make_shared<DictValue>();
  std::string alias = s.alias.empty() ? s.pkg : s.alias;
  currentEnv_->define(alias, makeDict(ns), true);
}

void Interpreter::execUnsafe(const UnsafeStmt &s) {
  trace("Entering unsafe block");
  execStmt(*s.body);
  trace("Leaving unsafe block");
}

void Interpreter::execDefer(const DeferStmt &s) {
  if (!deferStack_.empty())
    deferStack_.back().push_back(s.body.get());
}

// ─── evalExpr
// ─────────────────────────────────────────────────────────────────

ValuePtr Interpreter::evalExpr(const Expr &e) {
  return std::visit(
      [&](const auto &node) -> ValuePtr {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IntLitExpr>)
          return evalIntLit(node);
        if constexpr (std::is_same_v<T, FloatLitExpr>)
          return evalFloatLit(node);
        if constexpr (std::is_same_v<T, BoolLitExpr>)
          return evalBoolLit(node);
        if constexpr (std::is_same_v<T, NullLitExpr>)
          return evalNullLit(node);
        if constexpr (std::is_same_v<T, StringLitExpr>)
          return evalStringLit(node);
        if constexpr (std::is_same_v<T, CharLitExpr>)
          return evalCharLit(node);
        if constexpr (std::is_same_v<T, PSStringExpr>)
          return evalPSString(node);
        if constexpr (std::is_same_v<T, IdentExpr>)
          return evalIdent(node);
        if constexpr (std::is_same_v<T, BinaryExpr>)
          return evalBinary(node);
        if constexpr (std::is_same_v<T, UnaryExpr>)
          return evalUnary(node);
        if constexpr (std::is_same_v<T, AssignExpr>)
          return evalAssign(node);
        if constexpr (std::is_same_v<T, CallExpr>)
          return evalCall(node);
        if constexpr (std::is_same_v<T, IndexExpr>)
          return evalIndex(node);
        if constexpr (std::is_same_v<T, MemberExpr>)
          return evalMember(node);
        if constexpr (std::is_same_v<T, PipelineExpr>)
          return evalPipeline(node);
        if constexpr (std::is_same_v<T, NullCoalExpr>)
          return evalNullCoal(node);
        if constexpr (std::is_same_v<T, ListLitExpr>)
          return evalListLit(node);
        if constexpr (std::is_same_v<T, DictLitExpr>)
          return evalDictLit(node);
        if constexpr (std::is_same_v<T, NewExpr>)
          return evalNew(node);
        if constexpr (std::is_same_v<T, TernaryExpr>)
          return evalTernary(node);
        if constexpr (std::is_same_v<T, LambdaExpr>) {
          auto fn = std::make_shared<FunctionValue>();
          fn->name = "<lambda>";
          fn->params = node.params;
          fn->retType = node.retType;
          fn->body = node.body.get();
          fn->closure = currentEnv_;
          return makeFn(fn);
        }
        if constexpr (std::is_same_v<T, AddrOfExpr>) {
          // Basic pointer support — return the value itself for simplicity
          return evalExpr(*node.operand);
        }
        if constexpr (std::is_same_v<T, DerefExpr>) {
          return evalExpr(*node.operand);
        }
        return makeNull();
      },
      e.data);
}

ValuePtr Interpreter::evalIntLit(const IntLitExpr &e) {
  return makeInt(e.value);
}
ValuePtr Interpreter::evalFloatLit(const FloatLitExpr &e) {
  return makeFloat(e.value);
}
ValuePtr Interpreter::evalBoolLit(const BoolLitExpr &e) {
  return makeBool(e.value);
}
ValuePtr Interpreter::evalNullLit(const NullLitExpr &) { return makeNull(); }
ValuePtr Interpreter::evalStringLit(const StringLitExpr &e) {
  return makeStr(e.value);
}
ValuePtr Interpreter::evalCharLit(const CharLitExpr &e) {
  return makeStr(std::string(1, e.value));
}

ValuePtr Interpreter::evalPSString(const PSStringExpr &e) {
  std::string result;
  for (auto &seg : e.segments) {
    if (!seg.isExpr) {
      result += seg.text;
    } else {
      auto val = evalExpr(*seg.expr);
      if (!seg.fmtSpec.empty()) {
        // Apply format spec
        if (seg.fmtSpec.find("join") != std::string::npos) {
          // Extract separator from join(', ')
          std::string sep = ", ";
          size_t lp = seg.fmtSpec.find('(');
          size_t rp = seg.fmtSpec.rfind(')');
          if (lp != std::string::npos && rp != std::string::npos) {
            std::string raw = seg.fmtSpec.substr(lp + 1, rp - lp - 1);
            if (raw.size() >= 2 && (raw.front() == '\'' || raw.front() == '"'))
              raw = raw.substr(1, raw.size() - 2);
            sep = raw;
          }
          if (val->isList()) {
            std::string joined;
            bool first = true;
            for (auto &e : val->asList()->elements) {
              if (!first)
                joined += sep;
              joined += e->toString();
              first = false;
            }
            result += joined;
            continue;
          }
        }
        if (seg.fmtSpec.find("repeat") != std::string::npos) {
          size_t lp = seg.fmtSpec.find('(');
          size_t rp = seg.fmtSpec.rfind(')');
          int n = 1;
          if (lp != std::string::npos && rp != std::string::npos)
            n = std::stoi(seg.fmtSpec.substr(lp + 1, rp - lp - 1));
          std::string s = val->toString();
          for (int i = 0; i < n; i++)
            result += s;
          continue;
        }
      }
      result += val->toString();
    }
  }
  return makeStr(result);
}

ValuePtr Interpreter::evalIdent(const IdentExpr &e) {
  return currentEnv_->get(e.name, e.line);
}

ValuePtr Interpreter::evalBinary(const BinaryExpr &e) {
  // Short-circuit for && and ||
  if (e.op == "&&") {
    auto l = evalExpr(*e.left);
    if (!l->truthy())
      return makeBool(false);
    return makeBool(evalExpr(*e.right)->truthy());
  }
  if (e.op == "||") {
    auto l = evalExpr(*e.left);
    if (l->truthy())
      return makeBool(true);
    return makeBool(evalExpr(*e.right)->truthy());
  }

  // Stream output operator << (Terminal.Out << "Hello")
  if (e.op == "<<") {
    auto left = evalExpr(*e.left);
    auto right = evalExpr(*e.right);
    // Check if left is a stream object
    std::string stream = left->toString();
    std::string msg = right->toString();
    if (stream == "<Terminal.Out>" || stream == "<stdout>") {
      print(msg + "\n");
    } else if (stream == "<Terminal.Err>" || stream == "<stderr>") {
      printErr(msg + "\n");
    } else if (stream == "<Terminal.Warn>") {
      printErr("[WARN] " + msg + "\n");
    } else {
      print(msg);
    }
    return left; // return stream for chaining
  }

  auto l = evalExpr(*e.left);
  auto r = evalExpr(*e.right);

  // Arithmetic
  if (e.op == "+" || e.op == "-" || e.op == "*" || e.op == "/" || e.op == "%" ||
      e.op == "**")
    return applyBinaryArith(e.op, l, r, e.line);

  // Comparison
  if (e.op == "==" || e.op == "!=" || e.op == "<" || e.op == ">" ||
      e.op == "<=" || e.op == ">=")
    return applyBinaryCompare(e.op, l, r, e.line);

  // Bitwise
  if (e.op == "&")
    return makeInt(l->asInt() & r->asInt());
  if (e.op == "|")
    return makeInt(l->asInt() | r->asInt());
  if (e.op == "^")
    return makeInt(l->asInt() ^ r->asInt());

  throw RuntimeError("Unknown binary operator: " + e.op, e.line);
}

ValuePtr Interpreter::applyBinaryArith(const std::string &op, ValuePtr l,
                                       ValuePtr r, int line) {
  // String concatenation
  if (op == "+") {
    if (l->isStr() || r->isStr())
      return makeStr(l->toString() + r->toString());
    if (l->isList() && r->isList()) {
      auto res = std::make_shared<ListValue>();
      for (auto &e : l->asList()->elements)
        res->elements.push_back(e);
      for (auto &e : r->asList()->elements)
        res->elements.push_back(e);
      return makeList(res);
    }
  }

  // Numeric
  bool useFloat = l->isFloat() || r->isFloat();
  if (useFloat) {
    double a = l->asFloat(), b = r->asFloat();
    if (op == "+")
      return makeFloat(a + b);
    if (op == "-")
      return makeFloat(a - b);
    if (op == "*")
      return makeFloat(a * b);
    if (op == "/") {
      if (b == 0.0)
        throw RuntimeError("Division by zero", line);
      return makeFloat(a / b);
    }
    if (op == "%")
      return makeFloat(std::fmod(a, b));
    if (op == "**")
      return makeFloat(std::pow(a, b));
  } else {
    int64_t a = l->asInt(), b = r->asInt();
    if (op == "+")
      return makeInt(a + b);
    if (op == "-")
      return makeInt(a - b);
    if (op == "*")
      return makeInt(a * b);
    if (op == "/") {
      if (b == 0)
        throw RuntimeError("Division by zero", line);
      return makeInt(a / b);
    }
    if (op == "%") {
      if (b == 0)
        throw RuntimeError("Modulo by zero", line);
      return makeInt(a % b);
    }
    if (op == "**")
      return makeFloat(std::pow((double)a, (double)b));
  }
  throw RuntimeError("Cannot apply '" + op + "' to types " + l->typeName() +
                         " and " + r->typeName(),
                     line);
}

ValuePtr Interpreter::applyBinaryCompare(const std::string &op, ValuePtr l,
                                         ValuePtr r, int line) {
  if (op == "==")
    return makeBool(l->equals(*r));
  if (op == "!=")
    return makeBool(!l->equals(*r));

  auto cmp = [&]() -> int {
    if (l->isInt() && r->isInt())
      return (l->asInt() < r->asInt()) ? -1 : (l->asInt() > r->asInt()) ? 1 : 0;
    if ((l->isInt() || l->isFloat()) && (r->isInt() || r->isFloat())) {
      double a = l->asFloat(), b = r->asFloat();
      return (a < b) ? -1 : (a > b) ? 1 : 0;
    }
    if (l->isStr() && r->isStr())
      return l->asStr().compare(r->asStr());
    throw TypeError("Cannot compare " + l->typeName() + " and " + r->typeName(),
                    line);
  };

  int c = cmp();
  if (op == "<")
    return makeBool(c < 0);
  if (op == ">")
    return makeBool(c > 0);
  if (op == "<=")
    return makeBool(c <= 0);
  if (op == ">=")
    return makeBool(c >= 0);
  return makeBool(false);
}

ValuePtr Interpreter::evalUnary(const UnaryExpr &e) {
  auto val = evalExpr(*e.operand);
  if (e.op == "!")
    return makeBool(!val->truthy());
  if (e.op == "-") {
    if (val->isInt())
      return makeInt(-val->asInt());
    if (val->isFloat())
      return makeFloat(-val->asFloat());
    throw TypeError("Cannot negate " + val->typeName(), e.line);
  }
  if (e.op == "~") {
    if (val->isInt())
      return makeInt(~val->asInt());
    throw TypeError("Cannot bitwise-not " + val->typeName(), e.line);
  }
  throw RuntimeError("Unknown unary operator: " + e.op, e.line);
}

ValuePtr Interpreter::evalAssign(const AssignExpr &e) {
  auto newVal = evalExpr(*e.value);

  // Compound assignment: compute new value first
  if (e.op != "=") {
    ValuePtr old;
    if (auto *id = std::get_if<IdentExpr>(&e.target->data))
      old = currentEnv_->get(id->name, e.line);
    else
      old = evalExpr(*e.target);
    std::string binOp = e.op.substr(0, e.op.size() - 1);
    newVal = applyBinaryArith(binOp, old, newVal, e.line);
  }

  // Assign to target
  if (auto *id = std::get_if<IdentExpr>(&e.target->data)) {
    // Check if reactive
    bool reactive = false;
    auto *env = currentEnv_.get();
    while (env) {
      auto it = env->vars().find(id->name);
      if (it != env->vars().end()) {
        reactive = it->second.reactive;
        break;
      }
      env = env->parent().get();
    }
    currentEnv_->set(id->name, newVal, e.line);
    if (reactive)
      notifyWatchers(id->name, newVal);
  } else if (auto *idx = std::get_if<IndexExpr>(&e.target->data)) {
    auto obj = evalExpr(*idx->object);
    auto key = evalExpr(*idx->index);
    if (obj->isList()) {
      auto &elems = obj->asList()->elements;
      int64_t i = key->asInt();
      if (i < 0)
        i += elems.size();
      if (i < 0 || (size_t)i >= elems.size())
        throw RuntimeError("List index out of bounds", e.line);
      elems[i] = newVal;
    } else if (obj->isDict()) {
      obj->asDict()->set(key->toString(), newVal);
    } else {
      throw RuntimeError("Cannot index-assign to " + obj->typeName(), e.line);
    }
  } else if (auto *mem = std::get_if<MemberExpr>(&e.target->data)) {
    auto obj = evalExpr(*mem->object);
    if (obj->isClassInst()) {
      obj->asClassInst()->members->set(mem->member, newVal, e.line);
    } else if (obj->isStructInst()) {
      obj->asStructInst()->fields[mem->member] = newVal;
    } else if (obj->isDict()) {
      obj->asDict()->set(mem->member, newVal);
    } else {
      throw RuntimeError("Cannot assign member '" + mem->member + "' on " +
                             obj->typeName(),
                         e.line);
    }
  } else {
    throw RuntimeError("Invalid assignment target", e.line);
  }

  return newVal;
}

ValuePtr Interpreter::evalCall(const CallExpr &e) {
  // Evaluate arguments
  std::vector<ValuePtr> args;
  args.reserve(e.args.size());
  for (auto &a : e.args)
    args.push_back(evalExpr(*a));

  // Intercept MemberExpr for method calls
  if (auto *mem = std::get_if<MemberExpr>(&e.callee->data)) {
    auto obj = evalExpr(*mem->object);
    if (obj->isNull() && mem->safe) return makeNull();

    if (obj->isClassInst()) {
      auto inst = obj->asClassInst();
      if (inst->members->hasLocal(mem->member) || inst->members->has(mem->member)) {
        return callMethod(inst, mem->member, std::move(args), e.line);
      }
      throw RuntimeError("Method '" + mem->member + "' not found on " + inst->def->name, e.line);
    }
    
    if (obj->isStructInst()) {
      auto inst = obj->asStructInst();
      auto it = inst->fields.find(mem->member);
      if (it != inst->fields.end() && it->second->isFn()) {
        return callFunction(it->second->asFn(), std::move(args), e.line);
      }
      throw RuntimeError("Method/Field '" + mem->member + "' not found on struct " + inst->def->name, e.line);
    }
    
    if (obj->isDict()) {
      auto v = obj->asDict()->get(mem->member);
      if (v && v->isFn()) return callFunction(v->asFn(), std::move(args), e.line);
      return callBuiltinMethod(obj, mem->member, std::move(args), e.line);
    }
    
    // For lists, strings, etc.
    return callBuiltinMethod(obj, mem->member, std::move(args), e.line);
  }

  // Evaluate callee normally
  auto callee = evalExpr(*e.callee);

  if (callee->isFn()) {
    return callFunction(callee->asFn(), std::move(args), e.line);
  }

  if (callee->isClassDef()) {
    // Instantiate class
    auto def = std::get<std::shared_ptr<ClassDef>>(callee->data);
    auto inst = std::make_shared<ClassInstance>();
    inst->def = def;
    inst->members = std::make_shared<Environment>(def->methods);

    // Initialize fields with defaults
    for (auto &[fname, ftype] : def->fields) {
      if (!inst->members->hasLocal(fname))
        inst->members->define(fname, makeNull(), true);
    }

    // Run ordered constructors
    for (auto &[n, method] : def->ctorOrder) {
      if (inst->members->has(method)) {
        auto mv = inst->members->get(method);
        if (mv->isFn())
          callMethod(inst, method, args, e.line);
      }
    }
    // If there's an 'init' or 'constructor' method, call it
    if (inst->members->has("init"))
      callMethod(inst, "init", args, e.line);
    else if (inst->members->has("constructor"))
      callMethod(inst, "constructor", args, e.line);

    return makeClassInst(inst);
  }

  if (callee->isStructDef()) {
    auto def = std::get<std::shared_ptr<StructDef>>(callee->data);
    auto inst = std::make_shared<StructInstance>();
    inst->def = def;
    for (size_t i = 0; i < def->fields.size(); i++) {
      auto &[fname, ftype] = def->fields[i];
      inst->fields[fname] = (i < args.size()) ? args[i] : makeNull();
    }
    return makeStructInst(inst);
  }

  // Built-in method on a class instance (callee is a member expr result)
  throw RuntimeError("'" + callee->typeName() + "' is not callable", e.line);
}

ValuePtr Interpreter::callFunction(std::shared_ptr<FunctionValue> fn,
                                   std::vector<ValuePtr> args, int line) {
  if (fn->isNative) {
    return fn->native(std::move(args));
  }

  if (!fn->body)
    throw RuntimeError("Function '" + fn->name + "' has no body", line);

  auto callEnv = std::make_shared<Environment>(fn->closure);
  for (size_t i = 0; i < fn->params.size(); i++) {
    auto &[pname, ptype] = fn->params[i];
    callEnv->define(pname, (i < args.size()) ? args[i] : makeNull(), true);
  }

  auto savedEnv = currentEnv_;
  currentEnv_ = callEnv;
  ValuePtr result = makeNull();

  try {
    auto *block = static_cast<Stmt *>(fn->body);
    if (auto *b = std::get_if<BlockStmt>(&block->data))
      execBlock(*b, callEnv);
    else
      execStmt(*block);
  } catch (ReturnSignal &ret) {
    result = ret.value;
  }

  currentEnv_ = savedEnv;
  return result;
}

ValuePtr Interpreter::callMethod(std::shared_ptr<ClassInstance> inst,
                                 const std::string &name,
                                 std::vector<ValuePtr> args, int line) {
  if (!inst->members->has(name))
    throw RuntimeError("Method '" + name + "' not found", line);
  auto mv = inst->members->get(name);
  if (!mv->isFn())
    throw RuntimeError("'" + name + "' is not a method", line);
  auto fn = mv->asFn();
  // Add 'self' to closure
  auto callEnv = std::make_shared<Environment>(fn->closure);
  callEnv->define("self", makeClassInst(inst), false);
  callEnv->define("this", makeClassInst(inst), false);
  for (size_t i = 0; i < fn->params.size(); i++) {
    auto &[pname, _] = fn->params[i];
    callEnv->define(pname, (i < args.size()) ? args[i] : makeNull(), true);
  }
  auto savedEnv = currentEnv_;
  currentEnv_ = callEnv;
  ValuePtr result = makeNull();
  try {
    auto *block = static_cast<Stmt *>(fn->body);
    if (auto *b = std::get_if<BlockStmt>(&block->data))
      execBlock(*b, callEnv);
    else
      execStmt(*block);
  } catch (ReturnSignal &ret) {
    result = ret.value;
  }
  currentEnv_ = savedEnv;
  return result;
}

ValuePtr Interpreter::evalIndex(const IndexExpr &e) {
  auto obj = evalExpr(*e.object);
  auto key = evalExpr(*e.index);

  if (obj->isList()) {
    auto &elems = obj->asList()->elements;
    int64_t i = key->asInt();
    if (i < 0)
      i += elems.size();
    if (i < 0 || (size_t)i >= elems.size())
      throw RuntimeError("List index " + std::to_string(key->asInt()) +
                             " out of bounds",
                         e.line);
    return elems[i];
  }
  if (obj->isDict()) {
    auto v = obj->asDict()->get(key->toString());
    return v ? v : makeNull();
  }
  if (obj->isStr()) {
    int64_t i = key->asInt();
    const std::string &s = obj->asStr();
    if (i < 0)
      i += s.size();
    if (i < 0 || (size_t)i >= s.size())
      throw RuntimeError("String index out of bounds", e.line);
    return makeStr(std::string(1, s[i]));
  }
  throw RuntimeError("Cannot index '" + obj->typeName() + "'", e.line);
}

ValuePtr Interpreter::evalMember(const MemberExpr &e) {
  auto obj = evalExpr(*e.object);

  // Safe chaining: if null and safe, return null
  if (obj->isNull() && e.safe)
    return makeNull();

  if (obj->isClassInst()) {
    auto inst = obj->asClassInst();
    if (inst->members->hasLocal(e.member) || inst->members->has(e.member))
      return inst->members->get(e.member, e.line);
    // Return bound method
    throw RuntimeError(
        "Member '" + e.member + "' not found on " + inst->def->name, e.line);
  }

  if (obj->isStructInst()) {
    auto inst = obj->asStructInst();
    auto it = inst->fields.find(e.member);
    if (it != inst->fields.end())
      return it->second;
    throw RuntimeError("Field '" + e.member + "' not found on struct " +
                           inst->def->name,
                       e.line);
  }

  if (obj->isDict()) {
    auto v = obj->asDict()->get(e.member);
    return v ? v : makeNull();
  }

  // Built-in members
  return callBuiltinMethod(obj, e.member, {}, e.line);
}

ValuePtr Interpreter::evalPipeline(const PipelineExpr &e) {
  auto left = evalExpr(*e.left);

  // Right side should be a call expression; inject left as first argument
  if (auto *call = std::get_if<CallExpr>(&e.right->data)) {
    std::vector<ValuePtr> args;
    args.push_back(left);
    for (auto &a : call->args)
      args.push_back(evalExpr(*a));
    auto callee = evalExpr(*call->callee);
    if (callee->isFn())
      return callFunction(callee->asFn(), std::move(args), e.line);
    // Method call on left?
    if (auto *mem = std::get_if<MemberExpr>(&call->callee->data)) {
      auto obj = evalExpr(*mem->object);
      // Builtin method
      std::vector<ValuePtr> mArgs;
      for (auto &a : call->args)
        mArgs.push_back(evalExpr(*a));
      return callBuiltinMethod(left, mem->member, std::move(mArgs), e.line);
    }
    throw RuntimeError("Right side of pipeline is not callable", e.line);
  }

  // Right is an identifier: treat as function call with left as arg
  if (auto *id = std::get_if<IdentExpr>(&e.right->data)) {
    auto fn = currentEnv_->get(id->name, e.line);
    if (fn->isFn())
      return callFunction(fn->asFn(), {left}, e.line);
    throw RuntimeError("'" + id->name + "' is not a function", e.line);
  }

  // Right is a member access (method call on left itself)
  if (auto *mem = std::get_if<MemberExpr>(&e.right->data)) {
    return callBuiltinMethod(left, mem->member, {}, e.line);
  }

  throw RuntimeError("Invalid pipeline expression", e.line);
}

ValuePtr Interpreter::evalNullCoal(const NullCoalExpr &e) {
  auto left = evalExpr(*e.left);
  if (!left->isNull())
    return left;
  return evalExpr(*e.right);
}

ValuePtr Interpreter::evalListLit(const ListLitExpr &e) {
  auto lv = std::make_shared<ListValue>();
  for (auto &elem : e.elements)
    lv->elements.push_back(evalExpr(*elem));
  return makeList(lv);
}

ValuePtr Interpreter::evalDictLit(const DictLitExpr &e) {
  auto dv = std::make_shared<DictValue>();
  for (auto &[k, v] : e.pairs) {
    auto key = evalExpr(*k);
    auto val = evalExpr(*v);
    dv->set(key->toString(), val);
  }
  return makeDict(dv);
}

ValuePtr Interpreter::evalNew(const NewExpr &e) {
  auto classDef = currentEnv_->get(e.className, e.line);
  std::vector<ValuePtr> args;
  for (auto &a : e.args)
    args.push_back(evalExpr(*a));

  if (classDef->isClassDef()) {
    auto def = std::get<std::shared_ptr<ClassDef>>(classDef->data);
    auto inst = std::make_shared<ClassInstance>();
    inst->def = def;
    inst->members = std::make_shared<Environment>(def->methods);
    for (auto &[fname, _] : def->fields) {
      if (!inst->members->hasLocal(fname))
        inst->members->define(fname, makeNull(), true);
    }
    for (auto &[n, method] : def->ctorOrder) {
      if (inst->members->has(method))
        callMethod(inst, method, args, e.line);
    }
    if (inst->members->has("init"))
      callMethod(inst, "init", args, e.line);
    return makeClassInst(inst);
  }

  if (classDef->isStructDef()) {
    auto def = std::get<std::shared_ptr<StructDef>>(classDef->data);
    auto inst = std::make_shared<StructInstance>();
    inst->def = def;
    for (size_t i = 0; i < def->fields.size(); i++) {
      inst->fields[def->fields[i].first] =
          (i < args.size()) ? args[i] : makeNull();
    }
    return makeStructInst(inst);
  }

  throw RuntimeError("'" + e.className + "' is not a class or struct", e.line);
}

ValuePtr Interpreter::evalTernary(const TernaryExpr &e) {
  auto cond = evalExpr(*e.cond);
  return cond->truthy() ? evalExpr(*e.thenExpr) : evalExpr(*e.elseExpr);
}

// ─── Built-in methods
// ─────────────────────────────────────────────────────────

ValuePtr Interpreter::callBuiltinMethod(ValuePtr obj, const std::string &method,
                                        std::vector<ValuePtr> args, int line) {
  // String methods
  if (obj->isStr()) {
    const std::string &s = obj->asStr();
    if (method == "length" || method == "len")
      return makeInt(s.size());
    if (method == "upper") {
      std::string r = s;
      std::transform(r.begin(), r.end(), r.begin(), ::toupper);
      return makeStr(r);
    }
    if (method == "lower") {
      std::string r = s;
      std::transform(r.begin(), r.end(), r.begin(), ::tolower);
      return makeStr(r);
    }
    if (method == "trim") {
      size_t l = s.find_first_not_of(" \t\r\n");
      size_t r2 = s.find_last_not_of(" \t\r\n");
      return (l == std::string::npos) ? makeStr("")
                                      : makeStr(s.substr(l, r2 - l + 1));
    }
    if (method == "contains") {
      if (args.empty())
        throw RuntimeError("contains() requires an argument", line);
      return makeBool(s.find(args[0]->toString()) != std::string::npos);
    }
    if (method == "startsWith") {
      if (args.empty())
        throw RuntimeError("startsWith() requires an argument", line);
      std::string p = args[0]->toString();
      return makeBool(s.substr(0, p.size()) == p);
    }
    if (method == "endsWith") {
      if (args.empty())
        throw RuntimeError("endsWith() requires an argument", line);
      std::string p = args[0]->toString();
      return makeBool(s.size() >= p.size() &&
                      s.substr(s.size() - p.size()) == p);
    }
    if (method == "split") {
      std::string delim = args.empty() ? " " : args[0]->toString();
      auto lv = std::make_shared<ListValue>();
      size_t pos = 0, found;
      while ((found = s.find(delim, pos)) != std::string::npos) {
        lv->elements.push_back(makeStr(s.substr(pos, found - pos)));
        pos = found + delim.size();
      }
      lv->elements.push_back(makeStr(s.substr(pos)));
      return makeList(lv);
    }
    if (method == "replace") {
      if (args.size() < 2)
        throw RuntimeError("replace() requires 2 arguments", line);
      std::string from = args[0]->toString(), to = args[1]->toString();
      std::string r = s;
      size_t pos = 0;
      while ((pos = r.find(from, pos)) != std::string::npos) {
        r.replace(pos, from.size(), to);
        pos += to.size();
      }
      return makeStr(r);
    }
    if (method == "slice" || method == "substr") {
      int64_t start = args.empty() ? 0 : args[0]->asInt();
      int64_t end = (args.size() > 1) ? args[1]->asInt() : (int64_t)s.size();
      if (start < 0)
        start = std::max((int64_t)0, (int64_t)s.size() + start);
      if (end < 0)
        end = std::max((int64_t)0, (int64_t)s.size() + end);
      start = std::min(start, (int64_t)s.size());
      end = std::min(end, (int64_t)s.size());
      return makeStr(s.substr(start, end - start));
    }
    if (method == "toString" || method == "str")
      return obj;
    if (method == "toInt")
      return makeInt(std::stoll(s));
    if (method == "toFloat")
      return makeFloat(std::stod(s));
    if (method == "repeat") {
      int n = args.empty() ? 1 : (int)args[0]->asInt();
      std::string r;
      for (int i = 0; i < n; i++)
        r += s;
      return makeStr(r);
    }
    if (method == "chars") {
      auto lv = std::make_shared<ListValue>();
      for (char c : s)
        lv->elements.push_back(makeStr(std::string(1, c)));
      return makeList(lv);
    }
  }

  // List methods
  if (obj->isList()) {
    auto lv = obj->asList();
    if (method == "length" || method == "len" || method == "size")
      return makeInt(lv->elements.size());
    if (method == "push" || method == "add" || method == "append") {
      if (!args.empty())
        lv->elements.push_back(args[0]);
      return makeNull();
    }
    if (method == "pop") {
      if (lv->elements.empty())
        return makeNull();
      auto v = lv->elements.back();
      lv->elements.pop_back();
      return v;
    }
    if (method == "shift") {
      if (lv->elements.empty())
        return makeNull();
      auto v = lv->elements.front();
      lv->elements.erase(lv->elements.begin());
      return v;
    }
    if (method == "unshift") {
      if (!args.empty())
        lv->elements.insert(lv->elements.begin(), args[0]);
      return makeNull();
    }
    if (method == "contains" || method == "includes") {
      if (args.empty())
        return makeBool(false);
      for (auto &e : lv->elements)
        if (e->equals(*args[0]))
          return makeBool(true);
      return makeBool(false);
    }
    if (method == "reverse") {
      std::reverse(lv->elements.begin(), lv->elements.end());
      return obj;
    }
    if (method == "join") {
      std::string sep = args.empty() ? "" : args[0]->toString();
      std::string r;
      for (size_t i = 0; i < lv->elements.size(); i++) {
        if (i)
          r += sep;
        r += lv->elements[i]->toString();
      }
      return makeStr(r);
    }
    if (method == "slice") {
      int64_t start = args.empty() ? 0 : args[0]->asInt();
      int64_t end =
          (args.size() > 1) ? args[1]->asInt() : (int64_t)lv->elements.size();
      if (start < 0)
        start += lv->elements.size();
      if (end < 0)
        end += lv->elements.size();
      auto nl = std::make_shared<ListValue>();
      for (int64_t i = start; i < end && i < (int64_t)lv->elements.size(); i++)
        nl->elements.push_back(lv->elements[i]);
      return makeList(nl);
    }
    if (method == "map") {
      if (args.empty() || !args[0]->isFn())
        throw RuntimeError("map() requires a function", line);
      auto nl = std::make_shared<ListValue>();
      for (auto &e : lv->elements)
        nl->elements.push_back(callFunction(args[0]->asFn(), {e}, line));
      return makeList(nl);
    }
    if (method == "filter") {
      if (args.empty() || !args[0]->isFn())
        throw RuntimeError("filter() requires a function", line);
      auto nl = std::make_shared<ListValue>();
      for (auto &e : lv->elements) {
        auto r = callFunction(args[0]->asFn(), {e}, line);
        if (r->truthy())
          nl->elements.push_back(e);
      }
      return makeList(nl);
    }
    if (method == "reduce") {
      if (args.empty() || !args[0]->isFn())
        throw RuntimeError("reduce() requires a function", line);
      if (lv->elements.empty())
        return args.size() > 1 ? args[1] : makeNull();
      ValuePtr acc = (args.size() > 1) ? args[1] : lv->elements[0];
      size_t start = (args.size() > 1) ? 0 : 1;
      for (size_t i = start; i < lv->elements.size(); i++)
        acc = callFunction(args[0]->asFn(), {acc, lv->elements[i]}, line);
      return acc;
    }
    if (method == "first")
      return lv->elements.empty() ? makeNull() : lv->elements.front();
    if (method == "last")
      return lv->elements.empty() ? makeNull() : lv->elements.back();
    if (method == "clear") {
      lv->elements.clear();
      return makeNull();
    }
    if (method == "toString")
      return makeStr(obj->toString());
  }

  // Dict methods
  if (obj->isDict()) {
    auto dv = obj->asDict();
    if (method == "keys") {
      auto lv = std::make_shared<ListValue>();
      for (auto &[k, v] : dv->pairs)
        lv->elements.push_back(k);
      return makeList(lv);
    }
    if (method == "values") {
      auto lv = std::make_shared<ListValue>();
      for (auto &[k, v] : dv->pairs)
        lv->elements.push_back(v);
      return makeList(lv);
    }
    if (method == "has" || method == "contains") {
      if (args.empty())
        return makeBool(false);
      return makeBool(dv->get(args[0]->toString()) != nullptr);
    }
    if (method == "remove") {
      if (!args.empty()) {
        auto key = args[0]->toString();
        dv->pairs.erase(std::remove_if(dv->pairs.begin(), dv->pairs.end(),
                                       [&](auto &p) {
                                         return p.first->isStr() &&
                                                p.first->asStr() == key;
                                       }),
                        dv->pairs.end());
      }
      return makeNull();
    }
    if (method == "size" || method == "length" || method == "len")
      return makeInt(dv->pairs.size());
  }

  // withProperties (no-op in interpreter)
  if (method == "withProperties")
    return obj;

  throw RuntimeError("No method '" + method + "' on " + obj->typeName(), line);
}

// ─── Built-in functions
// ───────────────────────────────────────────────────────

void Interpreter::registerBuiltins() {
  auto defNative = [&](const std::string &name,
                       std::function<ValuePtr(std::vector<ValuePtr>)> fn) {
    auto fv = std::make_shared<FunctionValue>();
    fv->name = name;
    fv->isNative = true;
    fv->native = std::move(fn);
    globalEnv_->define(name, makeFn(fv), true);
  };

  // print / println
  defNative("print", [this](std::vector<ValuePtr> args) -> ValuePtr {
    for (size_t i = 0; i < args.size(); i++) {
      if (i)
        print(" ");
      print(args[i]->toString());
    }
    return makeNull();
  });
  defNative("println", [this](std::vector<ValuePtr> args) -> ValuePtr {
    for (size_t i = 0; i < args.size(); i++) {
      if (i)
        print(" ");
      print(args[i]->toString());
    }
    print("\n");
    return makeNull();
  });
  defNative("printErr", [this](std::vector<ValuePtr> args) -> ValuePtr {
    for (auto &a : args)
      printErr(a->toString());
    printErr("\n");
    return makeNull();
  });

  // input
  defNative("input", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (!args.empty())
      print(args[0]->toString());
    return makeStr(readLine());
  });

  // type / typeof
  defNative("typeOf", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty())
      return makeStr("null");
    return makeStr(args[0]->typeName());
  });
  defNative("type", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty())
      return makeStr("null");
    return makeStr(args[0]->typeName());
  });

  // toString / str
  defNative("str", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty())
      return makeStr("");
    return makeStr(args[0]->toString());
  });
  defNative("int", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty())
      return makeInt(0);
    auto &v = args[0];
    if (v->isInt())
      return v;
    if (v->isFloat())
      return makeInt((int64_t)v->asFloat());
    if (v->isStr()) {
      try {
        return makeInt(std::stoll(v->asStr()));
      } catch (...) {
      }
    }
    return makeInt(0);
  });
  defNative("float", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty())
      return makeFloat(0.0);
    auto &v = args[0];
    if (v->isFloat())
      return v;
    if (v->isInt())
      return makeFloat((double)v->asInt());
    if (v->isStr()) {
      try {
        return makeFloat(std::stod(v->asStr()));
      } catch (...) {
      }
    }
    return makeFloat(0.0);
  });
  defNative("bool", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty())
      return makeBool(false);
    return makeBool(args[0]->truthy());
  });

  // len
  defNative("len", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty())
      return makeInt(0);
    auto &v = args[0];
    if (v->isStr())
      return makeInt(v->asStr().size());
    if (v->isList())
      return makeInt(v->asList()->elements.size());
    if (v->isDict())
      return makeInt(v->asDict()->pairs.size());
    if (v->isSet())
      return makeInt(v->asSet()->elements.size());
    return makeInt(0);
  });

  // range
  defNative("range", [](std::vector<ValuePtr> args) -> ValuePtr {
    int64_t start = 0, end = 0, step = 1;
    if (args.size() == 1) {
      end = args[0]->asInt();
    } else if (args.size() >= 2) {
      start = args[0]->asInt();
      end = args[1]->asInt();
    }
    if (args.size() >= 3)
      step = args[2]->asInt();
    if (step == 0)
      step = 1;
    auto lv = std::make_shared<ListValue>();
    if (step > 0)
      for (int64_t i = start; i < end; i += step)
        lv->elements.push_back(makeInt(i));
    else
      for (int64_t i = start; i > end; i += step)
        lv->elements.push_back(makeInt(i));
    return makeList(lv);
  });

  // Math functions
  defNative("abs", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty())
      return makeInt(0);
    if (a[0]->isFloat())
      return makeFloat(std::abs(a[0]->asFloat()));
    return makeInt(std::abs(a[0]->asInt()));
  });
  defNative("sqrt", [](std::vector<ValuePtr> a) -> ValuePtr {
    return makeFloat(std::sqrt(a.empty() ? 0 : a[0]->asFloat()));
  });
  defNative("pow", [](std::vector<ValuePtr> a) -> ValuePtr {
    return makeFloat(std::pow(a.size() < 2 ? 1 : a[0]->asFloat(),
                              a.size() < 2 ? 1 : a[1]->asFloat()));
  });
  defNative("floor", [](std::vector<ValuePtr> a) -> ValuePtr {
    return makeInt((int64_t)std::floor(a.empty() ? 0 : a[0]->asFloat()));
  });
  defNative("ceil", [](std::vector<ValuePtr> a) -> ValuePtr {
    return makeInt((int64_t)std::ceil(a.empty() ? 0 : a[0]->asFloat()));
  });
  defNative("round", [](std::vector<ValuePtr> a) -> ValuePtr {
    return makeInt((int64_t)std::round(a.empty() ? 0 : a[0]->asFloat()));
  });
  defNative("max", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty())
      return makeNull();
    ValuePtr m = a[0];
    for (auto &v : a)
      if (v->asFloat() > m->asFloat())
        m = v;
    return m;
  });
  defNative("min", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty())
      return makeNull();
    ValuePtr m = a[0];
    for (auto &v : a)
      if (v->asFloat() < m->asFloat())
        m = v;
    return m;
  });
  defNative("sin", [](std::vector<ValuePtr> a) -> ValuePtr {
    return makeFloat(std::sin(a.empty() ? 0 : a[0]->asFloat()));
  });
  defNative("cos", [](std::vector<ValuePtr> a) -> ValuePtr {
    return makeFloat(std::cos(a.empty() ? 0 : a[0]->asFloat()));
  });
  defNative("tan", [](std::vector<ValuePtr> a) -> ValuePtr {
    return makeFloat(std::tan(a.empty() ? 0 : a[0]->asFloat()));
  });
  defNative("log", [](std::vector<ValuePtr> a) -> ValuePtr {
    return makeFloat(std::log(a.empty() ? 1 : a[0]->asFloat()));
  });
  defNative("log2", [](std::vector<ValuePtr> a) -> ValuePtr {
    return makeFloat(std::log2(a.empty() ? 1 : a[0]->asFloat()));
  });
  defNative("log10", [](std::vector<ValuePtr> a) -> ValuePtr {
    return makeFloat(std::log10(a.empty() ? 1 : a[0]->asFloat()));
  });

  // exit
  defNative("exit", [](std::vector<ValuePtr> args) -> ValuePtr {
    int code = args.empty() ? 0 : (int)args[0]->asInt();
    std::exit(code);
    return makeNull();
  });

  // assert
  defNative("assert", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty() || !args[0]->truthy()) {
      std::string msg =
          (args.size() > 1) ? args[1]->toString() : "Assertion failed";
      throw RuntimeError(msg, -1);
    }
    return makeNull();
  });

  // Global constants
  globalEnv_->define("PI", makeFloat(3.14159265358979323846), false);
  globalEnv_->define("E", makeFloat(2.71828182845904523536), false);
  globalEnv_->define("TAU", makeFloat(6.28318530717958647692), false);
  globalEnv_->define("PHI", makeFloat(1.61803398874989484820), false);
  globalEnv_->define("INF", makeFloat(std::numeric_limits<double>::infinity()),
                     false);
  globalEnv_->define(
      "NEG_INF", makeFloat(-std::numeric_limits<double>::infinity()), false);
  globalEnv_->define("NAN", makeFloat(std::numeric_limits<double>::quiet_NaN()),
                     false);

  globalEnv_->define("SECOND", makeInt(1), false);
  globalEnv_->define("MINUTE", makeInt(60), false);
  globalEnv_->define("HOUR", makeInt(3600), false);
  globalEnv_->define("DAY", makeInt(86400), false);
  globalEnv_->define("WEEK", makeInt(604800), false);

  globalEnv_->define("RUNTIME_VERSION", makeStr("1.0.0"), false);
  globalEnv_->define("LANG_VERSION", makeStr("1.0.0"), false);
  globalEnv_->define("BUILD_MODE", makeStr("debug"), false);
  globalEnv_->define("DEBUG_MODE", makeBool(false), false);

  // Property constants (runtime no-ops)
  for (auto &p : {"PROP_FROZEN",   "PROP_READONLY",     "PROP_IMMUTABLE",
                  "PROP_REACTIVE", "PROP_SERIALIZABLE", "PROP_UNSAFE",
                  "PROP_VOLATILE", "PROP_DEBUG",        "PROP_HIDDEN",
                  "PROP_PRIVATE",  "PROP_PUBLIC",       "PROP_FINAL",
                  "PROP_CONST",    "PROP_STATIC",       "PROP_SYNC",
                  "PROP_ASYNC",    "PROP_LAZY",         "PROP_CACHED",
                  "PROP_TEMP",     "PROP_NATIVE",       "PROP_PROTECTED",
                  "PROP_INTERNAL", "PROP_EXPERIMENTAL", "PROP_DEPRECATED",
                  "PROP_LOCKED",   "PROP_OBSERVABLE"}) {
    globalEnv_->define(p, makeStr(p), false);
  }

  // Terminal namespace
  auto terminal = std::make_shared<DictValue>();
  // Terminal.Out, Terminal.Warn, Terminal.Err are stream markers

  // We use a trick: Terminal is a dict with Out/Warn/Err as special sentinel
  // strings
  terminal->set("Out", makeStr("<Terminal.Out>"));
  terminal->set("Warn", makeStr("<Terminal.Warn>"));
  terminal->set("Err", makeStr("<Terminal.Err>"));

  // Terminal.In is a special object
  auto terminalIn = std::make_shared<DictValue>();
  // Terminal.In.read() - handled via callBuiltinMethod
  terminal->set("In", makeDict(terminalIn));

  globalEnv_->define("Terminal", makeDict(terminal), false);
  globalEnv_->define("stdout", makeStr("<Terminal.Out>"), false);
  globalEnv_->define("stderr", makeStr("<Terminal.Err>"), false);
  globalEnv_->define("stdin", makeStr("<Terminal.In>"), false);
  globalEnv_->define("stdwarn", makeStr("<Terminal.Warn>"), false);

  // Terminal.In.read()
  defNative("read", [this](std::vector<ValuePtr>) -> ValuePtr {
    return makeStr(readLine());
  });

  // Conversion utils
  defNative("ord", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty() || !args[0]->isStr() || args[0]->asStr().empty())
      return makeInt(0);
    return makeInt((int64_t)(unsigned char)args[0]->asStr()[0]);
  });
  defNative("chr", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty())
      return makeStr("");
    return makeStr(std::string(1, (char)args[0]->asInt()));
  });
}
