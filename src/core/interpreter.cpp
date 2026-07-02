#include "../include/interpreter.hpp"
#include "../include/error.hpp"
#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/version.hpp"
#include <algorithm>
#include <unordered_set>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>



// --- Constructor
// --------------------------------------------------------------

Interpreter::Interpreter(bool debugMode)
    : debugMode_(debugMode), stdout_(&std::cout), stderr_(&std::cerr),
      stdin_(&std::cin) {
  globalEnv_ = std::make_shared<Environment>();
  currentEnv_ = globalEnv_;
  registerBuiltins();
}

// --- run
// ----------------------------------------------------------------------

void Interpreter::run(const std::vector<StmtPtr> &stmts, const std::string& filepath) {
  std::string absPath = filepath;
  if (filepath != "<repl>" && !filepath.empty()) {
      try {
          absPath = std::filesystem::absolute(filepath).string();
      } catch (...) {}
  }
  fileStack_.push_back(absPath);
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
  fileStack_.pop_back();
}

void Interpreter::injectBuiltinsIntoGlobal() {
    if (!builtinsRegistry_.isNull() && builtinsRegistry_.isDict()) {
        for (const auto& kv : builtinsRegistry_.asDict()->pairs) {
            globalEnv_->define(kv.first, kv.second, false);
        }
    }
}

// --- Environment helpers
// ------------------------------------------------------

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

#define trace(msg) do { if (debugMode_) { this->trace(msg); } } while(0)

// --- Output helpers
// -----------------------------------------------------------

void Interpreter::print(const std::string &s) { *stdout_ << s; }
void Interpreter::printErr(const std::string &s) { *stderr_ << s; }
std::string Interpreter::readLine() {
  std::string line;
  std::getline(*stdin_, line);
  return line;
}



// --- execStmt
// -----------------------------------------------------------------

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
        else if constexpr (std::is_same_v<T, ThrowStmt>)
          execThrow(node);
        else if constexpr (std::is_same_v<T, BreakStmt>)
          throw BreakSignal{};
        else if constexpr (std::is_same_v<T, ContinueStmt>)
          throw ContinueSignal{};
        else if constexpr (std::is_same_v<T, ExprStmt>)
          evalExpr(*node.expr);
        else if constexpr (std::is_same_v<T, StreamOutStmt>)
          execStreamOut(node);

        else if constexpr (std::is_same_v<T, ImportStmt>)
          execImport(node);
        else if constexpr (std::is_same_v<T, ExportStmt>)
          execExport(node);
        else if constexpr (std::is_same_v<T, ExposeStmt>)
          execExpose(node);
        else if constexpr (std::is_same_v<T, OverwriteStmt>)
          execOverwrite(node);
        else if constexpr (std::is_same_v<T, UnsafeStmt>)
          execUnsafe(node);
        else if constexpr (std::is_same_v<T, DeferStmt>)
          execDefer(node);
        else if constexpr (std::is_same_v<T, TryCatchStmt>)
          execTryCatch(node);
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
  if (debugMode_) trace("VarDecl: " + s.keyword + " " + s.name);
  ValuePtr val = makeNull();
  if (s.initializer)
    val = evalExpr(*s.initializer);

  bool mutable_ = (s.keyword != "let");
  currentEnv_->define(s.name, val, mutable_);
}

void Interpreter::execFnDecl(const FnDeclStmt &s) {
  trace("FnDecl: " + s.name);
  auto fn = std::make_shared<FunctionValue>();
  fn->name = s.name;
  fn->params = s.params;
  fn->retType = s.retType;
  fn->body = s.body.get();
  fn->closure = currentEnv_;
  fn->definedInFile = currentFile();
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
      fv->definedInFile = currentFile();
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
  if (cond.truthy()) {
    trace("If: condition true, executing thenBranch");
    execStmt(*s.thenBranch);
  }
  else if (s.elseBranch) {
    trace("If: condition false, executing elseBranch");
    execStmt(*s.elseBranch);
  } else {
    trace("If: condition false");
  }
}

void Interpreter::execWhile(const WhileStmt &s) {
  trace("While: starting loop");
  while (true) {
    auto cond = evalExpr(*s.cond);
    if (!cond.truthy()) {
      trace("While: condition false, breaking");
      break;
    }
    try {
      execStmt(*s.body);
    } catch (BreakSignal &) {
      trace("While: caught break");
      break;
    } catch (ContinueSignal &) {
      trace("While: caught continue");
      continue;
    }
  }
}

void Interpreter::execFor(const ForStmt &s) {
  trace("For: evaluating iterable");
  auto iterable = evalExpr(*s.iterable);
  auto doBody = [&](ValuePtr item) {
    pushEnv();
    currentEnv_->define(s.var, item, true);
    try {
      execStmt(*s.body);
    } catch (BreakSignal &) {
      trace("For: caught break");
      popEnv();
      return false;
    } catch (ContinueSignal &) {
      trace("For: caught continue");
    }
    popEnv();
    return true;
  };

  if (iterable.isList()) {
    for (auto &elem : iterable.asList()->elements)
      if (!doBody(elem))
        break;
  } else if (iterable.isStr()) {
    for (char c : iterable.asStr())
      if (!doBody(makeStr(std::string(1, c))))
        break;
  } else if (iterable.isDict()) {
    for (auto &[k, v] : iterable.asDict()->pairs)
      if (!doBody(makeStr(k)))
        break;
  } else if (iterable.isSet()) {
    for (auto &elem : iterable.asSet()->elements)
      if (!doBody(elem))
        break;
  } else {
    throw RuntimeError(
        "Value of type '" + iterable.typeName() + "' is not iterable", s.line);
  }
}

void Interpreter::execReturn(const ReturnStmt &s) {
  ValuePtr val = makeNull();
  if (s.value)
    val = evalExpr(*s.value);
  trace("Return: returning " + val.typeName());
  throw ReturnSignal{val};
}

void Interpreter::execThrow(const ThrowStmt &s) {
  ValuePtr val = evalExpr(*s.value);
  trace("Throw: " + val.toString());

  if (val.isDict()) {
      auto dict = val.asDict();
      std::string msg = "Unknown error";
      std::string code = "RUNTIME_500";
      std::string hint = "";
      std::string severity = "E";

      for (const auto& kv : dict->pairs) {
          std::string key = kv.first;
          if (key == "message") msg = kv.second.isStr() ? kv.second.asStr() : kv.second.toString();
          else if (key == "code") code = kv.second.isStr() ? kv.second.asStr() : kv.second.toString();
          else if (key == "hint") hint = kv.second.isStr() ? kv.second.asStr() : kv.second.toString();
          else if (key == "severity") severity = kv.second.isStr() ? kv.second.asStr() : kv.second.toString();
      }
      
      if (code.find(severity + "_") != 0) {
          code = severity + "_" + code;
      }
      
      if (severity == "W") {
          std::string colorCode = "\033[33;1m"; // Yellow
          std::string loc = s.line > 0 ? " line " + std::to_string(s.line) : "";
          std::cerr << colorCode << "[" << code << "]" << loc << ": " << msg << "\033[0m\n";
          
          if (!hint.empty()) std::cerr << "  \033[33mhint: " << hint << "\033[0m\n";
          return;
      }
      
      throw RuntimeError(msg, s.line, code, hint);
  }

  throw RuntimeError(val.isStr() ? val.asStr() : val.toString(), s.line);
}

void Interpreter::execStreamOut(const StreamOutStmt &s) {
  auto val = evalExpr(*s.value);
  print(val.toString());
}



void Interpreter::execImport(const ImportStmt &s) {
  trace("Import: " + s.pkg + " as " + s.alias);

  std::string alias = s.alias;
  if (alias.empty()) {
    size_t slash = s.pkg.find_last_of('/');
    if (slash != std::string::npos) {
      alias = s.pkg.substr(slash + 1);
    } else {
      alias = s.pkg;
      // @ handling removed
    }
  }

  std::string pkgKey = s.pkg;

  bool noLibName = false;
  for (const auto& flag : s.flags) {
      if (flag == "NOLIBNAME" || flag == "FULL") {
          noLibName = true;
      }
  }

  auto defineModuleExports = [&](ValuePtr dictVal) {
      if (noLibName) {
          if (dictVal.isDict()) {
               for (const auto& [k, v] : dictVal.asDict()->pairs) {
                   currentEnv_->define(k, v, true);
               }
          }
      } else {
          currentEnv_->define(alias, dictVal, true);
      }
  };

  // Check if module is already cached (try both with and without @)
  if (exportedModules_.count(s.pkg)) {
    defineModuleExports(exportedModules_[s.pkg]);
    return;
  }
  if (exportedModules_.count(pkgKey)) {
    defineModuleExports(exportedModules_[pkgKey]);
    return;
  }


  std::string path = s.pkg;
  if (path.length() < 5 || path.substr(path.length() - 5) != ".sfpp") {
    path += ".sfpp";
  }

  // Try direct path first, then packages/ directory
    // Try direct path first
  std::ifstream f(path);
  if (!f) {
    // Cek apakah package diawali dengan "std/"
    bool isStdModule = (s.pkg.rfind("std/", 0) == 0);

    if (isStdModule) {
      // 1. Coba cari di folder stdlib yang berada satu tingkat/folder dengan binary 'combust'
      // (Ini bekerja untuk skenario ./build/combust karena std/ dicopy ke build/std/)
      std::string exeDirStdPath = "build/" + path; // Mengarah ke build/std/...
      
      f.open(exeDirStdPath);
      if (f) {
        path = exeDirStdPath;
      } else {
        // 2. Fallback: Coba cari di folder development asli (src/stdlib/...)
        std::string srcStdPath = "src/stdlib/" + path; // Mengarah ke src/stdlib/...
        f.open(srcStdPath);
        if (f) {
          path = srcStdPath;
        } else {
          // Jika keduanya gagal, baru lempar error
          throw RuntimeError("Cannot open standard module '" + s.pkg + "' at " + path);
        }
      }
    } else {
      // --- LOGIKA LAMA UNTUK PACKAGES/ NON-STD ---
      std::string pkgPath = "packages/" + path;
      f.open(pkgPath);
      if (f) {
        path = pkgPath;
      } else {
        std::string stripped = s.pkg;
        if (!stripped.empty() && stripped[0] == '@') stripped = s.pkg.substr(1);
        std::string altPath = "packages/" + stripped + ".sfpp";
        f.open(altPath);
        if (f) {
          path = altPath;
        } else {
          throw RuntimeError("Cannot open module '" + s.pkg + "'.");
        }
      }
    }
  }



  std::ostringstream ss;
  ss << f.rdbuf();
  std::string src = ss.str();

  Lexer lex(src, path);
  auto tokens = lex.tokenize();
  Parser par(std::move(tokens));
  auto stmts = par.parse();

  moduleASTs_.push_back(std::move(stmts));
  auto& runStmts = moduleASTs_.back();

  auto moduleEnv = std::make_shared<Environment>(globalEnv_);
  auto savedEnv = currentEnv_;
  currentEnv_ = moduleEnv;

  std::string absPath = path;
  if (path != "<repl>" && !path.empty()) {
      try {
          absPath = std::filesystem::absolute(path).string();
      } catch (...) {}
  }
  fileStack_.push_back(absPath);

  try {
    for (auto& stmt : runStmts) {
      execStmt(*stmt);
    }
  } catch (...) {
    currentEnv_ = savedEnv;
    fileStack_.pop_back();
    throw;
  }
  currentEnv_ = savedEnv;
  fileStack_.pop_back();

  auto ns = std::make_shared<DictValue>();
  for (const auto& kv : moduleEnv->vars()) {
    if (kv.first != "__export_alias__") {
      ns->set(kv.first, kv.second.value);
    }
  }

  ValuePtr dictVal = makeDict(ns);
  defineModuleExports(dictVal);
  
  if (moduleEnv->hasLocal("__export_alias__")) {
      std::string exportAlias = moduleEnv->get("__export_alias__", 0)->asStr();
      exportedModules_[exportAlias] = dictVal;
  }
}

void Interpreter::execExport(const ExportStmt &s) {
  auto env = currentEnv_;
  while (env && env->parent() != globalEnv_ && env->parent() != nullptr) {
      env = env->parent();
  }
  if (!env) env = currentEnv_;
  env->define("__export_alias__", makeStr(s.alias), true);
}

void Interpreter::execExpose(const ExposeStmt &s) {
  std::string file = currentFile();
  if (file.find("std/") == std::string::npos && file.find("std\\") == std::string::npos) {
      throw RuntimeError("The 'expose' keyword can only be used inside the standard library (std/)", s.line, "E_NATIVE_403");
  }
  
  if (!builtinsRegistry_.isNull() && builtinsRegistry_.isDict()) {
      auto dict = builtinsRegistry_.asDict();
      if (dict->has(s.name)) {
          currentEnv_->define(s.alias, dict->get(s.name));
      } else {
          throw RuntimeError("Cannot expose unknown native symbol '" + s.name + "'", s.line, "E_NATIVE_404");
      }
  }
}

void Interpreter::execOverwrite(const OverwriteStmt &s) {
  std::string file = currentFile();
  if (file.find("std/") == std::string::npos && file.find("std\\") == std::string::npos) {
      throw RuntimeError("The 'overwrite' keyword can only be used inside the standard library (std/)", s.line, "E_NATIVE_403");
  }

  auto val = evalExpr(*s.value);
  size_t dotPos = s.target.find('.');
  if (dotPos == std::string::npos) {
      if (currentEnv_->has(s.target)) {
          currentEnv_->set(s.target, val);
      } else {
          currentEnv_->define(s.target, val);
      }
  } else {
      std::string base = s.target.substr(0, dotPos);
      std::string prop = s.target.substr(dotPos + 1);
      auto baseVal = currentEnv_->get(base, s.line);
      if (baseVal.isDict()) {
          baseVal.asDict()->set(prop, val);
      } else {
          throw RuntimeError("Cannot overwrite property on non-dict target '" + base + "'", s.line, "E_RUNTIME_405");
      }
  }
}

void Interpreter::execUnsafe(const UnsafeStmt &s) {
  std::string file = currentFile();
  if (file.find("std/") == std::string::npos && file.find("std\\") == std::string::npos) {
      throw RuntimeError("The 'unsafe' keyword can only be used inside the standard library (std/)", s.line, "E_NATIVE_403");
  }

  trace("Entering unsafe block");
  execStmt(*s.body);
  trace("Leaving unsafe block");
}

void Interpreter::execDefer(const DeferStmt &s) {
  if (!deferStack_.empty())
    deferStack_.back().push_back(s.body.get());
}

void Interpreter::execTryCatch(const TryCatchStmt &s) {
  auto runFinally = [&]() {
    if (s.finallyBody) {
      try { execStmt(*s.finallyBody); } catch (...) {}
    }
  };

  try {
    execStmt(*s.tryBody);
    runFinally();
  } catch (const FatalError&) {
    // FatalError is always re-thrown - cannot be caught
    runFinally();
    throw;
  } catch (const SulfurError& err) {
    runFinally();
    if (s.catchBody) {
      // Build error dict and bind to catchVar
      auto errDict = std::make_shared<DictValue>();
      errDict->set("message", makeStr(err.what()));
      errDict->set("code",    makeStr(err.code));
      errDict->set("hint",    makeStr(err.hint));
      errDict->set("line",    makeInt(err.line));
      auto catchEnv = std::make_shared<Environment>(currentEnv_);
      catchEnv->define(s.catchVar, makeDict(errDict), true);
      execBlock(*std::get_if<BlockStmt>(&s.catchBody->data), catchEnv);
    }
  } catch (const std::exception& ex) {
    runFinally();
    if (s.catchBody) {
      auto errDict = std::make_shared<DictValue>();
      errDict->set("message", makeStr(std::string(ex.what())));
      errDict->set("code",    makeStr("E_RUNTIME_500"));
      errDict->set("hint",    makeStr(""));
      errDict->set("line",    makeInt(-1));
      auto catchEnv = std::make_shared<Environment>(currentEnv_);
      catchEnv->define(s.catchVar, makeDict(errDict), true);
      execBlock(*std::get_if<BlockStmt>(&s.catchBody->data), catchEnv);
    }
  }
}

// --- evalExpr
// -----------------------------------------------------------------

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
          if (auto* ident = std::get_if<IdentExpr>(&node.operand->data)) {
            auto addr = currentEnv_->getAddr(ident->name);
            if (!addr) {
              throw MemoryError("Undefined variable address: " + ident->name, node.line);
            }
            auto pv = std::make_shared<PtrValue>();
            pv->target = addr;
            return Value(pv);
          } else if (auto* mem = std::get_if<MemberExpr>(&node.operand->data)) {
            auto obj = evalExpr(*mem->object);
            if (obj.isStructInst()) {
              auto inst = obj.asStructInst();
              return makePtr(&inst->fields[mem->member]);
            } else if (obj.isClassInst()) {
              auto inst = obj.asClassInst();
              auto addr = inst->members->getAddr(mem->member);
              if (addr) {
                auto pv = std::make_shared<PtrValue>();
                pv->target = addr;
                return Value(pv);
              }
            }
            throw MemoryError("Cannot take address of member '" + mem->member + "'", node.line);
          }
          throw MemoryError("Cannot take address of rvalue", node.line);
        }
        if constexpr (std::is_same_v<T, DerefExpr>) {
          auto obj = evalExpr(*node.operand);
          if (!obj.isPtr()) {
            throw TypeError("Cannot dereference non-pointer of type " + obj.typeName(), node.line);
          }
          auto target = obj.asPtr()->target;
          if (!target || (*target).isNull()) {
            throw MemoryError("Null pointer dereference", node.line);
          }
          return *target;
        }
        if constexpr (std::is_same_v<T, DeleteExpr>) {
          return evalDelete(node);
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
          if (val.isList()) {
            std::string joined;
            bool first = true;
            for (auto &e : val.asList()->elements) {
              if (!first)
                joined += sep;
              joined += e.toString();
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
          std::string s = val.toString();
          for (int i = 0; i < n; i++)
            result += s;
          continue;
        }
      }
      result += val.toString();
    }
  }
  return makeStr(result);
}

ValuePtr Interpreter::evalIdent(const IdentExpr &e) {
  if (e.name == "this" && !currentEnv_->has("this")) {
    return makeStr(currentFile());
  }
  return currentEnv_->get(e.name, e.line);
}

ValuePtr Interpreter::evalBinary(const BinaryExpr &e) {
  // Short-circuit for && and ||
  if (e.op == "&&") {
    auto l = evalExpr(*e.left);
    if (!l.truthy())
      return makeBool(false);
    return makeBool(evalExpr(*e.right).truthy());
  }
  if (e.op == "||") {
    auto l = evalExpr(*e.left);
    if (l.truthy())
      return makeBool(true);
    return makeBool(evalExpr(*e.right).truthy());
  }

  // Stream output / Bitwise Left Shift operator <<
  if (e.op == "<<") {
    auto left = evalExpr(*e.left);
    std::string stream;
    if (left.isStr()) {
      stream = left.asStr();
    } else if (left.isDict()) {
      if (auto marker = left.asDict()->get("__stream__"); !marker.isNull()) {
        stream = marker.asStr();
      }
    }

    if (stream == "<Terminal.Out>" || stream == "<stdout>" ||
        stream == "<Terminal.Err>" || stream == "<stderr>" ||
        stream == "<Terminal.Warn>" || stream == "<stdwarn>" ||
        stream == "<Terminal.Return>") {
      auto right = evalExpr(*e.right);
      
      if (stream == "<Terminal.Return>") {
        std::string severity = "E";
        std::string msg = "Error occurred";
        if (right.isFn()) {
          std::string name = right.asFn()->name;
          if (name == "TIO.E" || name == "TIO.Err" || name == "TIO.err") {
            severity = "E";
            msg = "Error occurred";
          } else if (name == "TIO.FE" || name == "TIO.F_Err" || name == "TIO.f_err") {
            severity = "FE";
            msg = "Fatal error occurred";
          } else if (name == "TIO.W" || name == "TIO.w") {
            severity = "W";
            msg = "Warning occurred";
          }
        } else if (right.isDict()) {
          auto sVal = right.asDict()->get("severity");
          auto mVal = right.asDict()->get("message");
          auto cVal = right.asDict()->get("code");
          auto catVal = right.asDict()->get("category");
          if (!sVal.isNull()) severity = sVal.toString();
          if (!mVal.isNull()) msg = mVal.toString();
          // Prepend [category] and/or [code] to the message if present
          std::string prefix;
          if (!catVal.isNull()) prefix += "[" + catVal.toString() + "] ";
          if (!cVal.isNull())   prefix += "[" + cVal.toString() + "] ";
          if (!prefix.empty()) msg = prefix + msg;
        } else {
          msg = right.toString();
        }

        if (severity == "W") {
          printErr("[WARN] line " + std::to_string(e.line) + ": " + msg + "\n");
        } else if (severity == "FE") {
          throw FatalError(msg, e.line, "FE_TIO_Err");
        } else {
          throw RuntimeError(msg, e.line, "E_TIO_Err");
        }
        return left;
      }

      std::string msg = right.toString();
      if (stream == "<Terminal.Out>" || stream == "<stdout>") {
        print(msg);
      } else if (stream == "<Terminal.Err>" || stream == "<stderr>") {
        printErr(msg);
      } else if (stream == "<Terminal.Warn>" || stream == "<stdwarn>") {
        printErr("[WARN] " + msg);
      }
      return left; // return stream for chaining
    } else {
      auto right = evalExpr(*e.right);
      if (left.isInt() && right.isInt()) {
        return makeInt(left.asInt() << right.asInt());
      }
      throw TypeError("Cannot perform bitwise left shift on non-integer types", e.line);
    }
  }

  // Stream input / Bitwise Right Shift operator >>
  if (e.op == ">>") {
    auto left = evalExpr(*e.left);
    std::string stream;
    if (left.isStr()) {
      stream = left.asStr();
    } else if (left.isDict()) {
      if (auto marker = left.asDict()->get("__stream__"); !marker.isNull()) {
        stream = marker.asStr();
      }
    }

    if (stream == "<Terminal.In>" || stream == "<stdin>") {
      std::string word;
      if (stdin_) {
        *stdin_ >> word;
      }
      auto newVal = makeStr(word);

      if (auto *id = std::get_if<IdentExpr>(&e.right->data)) {
        currentEnv_->set(id->name, newVal, e.line);
      } else if (auto *idx = std::get_if<IndexExpr>(&e.right->data)) {
        auto obj = evalExpr(*idx->object);
        auto key = evalExpr(*idx->index);
        if (obj.isList()) {
          auto &elems = obj.asList()->elements;
          int64_t i = key.asInt();
          if (i < 0) i += elems.size();
          if (i < 0 || (size_t)i >= elems.size())
            throw IndexError("List index out of bounds", e.line);
          elems[i] = newVal;
        } else if (obj.isDict()) {
          obj.asDict()->set(key.toString(), newVal);
        } else {
          throw TypeError("Cannot index-assign to " + obj.typeName(), e.line);
        }
      } else if (auto *mem = std::get_if<MemberExpr>(&e.right->data)) {
        auto obj = evalExpr(*mem->object);
        if (mem->op == "->") {
          if (!obj.isPtr()) {
            throw TypeError("Cannot use '->' on non-pointer object of type '" + obj.typeName() + "'. Use '.' instead.", e.line);
          }
          auto target = obj.asPtr()->target;
          if (!target || (*target).isNull()) {
            throw RuntimeError("Null pointer dereference", e.line, "E_RUNTIME_403", "Ensure the pointer was initialized using a valid reference.");
          }
          obj = *target;
        } else if (mem->op == "." || mem->op == "?.") {
          if (obj.isPtr()) {
            throw TypeError("Cannot use '" + mem->op + "' on pointer object. Use '->' instead.", e.line);
          }
        }
        if (obj.isClassInst()) {
          obj.asClassInst()->members->set(mem->member, newVal, e.line);
        } else if (obj.isStructInst()) {
          obj.asStructInst()->fields[mem->member] = newVal;
        } else if (obj.isDict()) {
          obj.asDict()->set(mem->member, newVal);
        } else {
          throw RuntimeError("Cannot assign member '" + mem->member + "' on " + obj.typeName(), e.line);
        }
      } else {
        throw RuntimeError("Invalid stream extraction target", e.line);
      }
      return left; // return stream for chaining
    } else {
      auto right = evalExpr(*e.right);
      if (left.isInt() && right.isInt()) {
        return makeInt(left.asInt() >> right.asInt());
      }
      throw TypeError("Cannot perform bitwise right shift on non-integer types", e.line);
    }
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
    return makeInt(l.asInt() & r.asInt());
  if (e.op == "|")
    return makeInt(l.asInt() | r.asInt());
  if (e.op == "^")
    return makeInt(l.asInt() ^ r.asInt());

  throw RuntimeError("Unknown binary operator: " + e.op, e.line);
}

ValuePtr Interpreter::applyBinaryArith(const std::string &op, ValuePtr l,
                                       ValuePtr r, int line) {
  // String concatenation
  if (op == "+") {
    if (l.isStr() || r.isStr())
      return makeStr(l.toString() + r.toString());
    if (l.isList() && r.isList()) {
      auto res = std::make_shared<ListValue>();
      for (auto &e : l.asList()->elements)
        res->elements.push_back(e);
      for (auto &e : r.asList()->elements)
        res->elements.push_back(e);
      return makeList(res);
    }
  }

  // Numeric
  bool useComplex = l.isComplex() || r.isComplex();
  if (useComplex) {
      std::complex<double> a = l.asComplex(), b = r.asComplex();
      if (op == "+") return makeComplex(a + b);
      if (op == "-") return makeComplex(a - b);
      if (op == "*") return makeComplex(a * b);
      if (op == "/") {
          if (b == 0.0) throw MathError("Division by zero", line);
          return makeComplex(a / b);
      }
      if (op == "**") return makeComplex(std::pow(a, b));
      if (op == "%") throw MathError("Modulo operation is not defined for complex numbers", line);
  }

  bool useFloat = l.isFloat() || r.isFloat();
  if (useFloat) {
    double a = l.asFloat(), b = r.asFloat();
    if (op == "+")
      return makeFloat(a + b);
    if (op == "-")
      return makeFloat(a - b);
    if (op == "*")
      return makeFloat(a * b);
    if (op == "/") {
      if (b == 0.0)
        throw MathError("Division by zero", line);
      return makeFloat(a / b);
    }
    if (op == "%")
      return makeFloat(std::fmod(a, b));
    if (op == "**")
      return makeFloat(std::pow(a, b));
  } else {
    int64_t a = l.asInt(), b = r.asInt();
    if (op == "+")
      return makeInt(a + b);
    if (op == "-")
      return makeInt(a - b);
    if (op == "*")
      return makeInt(a * b);
    if (op == "/") {
      if (b == 0)
        throw MathError("Division by zero", line);
      return makeInt(a / b);
    }
    if (op == "%") {
      if (b == 0)
        throw MathError("Modulo by zero", line);
      return makeInt(a % b);
    }
    if (op == "**")
      return makeFloat(std::pow((double)a, (double)b));
  }
  throw RuntimeError("Cannot apply '" + op + "' to types " + l.typeName() +
                         " and " + r.typeName(),
                     line);
}

ValuePtr Interpreter::applyBinaryCompare(const std::string &op, ValuePtr l,
                                         ValuePtr r, int line) {
  if (op == "==")
    return makeBool(l.equals(r));
  if (op == "!=")
    return makeBool(!l.equals(r));

  auto cmp = [&]() -> int {
    if (l.isInt() && r.isInt())
      return (l.asInt() < r.asInt()) ? -1 : (l.asInt() > r.asInt()) ? 1 : 0;
    if ((l.isInt() || l.isFloat()) && (r.isInt() || r.isFloat())) {
      double a = l.asFloat(), b = r.asFloat();
      return (a < b) ? -1 : (a > b) ? 1 : 0;
    }
    if (l.isStr() && r.isStr())
      return l.asStr().compare(r.asStr());
    throw TypeError("Cannot compare " + l.typeName() + " and " + r.typeName(),
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
    return makeBool(!val.truthy());
  if (e.op == "-") {
    if (val.isInt())
      return makeInt(-val.asInt());
    if (val.isFloat())
      return makeFloat(-val.asFloat());
    if (val.isComplex())
      return makeComplex(-val.asComplex());
    throw TypeError("Cannot negate " + val.typeName(), e.line);
  }
  if (e.op == "~") {
    if (val.isInt())
      return makeInt(~val.asInt());
    throw TypeError("Cannot bitwise-not " + val.typeName(), e.line);
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
    currentEnv_->set(id->name, newVal, e.line);
  } else if (auto *idx = std::get_if<IndexExpr>(&e.target->data)) {
    auto obj = evalExpr(*idx->object);
    auto key = evalExpr(*idx->index);
    if (obj.isList()) {
      auto &elems = obj.asList()->elements;
      int64_t i = key.asInt();
      if (i < 0)
        i += elems.size();
      if (i < 0 || (size_t)i >= elems.size())
        throw IndexError("List index out of bounds", e.line);
      elems[i] = newVal;
    } else if (obj.isDict()) {
      obj.asDict()->set(key.toString(), newVal);
    } else {
      throw TypeError("Cannot index-assign to " + obj.typeName(), e.line);
    }
  } else if (auto *mem = std::get_if<MemberExpr>(&e.target->data)) {
    auto obj = evalExpr(*mem->object);
    if (mem->op == "->") {
      if (!obj.isPtr()) {
        throw TypeError("Cannot use '->' on non-pointer object of type '" + obj.typeName() + "'. Use '.' instead.", e.line);
      }
      auto target = obj.asPtr()->target;
      if (!target || (*target).isNull()) {
        throw RuntimeError("Null pointer dereference", e.line, "E_RUNTIME_403");
      }
      obj = *target;
    } else if (mem->op == "." || mem->op == "?.") {
      if (obj.isPtr()) {
        throw TypeError("Cannot use '" + mem->op + "' on pointer object. Use '->' instead.", e.line);
      }
      if (obj.isClassDef()) {
        throw TypeError("Cannot use '" + mem->op + "' on Class. Use '::' instead.", e.line);
      }
    } else if (mem->op == "::") {
      if (obj.isClassInst() || obj.isStructInst()) {
        throw TypeError("Cannot use '::' on instance of '" + obj.typeName() + "'. Use '.' instead.", e.line);
      }
      if (obj.isPtr()) {
        throw TypeError("Cannot use '::' on pointer object. Use '->' instead.", e.line);
      }
    }

    if (obj.isClassInst()) {
      obj.asClassInst()->members->set(mem->member, newVal, e.line);
    } else if (obj.isStructInst()) {
      obj.asStructInst()->fields[mem->member] = newVal;
    } else if (obj.isDict()) {
      obj.asDict()->set(mem->member, newVal);
    } else {
      throw RuntimeError("Cannot assign member '" + mem->member + "' on " +
                             obj.typeName(),
                         e.line);
    }
  } else {
    throw RuntimeError("Invalid assignment target", e.line);
  }

  trace("Assign: assigned to " + newVal.typeName());
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
    if (obj.isNull() && mem->safe) return makeNull();

    if (mem->op == "->") {
      if (!obj.isPtr()) {
        throw TypeError("Cannot use '->' on non-pointer object of type '" + obj.typeName() + "'. Use '.' instead.", e.line);
      }
      auto target = obj.asPtr()->target;
      if (!target || (*target).isNull()) {
        throw RuntimeError("Null pointer dereference", e.line, "E_RUNTIME_403");
      }
      obj = *target;
    } else if (mem->op == "." || mem->op == "?.") {
      if (obj.isPtr()) {
        throw TypeError("Cannot use '" + mem->op + "' on pointer object. Use '->' instead.", e.line);
      }
      if (obj.isClassDef()) {
        throw TypeError("Cannot use '" + mem->op + "' on Class. Use '::' instead.", e.line);
      }
    } else if (mem->op == "::") {
      if (obj.isClassInst() || obj.isStructInst()) {
        throw TypeError("Cannot use '::' on instance of '" + obj.typeName() + "'. Use '.' instead.", e.line);
      }
      if (obj.isPtr()) {
        throw TypeError("Cannot use '::' on pointer object. Use '->' instead.", e.line);
      }
    }

    if (obj.isClassInst()) {
      auto inst = obj.asClassInst();
      if (inst->members->hasLocal(mem->member) || inst->members->has(mem->member)) {
        return callMethod(inst, mem->member, std::move(args), e.line);
      }
      throw RuntimeError("Method '" + mem->member + "' not found on " + inst->def->name, e.line);
    }
    
    if (obj.isStructInst()) {
      auto inst = obj.asStructInst();
      auto it = inst->fields.find(mem->member);
      if (it != inst->fields.end() && it->second.isFn()) {
        return callFunction(it->second.asFn(), std::move(args), e.line);
      }
      throw RuntimeError("Method/Field '" + mem->member + "' not found on struct " + inst->def->name, e.line);
    }
    
    if (obj.isDict()) {
      auto v = obj.asDict()->get(mem->member);
      if (!v.isNull() && v.isFn()) return callFunction(v.asFn(), std::move(args), e.line);
      return callBuiltinMethod(obj, mem->member, std::move(args), e.line);
    }
    
    // For lists, strings, etc.
    return callBuiltinMethod(obj, mem->member, std::move(args), e.line);
  }

  // Evaluate callee normally
  auto callee = evalExpr(*e.callee);

  if (callee.isFn()) {
    return callFunction(callee.asFn(), std::move(args), e.line);
  }

  if (callee.isClassDef()) {
    // Instantiate class
    auto def = std::get<std::shared_ptr<ClassDef>>(callee.data);
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
        if (mv.isFn())
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

  if (callee.isStructDef()) {
    auto def = std::get<std::shared_ptr<StructDef>>(callee.data);
    auto inst = std::make_shared<StructInstance>();
    inst->def = def;
    for (size_t i = 0; i < def->fields.size(); i++) {
      auto &[fname, ftype] = def->fields[i];
      inst->fields[fname] = (i < args.size()) ? args[i] : makeNull();
    }
    return makeStructInst(inst);
  }

  // Built-in method on a class instance (callee is a member expr result)
  throw RuntimeError("'" + callee.typeName() + "' is not callable", e.line);
}

ValuePtr Interpreter::callFunction(std::shared_ptr<FunctionValue> fn,
                                   std::vector<ValuePtr> args, int line) {
  trace("Call: function '" + fn->name + "'");
  if (fn->isNative) {
    return fn->native(std::move(args));
  }

  if (!fn->body)
    throw RuntimeError("Function '" + fn->name + "' has no body", line);

  struct FileStackGuard {
    std::vector<std::string>& stack;
    bool pushed;
    FileStackGuard(std::vector<std::string>& s, const std::string& file) : stack(s), pushed(!file.empty()) {
      if (pushed) stack.push_back(file);
    }
    ~FileStackGuard() {
      if (pushed) stack.pop_back();
    }
  } guard(fileStack_, fn->definedInFile);

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
  trace("Call: method '" + name + "' on " + inst->def->name);
  if (!inst->members->has(name))
    throw RuntimeError("Method '" + name + "' not found", line);
  auto mv = inst->members->get(name);
  if (!mv.isFn())
    throw RuntimeError("'" + name + "' is not a method", line);
  auto fn = mv.asFn();

  struct FileStackGuard {
    std::vector<std::string>& stack;
    bool pushed;
    FileStackGuard(std::vector<std::string>& s, const std::string& file) : stack(s), pushed(!file.empty()) {
      if (pushed) stack.push_back(file);
    }
    ~FileStackGuard() {
      if (pushed) stack.pop_back();
    }
  } guard(fileStack_, fn->definedInFile);

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

  if (obj.isList()) {
    auto &elems = obj.asList()->elements;
    int64_t i = key.asInt();
    if (i < 0)
      i += elems.size();
    if (i < 0 || (size_t)i >= elems.size())
      throw IndexError("List index " + std::to_string(key.asInt()) +
                             " out of bounds",
                         e.line);
    return elems[i];
  }
  if (obj.isDict()) {
    auto v = obj.asDict()->get(key.toString());
    return !v.isNull() ? v : makeNull();
  }
  if (obj.isStr()) {
    int64_t i = key.asInt();
    const std::string &s = obj.asStr();
    if (i < 0)
      i += s.size();
    if (i < 0 || (size_t)i >= s.size())
      throw IndexError("String index out of bounds", e.line);
    return makeStr(std::string(1, s[i]));
  }
  throw TypeError("Cannot index '" + obj.typeName() + "'", e.line);
}

ValuePtr Interpreter::evalMember(const MemberExpr &e) {
  auto obj = evalExpr(*e.object);

  // Safe chaining: if null and safe, return null
  if (obj.isNull() && e.safe)
    return makeNull();

  if (e.op == "->") {
    if (!obj.isPtr()) {
      throw TypeError("Cannot use '->' on non-pointer object of type '" + obj.typeName() + "'. Use '.' instead.", e.line);
    }
    auto target = obj.asPtr()->target;
    if (!target || (*target).isNull()) {
      throw RuntimeError("Null pointer dereference", e.line, "E_RUNTIME_403");
    }
    obj = *target;
  } else if (e.op == "." || e.op == "?.") {
    if (obj.isPtr()) {
      throw TypeError("Cannot use '" + e.op + "' on pointer object. Use '->' instead.", e.line);
    }
    if (obj.isClassDef()) {
      throw TypeError("Cannot use '" + e.op + "' on Class. Use '::' instead.", e.line);
    }
  } else if (e.op == "::") {
    if (obj.isClassInst() || obj.isStructInst()) {
      throw TypeError("Cannot use '::' on instance of '" + obj.typeName() + "'. Use '.' instead.", e.line);
    }
    if (obj.isPtr()) {
      throw TypeError("Cannot use '::' on pointer object. Use '->' instead.", e.line);
    }
  }

  if (obj.isClassInst()) {
    auto inst = obj.asClassInst();
    if (inst->members->hasLocal(e.member) || inst->members->has(e.member))
      return inst->members->get(e.member, e.line);
    // Return bound method
    throw RuntimeError(
        "Member '" + e.member + "' not found on " + inst->def->name, e.line);
  }

  if (obj.isClassDef()) {
    auto def = std::get<std::shared_ptr<ClassDef>>(obj.data);
    if (def->methods->has(e.member))
      return def->methods->get(e.member, e.line);
    throw RuntimeError(
        "Static member '" + e.member + "' not found on class " + def->name, e.line);
  }

  if (obj.isStructInst()) {
    auto inst = obj.asStructInst();
    auto it = inst->fields.find(e.member);
    if (it != inst->fields.end())
      return it->second;
    throw RuntimeError("Field '" + e.member + "' not found on struct " +
                           inst->def->name,
                       e.line);
  }

  if (obj.isDict()) {
    auto v = obj.asDict()->get(e.member);
    return !v.isNull() ? v : makeNull();
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

ValuePtr Interpreter::evalDelete(const DeleteExpr &e) {
  ValuePtr val = evalExpr(*e.operand);
  ValuePtr targetVal = val;
  ValuePtr* targetSlot = nullptr;

  if (val->isPtr()) {
    targetSlot = val->asPtr()->target;
    if (targetSlot) {
      targetVal = *targetSlot;
    }
  }

  if (!targetVal.isNull() && targetVal.isClassInst()) {
    auto inst = targetVal->asClassInst();
    std::unordered_set<std::string> executed;
    // Run ordered destructors in def->dtorOrder
    for (auto &[n, method] : inst->def->dtorOrder) {
      if (inst->members->has(method)) {
        auto mv = inst->members->get(method);
        if (mv.isFn()) {
          std::vector<ValuePtr> emptyArgs;
          callMethod(inst, method, emptyArgs, e.line);
          executed.insert(method);
        }
      }
    }
    // Also check for general "cleanup" or "destroy" or "destructor" methods if not already run
    for (const std::string method : {"cleanup", "destroy", "destructor"}) {
      if (executed.count(method) == 0 && inst->members->has(method)) {
        auto mv = inst->members->get(method);
        if (mv.isFn()) {
          std::vector<ValuePtr> emptyArgs;
          callMethod(inst, method, emptyArgs, e.line);
        }
      }
    }
  }

  // Set the variable slot to null to reclaim/release memory
  if (targetSlot) {
    *targetSlot = makeNull();
  } else {
    if (auto* ident = std::get_if<IdentExpr>(&e.operand->data)) {
      currentEnv_->set(ident->name, makeNull(), e.line);
    }
  }

  return makeNull();
}

// --- Built-in methods
// ---------------------------------------------------------

ValuePtr Interpreter::callBuiltinMethod(ValuePtr obj, const std::string &method,
                                        std::vector<ValuePtr> args, int line) {
  trace("Call: builtin method '" + method + "' on " + obj->typeName());
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
        if (e.equals(args[0]))
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
        lv->elements.push_back(makeStr(k));
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
      return makeBool(!dv->get(args[0]->toString()).isNull());
    }
    if (method == "remove") {
      if (!args.empty()) {
        auto key = args[0]->toString();
        dv->pairs.erase(key);
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

// --- Built-in functions
// -------------------------------------------------------

void Interpreter::registerBuiltins() {
  auto builtins = std::make_shared<DictValue>();

  auto defNative = [&](const std::string &name,
                       std::function<ValuePtr(std::vector<ValuePtr>)> fn) {
    auto fv = std::make_shared<FunctionValue>();
    fv->name = name;
    fv->isNative = true;
    fv->native = std::move(fn);
    builtins->set(name, makeFn(fv));
  };

  // input
  defNative("input", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (!args.empty())
      print(args[0]->toString());
    return makeStr(readLine());
  });

  // time delays
  defNative("delay", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (!args.empty()) {
      double ms = args[0]->isFloat() ? args[0]->asFloat() : (double)args[0]->asInt();
      std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(ms));
    }
    return makeNull();
  });
  defNative("delayMilliseconds", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (!args.empty()) {
      double ms = args[0]->isFloat() ? args[0]->asFloat() : (double)args[0]->asInt();
      std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(ms));
    }
    return makeNull();
  });
  defNative("delayMicroseconds", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (!args.empty()) {
      double us = args[0]->isFloat() ? args[0]->asFloat() : (double)args[0]->asInt();
      std::this_thread::sleep_for(std::chrono::duration<double, std::micro>(us));
    }
    return makeNull();
  });
  defNative("now", [](std::vector<ValuePtr> args) -> ValuePtr {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    return makeInt((int64_t)micros);
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
  defNative("toStr", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty())
      return makeStr("");
    return makeStr(args[0].toString());
  });
  defNative("toInt", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty())
      return makeInt(0);
    auto &v = args[0];
    if (v.isInt())
      return v;
    if (v.isFloat())
      return makeInt((int64_t)v.asFloat());
    if (v.isStr()) {
      try {
        return makeInt(std::stoll(v.asStr()));
      } catch (...) {
      }
    }
    return makeInt(0);
  });
  defNative("toFloat", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty())
      return makeFloat(0.0);
    auto &v = args[0];
    if (v.isFloat())
      return v;
    if (v.isInt())
      return makeFloat((double)v.asInt());
    if (v.isStr()) {
      try {
        return makeFloat(std::stod(v.asStr()));
      } catch (...) {
      }
    }
    return makeFloat(0.0);
  });
  defNative("toBool", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty())
      return makeBool(false);
    return makeBool(args[0].truthy());
  });

  // len
  defNative("len", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty())
      return makeInt(0);
    auto &v = args[0];
    if (v.isStr())
      return makeInt(v.asStr().size());
    if (v.isList())
      return makeInt(v.asList()->elements.size());
    if (v.isDict())
      return makeInt(v.asDict()->pairs.size());
    if (v.isSet())
      return makeInt(v.asSet()->elements.size());
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

  // Complex constructors & utils
  defNative("complex", [](std::vector<ValuePtr> a) -> ValuePtr {
    double real = a.size() > 0 ? a[0].asFloat() : 0.0;
    double imag = a.size() > 1 ? a[1].asFloat() : 0.0;
    return makeComplex(std::complex<double>(real, imag));
  });
  defNative("real", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty()) return makeFloat(0.0);
    if (a[0].isComplex()) return makeFloat(a[0].asComplex().real());
    return makeFloat(a[0].asFloat());
  });
  defNative("imag", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty()) return makeFloat(0.0);
    if (a[0].isComplex()) return makeFloat(a[0].asComplex().imag());
    return makeFloat(0.0);
  });

  // Math functions
  defNative("abs", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty()) return makeInt(0);
    if (a[0].isComplex()) return makeFloat(std::abs(a[0].asComplex()));
    if (a[0].isFloat()) return makeFloat(std::abs(a[0].asFloat()));
    return makeInt(std::abs(a[0].asInt()));
  });
  defNative("sqrt", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty()) return makeFloat(0.0);
    if (a[0].isComplex()) return makeComplex(std::sqrt(a[0].asComplex()));
    double val = a[0].asFloat();
    if (val < 0) return makeComplex(std::sqrt(std::complex<double>(val, 0)));
    return makeFloat(std::sqrt(val));
  });
  defNative("pow", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.size() < 2) return makeFloat(1.0);
    if (a[0].isComplex() || a[1].isComplex()) return makeComplex(std::pow(a[0].asComplex(), a[1].asComplex()));
    return makeFloat(std::pow(a[0].asFloat(), a[1].asFloat()));
  });
  defNative("floor", [](std::vector<ValuePtr> a) -> ValuePtr {
    return makeInt((int64_t)std::floor(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("ceil", [](std::vector<ValuePtr> a) -> ValuePtr {
    return makeInt((int64_t)std::ceil(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("round", [](std::vector<ValuePtr> a) -> ValuePtr {
    return makeInt((int64_t)std::round(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("max", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty())
      return makeNull();
    ValuePtr m = a[0];
    for (auto &v : a)
      if (v.asFloat() > m.asFloat())
        m = v;
    return m;
  });
  defNative("min", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty())
      return makeNull();
    ValuePtr m = a[0];
    for (auto &v : a)
      if (v.asFloat() < m.asFloat())
        m = v;
    return m;
  });
  defNative("sin", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty()) return makeFloat(0.0);
    if (a[0].isComplex()) return makeComplex(std::sin(a[0].asComplex()));
    return makeFloat(std::sin(a[0].asFloat()));
  });
  defNative("cos", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty()) return makeFloat(0.0);
    if (a[0].isComplex()) return makeComplex(std::cos(a[0].asComplex()));
    return makeFloat(std::cos(a[0].asFloat()));
  });
  defNative("tan", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty()) return makeFloat(0.0);
    if (a[0].isComplex()) return makeComplex(std::tan(a[0].asComplex()));
    return makeFloat(std::tan(a[0].asFloat()));
  });
  defNative("log", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty()) return makeFloat(1.0);
    if (a[0].isComplex() || a[0].asFloat() < 0) return makeComplex(std::log(a[0].asComplex()));
    return makeFloat(std::log(a[0].asFloat()));
  });
  defNative("log2", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty()) return makeFloat(1.0);
    if (a[0].isComplex() || a[0].asFloat() < 0) {
        std::complex<double> z = a[0].asComplex();
        return makeComplex(std::log(z) / std::log(2.0));
    }
    return makeFloat(std::log2(a[0].asFloat()));
  });
  defNative("log10", [](std::vector<ValuePtr> a) -> ValuePtr {
    if (a.empty()) return makeFloat(1.0);
    if (a[0].isComplex() || a[0].asFloat() < 0) return makeComplex(std::log10(a[0].asComplex()));
    return makeFloat(std::log10(a[0].asFloat()));
  });

  // Inverse trig
  defNative("asin", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::asin(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("acos", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::acos(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("atan", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::atan(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("atan2", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::atan2(a.size() < 1 ? 0 : a[0].asFloat(), a.size() < 2 ? 0 : a[1].asFloat()));
  });

  // Matrix functions
  defNative("matrix_add", [this](std::vector<ValuePtr> a) -> ValuePtr {
      if (a.size() < 2) throw MathError("matrix_add requires 2 arguments");
      auto m1 = a[0];
      auto m2 = a[1];
      if (!m1.isList() || !m2.isList()) throw MathError("matrix_add arguments must be matrices");
      auto& lst1 = m1.asList()->elements;
      auto& lst2 = m2.asList()->elements;
      if (lst1.size() != lst2.size()) throw MathError("Matrix row dimensions mismatch");
      auto res = std::make_shared<ListValue>();
      for (size_t i = 0; i < lst1.size(); i++) {
          if (!lst1[i].isList() || !lst2[i].isList()) throw MathError("Matrix row must be a list");
          auto& row1 = lst1[i].asList()->elements;
          auto& row2 = lst2[i].asList()->elements;
          if (row1.size() != row2.size()) throw MathError("Matrix column dimensions mismatch");
          auto newRow = std::make_shared<ListValue>();
          for (size_t j = 0; j < row1.size(); j++) {
              newRow->elements.push_back(this->applyBinaryArith("+", row1[j], row2[j], 0));
          }
          res->elements.push_back(makeList(newRow));
      }
      return makeList(res);
  });

  defNative("matrix_mul", [this](std::vector<ValuePtr> a) -> ValuePtr {
      if (a.size() < 2) throw MathError("matrix_mul requires 2 arguments");
      auto m1 = a[0];
      auto m2 = a[1];
      if (!m1.isList() || !m2.isList()) throw MathError("matrix_mul arguments must be matrices");
      auto& lst1 = m1.asList()->elements;
      auto& lst2 = m2.asList()->elements;
      size_t rows1 = lst1.size();
      size_t cols1 = rows1 > 0 ? (lst1[0].isList() ? lst1[0].asList()->elements.size() : 0) : 0;
      size_t rows2 = lst2.size();
      size_t cols2 = rows2 > 0 ? (lst2[0].isList() ? lst2[0].asList()->elements.size() : 0) : 0;
      if (cols1 != rows2) throw MathError("Matrix inner dimensions must agree");
      auto res = std::make_shared<ListValue>();
      for (size_t i = 0; i < rows1; i++) {
          auto newRow = std::make_shared<ListValue>();
          for (size_t j = 0; j < cols2; j++) {
              ValuePtr sum = makeFloat(0.0);
              for (size_t k = 0; k < cols1; k++) {
                  ValuePtr v1 = lst1[i].asList()->elements[k];
                  ValuePtr v2 = lst2[k].asList()->elements[j];
                  ValuePtr prod = this->applyBinaryArith("*", v1, v2, 0);
                  sum = this->applyBinaryArith("+", sum, prod, 0);
              }
              newRow->elements.push_back(sum);
          }
          res->elements.push_back(makeList(newRow));
      }
      return makeList(res);
  });

  defNative("matrix_transpose", [](std::vector<ValuePtr> a) -> ValuePtr {
      if (a.empty() || !a[0].isList()) throw MathError("matrix_transpose requires a matrix");
      auto& lst = a[0].asList()->elements;
      size_t rows = lst.size();
      size_t cols = rows > 0 ? (lst[0].isList() ? lst[0].asList()->elements.size() : 0) : 0;
      auto res = std::make_shared<ListValue>();
      for (size_t j = 0; j < cols; j++) {
          auto newRow = std::make_shared<ListValue>();
          for (size_t i = 0; i < rows; i++) {
              newRow->elements.push_back(lst[i].asList()->elements[j]);
          }
          res->elements.push_back(makeList(newRow));
      }
      return makeList(res);
  });

  defNative("matrix_scale", [this](std::vector<ValuePtr> a) -> ValuePtr {
      if (a.size() < 2) throw MathError("matrix_scale requires matrix and scalar");
      auto m1 = a[0];
      auto scalar = a[1];
      if (!m1.isList()) throw MathError("matrix_scale first arg must be matrix");
      auto& lst1 = m1.asList()->elements;
      auto res = std::make_shared<ListValue>();
      for (size_t i = 0; i < lst1.size(); i++) {
          auto newRow = std::make_shared<ListValue>();
          if (!lst1[i].isList()) throw MathError("Matrix row must be a list");
          auto& row1 = lst1[i].asList()->elements;
          for (size_t j = 0; j < row1.size(); j++) {
              newRow->elements.push_back(this->applyBinaryArith("*", row1[j], scalar, 0));
          }
          res->elements.push_back(makeList(newRow));
      }
      return makeList(res);
  });

  defNative("matrix_eye", [](std::vector<ValuePtr> a) -> ValuePtr {
      if (a.empty()) throw MathError("matrix_eye requires size");
      int n = a[0].asInt();
      auto res = std::make_shared<ListValue>();
      for (int i = 0; i < n; i++) {
          auto row = std::make_shared<ListValue>();
          for (int j = 0; j < n; j++) {
              row->elements.push_back(i == j ? makeFloat(1.0) : makeFloat(0.0));
          }
          res->elements.push_back(makeList(row));
      }
      return makeList(res);
  });

  // Hyperbolic
  defNative("sinh", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::sinh(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("cosh", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::cosh(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("tanh", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::tanh(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("asinh", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::asinh(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("acosh", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::acosh(a.empty() ? 1 : a[0].asFloat()));
  });
  defNative("atanh", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::atanh(a.empty() ? 0 : a[0].asFloat()));
  });

  // Exponential/Power
  defNative("exp", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::exp(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("cbrt", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::cbrt(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("hypot", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::hypot(a.size() < 1 ? 0 : a[0].asFloat(), a.size() < 2 ? 0 : a[1].asFloat()));
  });

  // Special functions
  defNative("erf", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::erf(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("erfc", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::erfc(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("tgamma", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::tgamma(a.empty() ? 0 : a[0].asFloat()));
  });
  defNative("lgamma", [](std::vector<ValuePtr> a) -> ValuePtr {
      return makeFloat(std::lgamma(a.empty() ? 0 : a[0].asFloat()));
  });

  // Complex functions
  defNative("complex", [](std::vector<ValuePtr> a) -> ValuePtr {
      double r = a.empty() ? 0 : a[0].asFloat();
      double i = a.size() < 2 ? 0 : a[1].asFloat();
      return makeComplex(std::complex<double>(r, i));
  });
  defNative("real", [](std::vector<ValuePtr> a) -> ValuePtr {
      if (a.empty()) return makeFloat(0);
      if (a[0].isComplex()) return makeFloat(a[0].asComplex().real());
      return makeFloat(a[0].asFloat());
  });
  defNative("imag", [](std::vector<ValuePtr> a) -> ValuePtr {
      if (a.empty()) return makeFloat(0);
      if (a[0].isComplex()) return makeFloat(a[0].asComplex().imag());
      return makeFloat(0);
  });
  defNative("conj", [](std::vector<ValuePtr> a) -> ValuePtr {
      if (a.empty()) return makeNull();
      if (a[0].isComplex()) return makeComplex(std::conj(a[0].asComplex()));
      return a[0];
  });
  defNative("arg", [](std::vector<ValuePtr> a) -> ValuePtr {
      if (a.empty()) return makeFloat(0);
      return makeFloat(std::arg(a[0].asComplex()));
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

  // Global constants in builtins
  builtins->set("PI", makeFloat(3.14159265358979323846));
  builtins->set("E", makeFloat(2.71828182845904523536));
  builtins->set("TAU", makeFloat(6.28318530717958647692));
  builtins->set("PHI", makeFloat(1.61803398874989484820));
  builtins->set("INF", makeFloat(std::numeric_limits<double>::infinity()));
  builtins->set("NEG_INF", makeFloat(-std::numeric_limits<double>::infinity()));
  builtins->set("NAN", makeFloat(std::numeric_limits<double>::quiet_NaN()));

  // Scientific Constants
  builtins->set("SC_C", makeFloat(299792458.0));        // Speed of light (m/s)
  builtins->set("SC_G", makeFloat(6.67430e-11));      // Gravitational constant (m^3/kg/s^2)
  builtins->set("SC_H", makeFloat(6.62607015e-34));   // Planck constant (J*s)
  builtins->set("SC_K", makeFloat(1.380649e-23));     // Boltzmann constant (J/K)
  builtins->set("SC_NA", makeFloat(6.02214076e23));    // Avogadro number (1/mol)
  builtins->set("SC_R", makeFloat(8.314462618));      // Gas constant (J/mol/K)

  builtins->set("SECOND", makeInt(1));
  builtins->set("MINUTE", makeInt(60));
  builtins->set("HOUR", makeInt(3600));
  builtins->set("DAY", makeInt(86400));
  builtins->set("WEEK", makeInt(604800));

  builtins->set("RUNTIME_VERSION", makeStr(__RUNTIME_VERSION__));
  builtins->set("SULFUR_VERSION", makeStr(__SULFUR_VERSION__));
  builtins->set("FUSE_VERSION", makeStr(__FUSE_VERSION__));
  builtins->set("COMBUST_VERSION", makeStr(__COMBUST_VERSION__));
  builtins->set("BUILD_MODE", makeStr("debug"));
  builtins->set("DEBUG_MODE", makeBool(false));

  // Property constants (runtime no-ops)
  for (auto &p : {"PROP_FROZEN",   "PROP_READONLY",     "PROP_IMMUTABLE",
                  "PROP_REACTIVE", "PROP_SERIALIZABLE", "PROP_UNSAFE",
                  "PROP_VOLATILE", "PROP_DEBUG",        "PROP_HIDDEN",
                  "PROP_PRIVATE",  "PROP_PUBLIC",       "PROP_FINAL",
                  "PROP_CONST",    "PROP_STATIC",       "PROP_SYNC",
                  "PROP_ASYNC",    "PROP_LAZY",         "PROP_CACHED",
                  "PROP_TEMP",     "PROP_NATIVE",       "PROP_PROTECTED",
                  "PROP_INTERNAL", "PROP_EXPERIMENTAL", "PROP_DEPRECATED",
                  "PROP_LOCKED",   "PROP_OBSERVABLE",
                  "OBJ_NAME",      "GET_ITEM",          "PROPS"}) {
    builtins->set(p, makeStr(p));
  }

  // Terminal namespace
  auto terminal = std::make_shared<DictValue>();
  // Terminal.Out, Terminal.Warn, Terminal.Err are stream markers

  // We use a trick: Terminal is a dict with out/warn/err as special sentinel strings
  terminal->set("out", makeStr("<Terminal.Out>"));
  terminal->set("Out", makeStr("<Terminal.Out>"));
  terminal->set("warn", makeStr("<Terminal.Warn>"));
  terminal->set("Warn", makeStr("<Terminal.Warn>"));
  terminal->set("err", makeStr("<Terminal.Err>"));
  terminal->set("Err", makeStr("<Terminal.Err>"));
  terminal->set("Return", makeStr("<Terminal.Return>"));
  terminal->set("return", makeStr("<Terminal.Return>"));
  terminal->set("EOL", makeStr("\n"));
  terminal->set("eol", makeStr("\n"));

  // Terminal.in is a special object
  auto terminalIn = std::make_shared<DictValue>();
  terminalIn->set("__stream__", makeStr("<Terminal.In>"));
  // Terminal.in.read() - handled via callBuiltinMethod
  terminal->set("in", makeDict(terminalIn));
  terminal->set("In", makeDict(terminalIn));

  builtins->set("Terminal", makeDict(terminal));
  builtins->set("stdout", makeStr("<Terminal.Out>"));
  builtins->set("stderr", makeStr("<Terminal.Err>"));
  builtins->set("stdin", makeStr("<Terminal.In>"));
  builtins->set("stdwarn", makeStr("<Terminal.Warn>"));

  // TIO namespace
  auto tio = std::make_shared<DictValue>();

  // TIO.E - error
  auto tioE = std::make_shared<FunctionValue>();
  tioE->name = "TIO.E";
  tioE->isNative = true;
  tioE->native = [](std::vector<ValuePtr> args) -> ValuePtr {
      std::string msg = args.empty() ? "Error occurred" : args[0]->toString();
      auto errObj = std::make_shared<DictValue>();
      errObj->set("__type__", makeStr("TIO_Error"));
      errObj->set("severity", makeStr("E"));
      errObj->set("message", makeStr(msg));
      return makeDict(errObj);
  };
  tio->set("E", makeFn(tioE));

  // TIO.FE - fatal error
  auto tioFE = std::make_shared<FunctionValue>();
  tioFE->name = "TIO.FE";
  tioFE->isNative = true;
  tioFE->native = [](std::vector<ValuePtr> args) -> ValuePtr {
      std::string msg = args.empty() ? "Fatal error occurred" : args[0]->toString();
      auto errObj = std::make_shared<DictValue>();
      errObj->set("__type__", makeStr("TIO_Error"));
      errObj->set("severity", makeStr("FE"));
      errObj->set("message", makeStr(msg));
      return makeDict(errObj);
  };
  tio->set("FE", makeFn(tioFE));

  // TIO.W - warning
  auto tioW = std::make_shared<FunctionValue>();
  tioW->name = "TIO.W";
  tioW->isNative = true;
  tioW->native = [](std::vector<ValuePtr> args) -> ValuePtr {
      std::string msg = args.empty() ? "Warning occurred" : args[0]->toString();
      auto errObj = std::make_shared<DictValue>();
      errObj->set("__type__", makeStr("TIO_Error"));
      errObj->set("severity", makeStr("W"));
      errObj->set("message", makeStr(msg));
      return makeDict(errObj);
  };
  tio->set("W", makeFn(tioW));

  // TIO.withCategory - attaches a category string to an error/warning dict
  auto tioWithCategory = std::make_shared<FunctionValue>();
  tioWithCategory->name = "TIO.withCategory";
  tioWithCategory->isNative = true;
  tioWithCategory->native = [](std::vector<ValuePtr> args) -> ValuePtr {
      if (args.size() < 2)
          throw std::runtime_error("TIO.withCategory expects 2 arguments: (errorDict, category)");
      if (!args[0]->isDict())
          throw std::runtime_error("TIO.withCategory: first argument must be a TIO error dict");
      args[0]->asDict()->set("category", makeStr(args[1]->toString()));
      return args[0];
  };
  tio->set("withCategory", makeFn(tioWithCategory));

  // TIO.withCode - attaches an error/warning code string to an error/warning dict
  auto tioWithCode = std::make_shared<FunctionValue>();
  tioWithCode->name = "TIO.withCode";
  tioWithCode->isNative = true;
  tioWithCode->native = [](std::vector<ValuePtr> args) -> ValuePtr {
      if (args.size() < 2)
          throw std::runtime_error("TIO.withCode expects 2 arguments: (errorDict, code)");
      if (!args[0]->isDict())
          throw std::runtime_error("TIO.withCode: first argument must be a TIO error dict");
      args[0]->asDict()->set("code", makeStr(args[1]->toString()));
      return args[0];
  };
  tio->set("withCode", makeFn(tioWithCode));

  // TIO.withHint - attaches a hint string to an error/warning dict
  auto tioWithHint = std::make_shared<FunctionValue>();
  tioWithHint->name = "TIO.withHint";
  tioWithHint->isNative = true;
  tioWithHint->native = [](std::vector<ValuePtr> args) -> ValuePtr {
      if (args.size() < 2)
          throw std::runtime_error("TIO.withHint expects 2 arguments: (errorDict, hint)");
      if (!args[0]->isDict())
          throw std::runtime_error("TIO.withHint: first argument must be a TIO error dict");
      args[0]->asDict()->set("hint", makeStr(args[1]->toString()));
      return args[0];
  };
  tio->set("withHint", makeFn(tioWithHint));

  // TIO.withContext - attaches arbitrary context data to an error/warning dict
  auto tioWithContext = std::make_shared<FunctionValue>();
  tioWithContext->name = "TIO.withContext";
  tioWithContext->isNative = true;
  tioWithContext->native = [](std::vector<ValuePtr> args) -> ValuePtr {
      if (args.size() < 2)
          throw std::runtime_error("TIO.withContext expects 2 arguments: (errorDict, contextValue)");
      if (!args[0]->isDict())
          throw std::runtime_error("TIO.withContext: first argument must be a TIO error dict");
      args[0]->asDict()->set("context", args[1]);
      return args[0];
  };
  tio->set("withContext", makeFn(tioWithContext));

  builtins->set("TIO", makeDict(tio));

  // Terminal.In.read()
  defNative("read", [this](std::vector<ValuePtr>) -> ValuePtr {
    return makeStr(readLine());
  });

  // get function
  defNative("get", [this](std::vector<ValuePtr> args) -> ValuePtr {
      if (args.empty()) {
          throw TypeError("get() expects at least 1 argument");
      }
      ValuePtr obj = args[0];
      
      auto getSingleProp = [this](ValuePtr target, const std::string& prop) -> ValuePtr {
          if (prop == "OBJ_NAME") {
              if (target->isClassInst()) {
                  return makeStr(target->asClassInst()->def->name);
              } else if (target->isClassDef()) {
                  return makeStr(std::get<std::shared_ptr<ClassDef>>(target->data)->name);
              } else if (target->isStructInst()) {
                  return makeStr(target->asStructInst()->def->name);
              } else if (target->isStructDef()) {
                  return makeStr(std::get<std::shared_ptr<StructDef>>(target->data)->name);
              } else if (target->isFn()) {
                  return makeStr(target->asFn()->name);
              } else {
                  return makeStr(target->typeName());
              }
          } else if (prop == "GET_ITEM") {
              if (target->isList()) {
                  auto lv = std::make_shared<ListValue>();
                  lv->elements = target->asList()->elements;
                  return makeList(lv);
              } else if (target->isDict()) {
                  auto lv = std::make_shared<ListValue>();
                  for (auto& kv : target->asDict()->pairs) {
                      auto pair = std::make_shared<ListValue>();
                      pair->elements.push_back(makeStr(kv.first));
                      pair->elements.push_back(kv.second);
                      lv->elements.push_back(makeList(pair));
                  }
                  return makeList(lv);
              } else if (target->isStr()) {
                  auto lv = std::make_shared<ListValue>();
                  for (char c : target->asStr()) {
                      lv->elements.push_back(makeStr(std::string(1, c)));
                  }
                  return makeList(lv);
              } else if (target->isSet()) {
                  auto lv = std::make_shared<ListValue>();
                  for (auto& elem : target->asSet()->elements) {
                      lv->elements.push_back(elem);
                  }
                  return makeList(lv);
              } else if (target->isClassInst()) {
                  auto lv = std::make_shared<ListValue>();
                  auto inst = target->asClassInst();
                  for (auto& kv : inst->members->vars()) {
                      auto pair = std::make_shared<ListValue>();
                      pair->elements.push_back(makeStr(kv.first));
                      pair->elements.push_back(kv.second.value);
                      lv->elements.push_back(makeList(pair));
                  }
                  return makeList(lv);
              } else if (target->isStructInst()) {
                  auto lv = std::make_shared<ListValue>();
                  auto inst = target->asStructInst();
                  for (auto& kv : inst->fields) {
                      auto pair = std::make_shared<ListValue>();
                      pair->elements.push_back(makeStr(kv.first));
                      pair->elements.push_back(kv.second);
                      lv->elements.push_back(makeList(pair));
                  }
                  return makeList(lv);
              } else {
                  return makeList(std::make_shared<ListValue>());
              }
          } else if (prop == "PROPS") {
              auto lv = std::make_shared<ListValue>();
              if (target->isClassInst()) {
                  auto inst = target->asClassInst();
                  for (auto& kv : inst->members->vars()) {
                      lv->elements.push_back(makeStr(kv.first));
                  }
              } else if (target->isClassDef()) {
                  auto def = std::get<std::shared_ptr<ClassDef>>(target->data);
                  for (auto& kv : def->methods->vars()) {
                      lv->elements.push_back(makeStr(kv.first));
                  }
              } else if (target->isStructInst()) {
                  auto inst = target->asStructInst();
                  for (auto& kv : inst->fields) {
                      lv->elements.push_back(makeStr(kv.first));
                  }
              } else if (target->isStructDef()) {
                  auto def = std::get<std::shared_ptr<StructDef>>(target->data);
                  for (auto& f : def->fields) {
                      lv->elements.push_back(makeStr(f.first));
                  }
              } else if (target->isDict()) {
                  for (auto& kv : target->asDict()->pairs) {
                      lv->elements.push_back(makeStr(kv.first));
                  }
              } else if (target->isStr()) {
                  for (auto& m : {"length", "len", "upper", "lower", "trim", "contains", "startsWith", "endsWith", "split", "replace", "slice", "substr", "toString", "str", "toInt", "toFloat", "repeat", "chars"}) {
                      lv->elements.push_back(makeStr(m));
                  }
              } else if (target->isList()) {
                  for (auto& m : {"length", "len", "size", "push", "add", "append", "pop", "shift", "unshift", "contains", "includes", "reverse", "join", "slice", "map", "filter", "reduce", "first", "last", "clear", "toString"}) {
                      lv->elements.push_back(makeStr(m));
                  }
              }
              return makeList(lv);
          }
          return makeNull();
      };
      
      if (args.size() == 1) {
          auto resDict = std::make_shared<DictValue>();
          resDict->set("OBJ_NAME", getSingleProp(obj, "OBJ_NAME"));
          resDict->set("GET_ITEM", getSingleProp(obj, "GET_ITEM"));
          resDict->set("PROPS", getSingleProp(obj, "PROPS"));
          return makeDict(resDict);
      }
      
      ValuePtr spec = args[1];
      if (spec->isList()) {
          auto resDict = std::make_shared<DictValue>();
          for (auto& elem : spec->asList()->elements) {
              std::string prop = elem->toString();
              resDict->set(prop, getSingleProp(obj, prop));
          }
          return makeDict(resDict);
      } else {
          return getSingleProp(obj, spec->toString());
      }
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

  // Matrix functions
  auto parseMatrix = [](ValuePtr val) -> std::vector<std::vector<double>> {
    if (!val->isList()) {
      throw TypeError("Expected a 2D list (matrix), but got " + val->typeName());
    }
    auto listVal = val->asList();
    if (listVal->elements.empty()) {
      throw TypeError("Matrix cannot be empty");
    }
    std::vector<std::vector<double>> matrix;
    size_t cols = 0;
    bool first = true;
    for (auto& rowVal : listVal->elements) {
      if (!rowVal->isList()) {
        throw TypeError("Expected each row of the matrix to be a list, but got " + rowVal->typeName());
      }
      auto rowList = rowVal->asList();
      if (first) {
        cols = rowList->elements.size();
        if (cols == 0) {
          throw TypeError("Matrix rows cannot be empty");
        }
        first = false;
      } else if (rowList->elements.size() != cols) {
        throw TypeError("Matrix rows must have the same number of columns");
      }
      std::vector<double> row;
      for (auto& elem : rowList->elements) {
        if (elem->isInt()) {
          row.push_back((double)elem->asInt());
        } else if (elem->isFloat()) {
          row.push_back(elem->asFloat());
        } else {
          throw TypeError("Matrix elements must be numbers, but got " + elem->typeName());
        }
      }
      matrix.push_back(row);
    }
    return matrix;
  };

  auto isMatrixAllInts = [](ValuePtr val) -> bool {
    if (!val->isList()) return false;
    for (auto& rowVal : val->asList()->elements) {
      if (!rowVal->isList()) return false;
      for (auto& elem : rowVal->asList()->elements) {
        if (!elem->isInt()) return false;
      }
    }
    return true;
  };

  auto makeMatrix = [](const std::vector<std::vector<double>>& matrix, bool allInts) -> ValuePtr {
    auto outer = std::make_shared<ListValue>();
    for (const auto& row : matrix) {
      auto inner = std::make_shared<ListValue>();
      for (double val : row) {
        if (allInts) {
          inner->elements.push_back(makeInt((int64_t)val));
        } else {
          inner->elements.push_back(makeFloat(val));
        }
      }
      outer->elements.push_back(makeList(inner));
    }
    return makeList(outer);
  };

  defNative("matrix_add", [parseMatrix, isMatrixAllInts, makeMatrix](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() != 2) {
      throw TypeError("matrix_add expects exactly 2 arguments");
    }
    auto m1 = parseMatrix(args[0]);
    auto m2 = parseMatrix(args[1]);
    if (m1.size() != m2.size() || m1[0].size() != m2[0].size()) {
      throw MathError("Matrix dimensions do not match for addition: (" + 
                      std::to_string(m1.size()) + "x" + std::to_string(m1[0].size()) + ") and (" +
                      std::to_string(m2.size()) + "x" + std::to_string(m2[0].size()) + ")");
    }
    size_t rows = m1.size();
    size_t cols = m1[0].size();
    std::vector<std::vector<double>> res(rows, std::vector<double>(cols));
    for (size_t i = 0; i < rows; ++i) {
      for (size_t j = 0; j < cols; ++j) {
        res[i][j] = m1[i][j] + m2[i][j];
      }
    }
    bool allInts = isMatrixAllInts(args[0]) && isMatrixAllInts(args[1]);
    return makeMatrix(res, allInts);
  });

  defNative("matrix_mul", [parseMatrix, isMatrixAllInts, makeMatrix](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() != 2) {
      throw TypeError("matrix_mul expects exactly 2 arguments");
    }
    bool firstIsScalar = args[0]->isInt() || args[0]->isFloat();
    bool secondIsScalar = args[1]->isInt() || args[1]->isFloat();
    if (firstIsScalar && secondIsScalar) {
      throw TypeError("matrix_mul expects at least one matrix argument");
    }
    if (firstIsScalar) {
      double scalar = args[0]->isInt() ? (double)args[0]->asInt() : args[0]->asFloat();
      auto m = parseMatrix(args[1]);
      for (size_t i = 0; i < m.size(); ++i) {
        for (size_t j = 0; j < m[0].size(); ++j) {
          m[i][j] *= scalar;
        }
      }
      bool allInts = args[0]->isInt() && isMatrixAllInts(args[1]);
      return makeMatrix(m, allInts);
    }
    if (secondIsScalar) {
      double scalar = args[1]->isInt() ? (double)args[1]->asInt() : args[1]->asFloat();
      auto m = parseMatrix(args[0]);
      for (size_t i = 0; i < m.size(); ++i) {
        for (size_t j = 0; j < m[0].size(); ++j) {
          m[i][j] *= scalar;
        }
      }
      bool allInts = isMatrixAllInts(args[0]) && args[1]->isInt();
      return makeMatrix(m, allInts);
    }
    auto m1 = parseMatrix(args[0]);
    auto m2 = parseMatrix(args[1]);
    if (m1[0].size() != m2.size()) {
      throw MathError("Matrix dimensions do not match for multiplication: columns of first (" +
                      std::to_string(m1[0].size()) + ") must match rows of second (" +
                      std::to_string(m2.size()) + ")");
    }
    size_t rows = m1.size();
    size_t cols = m2[0].size();
    size_t innerDim = m1[0].size();
    std::vector<std::vector<double>> res(rows, std::vector<double>(cols, 0.0));
    for (size_t i = 0; i < rows; ++i) {
      for (size_t j = 0; j < cols; ++j) {
        for (size_t k = 0; k < innerDim; ++k) {
          res[i][j] += m1[i][k] * m2[k][j];
        }
      }
    }
    bool allInts = isMatrixAllInts(args[0]) && isMatrixAllInts(args[1]);
    return makeMatrix(res, allInts);
  });

  defNative("matrix_transpose", [parseMatrix, isMatrixAllInts, makeMatrix](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) {
      throw TypeError("matrix_transpose expects exactly 1 argument");
    }
    auto m = parseMatrix(args[0]);
    size_t rows = m.size();
    size_t cols = m[0].size();
    std::vector<std::vector<double>> res(cols, std::vector<double>(rows));
    for (size_t i = 0; i < rows; ++i) {
      for (size_t j = 0; j < cols; ++j) {
        res[j][i] = m[i][j];
      }
    }
    bool allInts = isMatrixAllInts(args[0]);
    return makeMatrix(res, allInts);
  });

  // --- String Native Builtins ---
  defNative("str_trim", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeStr("");
    return callBuiltinMethod(args[0], "trim", {}, -1);
  });
  defNative("str_trimleft", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeStr("");
    std::string s = args[0]->toString();
    size_t l = s.find_first_not_of(" \t\r\n");
    return (l == std::string::npos) ? makeStr("") : makeStr(s.substr(l));
  });
  defNative("str_trimright", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeStr("");
    std::string s = args[0]->toString();
    size_t r = s.find_last_not_of(" \t\r\n");
    return (r == std::string::npos) ? makeStr("") : makeStr(s.substr(0, r + 1));
  });
  defNative("str_upper", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeStr("");
    return callBuiltinMethod(args[0], "upper", {}, -1);
  });
  defNative("str_lower", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeStr("");
    return callBuiltinMethod(args[0], "lower", {}, -1);
  });
  defNative("str_startswith", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeBool(false);
    return callBuiltinMethod(args[0], "startsWith", {args[1]}, -1);
  });
  defNative("str_endswith", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeBool(false);
    return callBuiltinMethod(args[0], "endsWith", {args[1]}, -1);
  });
  defNative("str_contains", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeBool(false);
    return callBuiltinMethod(args[0], "contains", {args[1]}, -1);
  });
  defNative("str_replace", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 3) return args.empty() ? makeStr("") : args[0];
    return callBuiltinMethod(args[0], "replace", {args[1], args[2]}, -1);
  });
  defNative("str_split", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeList(std::make_shared<ListValue>());
    ValuePtr delim = (args.size() > 1) ? args[1] : makeStr(" ");
    return callBuiltinMethod(args[0], "split", {delim}, -1);
  });
  defNative("str_join", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeStr("");
    return callBuiltinMethod(args[0], "join", {args[1]}, -1);
  });
  defNative("str_repeat", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return args.empty() ? makeStr("") : args[0];
    return callBuiltinMethod(args[0], "repeat", {args[1]}, -1);
  });
  defNative("str_indexof", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeInt(-1);
    std::string s = args[0]->toString();
    std::string sub = args[1]->toString();
    size_t pos = s.find(sub);
    return makeInt(pos == std::string::npos ? -1 : (int64_t)pos);
  });
  defNative("str_slice", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeStr("");
    std::vector<ValuePtr> subArgs;
    if (args.size() > 1) subArgs.push_back(args[1]);
    if (args.size() > 2) subArgs.push_back(args[2]);
    return callBuiltinMethod(args[0], "slice", subArgs, -1);
  });
  defNative("str_padleft", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return args.empty() ? makeStr("") : args[0];
    std::string s = args[0]->toString();
    size_t width = (size_t)args[1]->asInt();
    char padChar = (args.size() > 2 && !args[2]->toString().empty()) ? args[2]->toString()[0] : ' ';
    if (s.size() >= width) return makeStr(s);
    return makeStr(std::string(width - s.size(), padChar) + s);
  });
  defNative("str_padright", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return args.empty() ? makeStr("") : args[0];
    std::string s = args[0]->toString();
    size_t width = (size_t)args[1]->asInt();
    char padChar = (args.size() > 2 && !args[2]->toString().empty()) ? args[2]->toString()[0] : ' ';
    if (s.size() >= width) return makeStr(s);
    return makeStr(s + std::string(width - s.size(), padChar));
  });
  defNative("str_reverse", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeStr("");
    std::string s = args[0]->toString();
    std::reverse(s.begin(), s.end());
    return makeStr(s);
  });
  defNative("str_isdigit", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeBool(false);
    std::string s = args[0]->toString();
    if (s.empty()) return makeBool(false);
    for (char c : s) if (!std::isdigit(c)) return makeBool(false);
    return makeBool(true);
  });
  defNative("str_isalpha", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeBool(false);
    std::string s = args[0]->toString();
    if (s.empty()) return makeBool(false);
    for (char c : s) if (!std::isalpha(c)) return makeBool(false);
    return makeBool(true);
  });
  defNative("str_isalphanum", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeBool(false);
    std::string s = args[0]->toString();
    if (s.empty()) return makeBool(false);
    for (char c : s) if (!std::isalnum(c)) return makeBool(false);
    return makeBool(true);
  });

  // --- IO Native Builtins ---
  defNative("io_readfile", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) throw IOError("readfile requires file path");
    std::string path = args[0]->toString();
    std::ifstream f(path);
    if (!f) throw IOError("Cannot open file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return makeStr(ss.str());
  });
  defNative("io_writefile", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) throw IOError("writefile requires path and content");
    std::string path = args[0]->toString();
    std::ofstream f(path);
    if (!f) throw IOError("Cannot write to file: " + path);
    f << args[1]->toString();
    return makeNull();
  });
  defNative("io_appendfile", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) throw IOError("appendfile requires path and content");
    std::string path = args[0]->toString();
    std::ofstream f(path, std::ios_base::app);
    if (!f) throw IOError("Cannot append to file: " + path);
    f << args[1]->toString();
    return makeNull();
  });
  defNative("io_fileexists", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeBool(false);
    return makeBool(std::filesystem::exists(args[0]->toString()));
  });
  defNative("io_deletefile", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) throw IOError("deletefile requires file path");
    std::filesystem::remove(args[0]->toString());
    return makeNull();
  });
  defNative("io_readlines", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) throw IOError("readlines requires file path");
    std::string path = args[0]->toString();
    std::ifstream f(path);
    if (!f) throw IOError("Cannot open file: " + path);
    auto lv = std::make_shared<ListValue>();
    std::string line;
    while (std::getline(f, line)) {
      lv->elements.push_back(makeStr(line));
    }
    return makeList(lv);
  });
  defNative("io_writelines", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) throw IOError("writelines requires path and list");
    std::string path = args[0]->toString();
    std::ofstream f(path);
    if (!f) throw IOError("Cannot write to file: " + path);
    if (args[1]->isList()) {
      for (auto& elem : args[1]->asList()->elements) {
        f << elem->toString() << "\n";
      }
    }
    return makeNull();
  });
  defNative("io_cwd", [](std::vector<ValuePtr>) -> ValuePtr {
    return makeStr(std::filesystem::current_path().string());
  });
  defNative("io_listdir", [](std::vector<ValuePtr> args) -> ValuePtr {
    std::string path = args.empty() ? "." : args[0]->toString();
    auto lv = std::make_shared<ListValue>();
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        lv->elements.push_back(makeStr(entry.path().filename().string()));
      }
    }
    return makeList(lv);
  });
  defNative("io_isdir", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeBool(false);
    return makeBool(std::filesystem::is_directory(args[0]->toString()));
  });
  defNative("io_isfile", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeBool(false);
    return makeBool(std::filesystem::is_regular_file(args[0]->toString()));
  });
  defNative("io_mkdir", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) throw IOError("mkdir requires path");
    std::filesystem::create_directories(args[0]->toString());
    return makeNull();
  });

  // --- Collections Native Builtins ---
  defNative("col_map", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return args.empty() ? makeList(std::make_shared<ListValue>()) : args[0];
    return callBuiltinMethod(args[0], "map", {args[1]}, -1);
  });
  defNative("col_filter", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return args.empty() ? makeList(std::make_shared<ListValue>()) : args[0];
    return callBuiltinMethod(args[0], "filter", {args[1]}, -1);
  });
  defNative("col_reduce", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeNull();
    std::vector<ValuePtr> subArgs = {args[1]};
    if (args.size() > 2) subArgs.push_back(args[2]);
    return callBuiltinMethod(args[0], "reduce", subArgs, -1);
  });
  defNative("col_any", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeBool(false);
    if (!args[0]->isList() || !args[1]->isFn()) return makeBool(false);
    for (auto& elem : args[0]->asList()->elements) {
      if (callFunction(args[1]->asFn(), {elem}, -1)->truthy()) return makeBool(true);
    }
    return makeBool(false);
  });
  defNative("col_all", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeBool(false);
    if (!args[0]->isList() || !args[1]->isFn()) return makeBool(false);
    for (auto& elem : args[0]->asList()->elements) {
      if (!callFunction(args[1]->asFn(), {elem}, -1)->truthy()) return makeBool(false);
    }
    return makeBool(true);
  });
  defNative("col_find", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeNull();
    if (!args[0]->isList() || !args[1]->isFn()) return makeNull();
    for (auto& elem : args[0]->asList()->elements) {
      if (callFunction(args[1]->asFn(), {elem}, -1)->truthy()) return elem;
    }
    return makeNull();
  });
  defNative("col_findindex", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeInt(-1);
    if (!args[0]->isList() || !args[1]->isFn()) return makeInt(-1);
    int64_t idx = 0;
    for (auto& elem : args[0]->asList()->elements) {
      if (callFunction(args[1]->asFn(), {elem}, -1)->truthy()) return makeInt(idx);
      idx++;
    }
    return makeInt(-1);
  });
  defNative("col_flatten", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeList(std::make_shared<ListValue>());
    auto res = std::make_shared<ListValue>();
    std::function<void(ValuePtr)> flattenHelper = [&](ValuePtr val) {
      if (val->isList()) {
        for (auto& elem : val->asList()->elements) flattenHelper(elem);
      } else {
        res->elements.push_back(val);
      }
    };
    flattenHelper(args[0]);
    return makeList(res);
  });
  defNative("col_unique", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty() || !args[0]->isList()) return makeList(std::make_shared<ListValue>());
    auto res = std::make_shared<ListValue>();
    for (auto& elem : args[0]->asList()->elements) {
      bool found = false;
      for (auto& ex : res->elements) {
        if (elem.equals(ex)) { found = true; break; }
      }
      if (!found) res->elements.push_back(elem);
    }
    return makeList(res);
  });
  defNative("col_sort", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty() || !args[0]->isList()) return makeList(std::make_shared<ListValue>());
    auto res = std::make_shared<ListValue>();
    res->elements = args[0]->asList()->elements;
    if (args.size() > 1 && args[1]->isFn()) {
      auto fn = args[1]->asFn();
      std::stable_sort(res->elements.begin(), res->elements.end(), [&](const ValuePtr& a, const ValuePtr& b) {
        return callFunction(fn, {a, b}, -1)->truthy();
      });
    } else {
      std::stable_sort(res->elements.begin(), res->elements.end(), [](const ValuePtr& a, const ValuePtr& b) {
        if (a->isInt() && b->isInt()) return a->asInt() < b->asInt();
        if ((a->isFloat() || a->isInt()) && (b->isFloat() || b->isInt())) return a->asFloat() < b->asFloat();
        return a->toString() < b->toString();
      });
    }
    return makeList(res);
  });
  defNative("col_sortby", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2 || !args[0]->isList() || !args[1]->isFn()) return args.empty() ? makeList(std::make_shared<ListValue>()) : args[0];
    auto res = std::make_shared<ListValue>();
    res->elements = args[0]->asList()->elements;
    auto fn = args[1]->asFn();
    std::stable_sort(res->elements.begin(), res->elements.end(), [&](const ValuePtr& a, const ValuePtr& b) {
      auto ka = callFunction(fn, {a}, -1);
      auto kb = callFunction(fn, {b}, -1);
      if (ka->isInt() && kb->isInt()) return ka->asInt() < kb->asInt();
      if ((ka->isFloat() || ka->isInt()) && (kb->isFloat() || kb->isInt())) return ka->asFloat() < kb->asFloat();
      return ka->toString() < kb->toString();
    });
    return makeList(res);
  });
  defNative("col_groupby", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2 || !args[0]->isList() || !args[1]->isFn()) return makeDict(std::make_shared<DictValue>());
    auto res = std::make_shared<DictValue>();
    auto fn = args[1]->asFn();
    for (auto& elem : args[0]->asList()->elements) {
      std::string key = callFunction(fn, {elem}, -1)->toString();
      auto bucket = res->get(key);
      if (bucket.isNull()) {
        auto nl = std::make_shared<ListValue>();
        bucket = makeList(nl);
        res->set(key, bucket);
      }
      bucket->asList()->elements.push_back(elem);
    }
    return makeDict(res);
  });
  defNative("col_zip", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2 || !args[0]->isList() || !args[1]->isList()) return makeList(std::make_shared<ListValue>());
    auto l1 = args[0]->asList();
    auto l2 = args[1]->asList();
    size_t sz = std::min(l1->elements.size(), l2->elements.size());
    auto res = std::make_shared<ListValue>();
    for (size_t i = 0; i < sz; i++) {
      auto pair = std::make_shared<ListValue>();
      pair->elements.push_back(l1->elements[i]);
      pair->elements.push_back(l2->elements[i]);
      res->elements.push_back(makeList(pair));
    }
    return makeList(res);
  });
  defNative("col_take", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2 || !args[0]->isList()) return makeList(std::make_shared<ListValue>());
    auto l = args[0]->asList();
    size_t n = (size_t)std::max((int64_t)0, args[1]->asInt());
    auto res = std::make_shared<ListValue>();
    for (size_t i = 0; i < n && i < l->elements.size(); i++) res->elements.push_back(l->elements[i]);
    return makeList(res);
  });
  defNative("col_drop", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2 || !args[0]->isList()) return makeList(std::make_shared<ListValue>());
    auto l = args[0]->asList();
    size_t n = (size_t)std::max((int64_t)0, args[1]->asInt());
    auto res = std::make_shared<ListValue>();
    for (size_t i = n; i < l->elements.size(); i++) res->elements.push_back(l->elements[i]);
    return makeList(res);
  });
  defNative("col_count", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty() || !args[0]->isList()) return makeInt(0);
    auto l = args[0]->asList();
    if (args.size() > 1 && args[1]->isFn()) {
      auto fn = args[1]->asFn();
      int64_t c = 0;
      for (auto& elem : l->elements) {
        if (callFunction(fn, {elem}, -1)->truthy()) c++;
      }
      return makeInt(c);
    }
    return makeInt(l->elements.size());
  });
  defNative("col_sum", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty() || !args[0]->isList()) return makeInt(0);
    double s = 0;
    bool isFloat = false;
    for (auto& elem : args[0]->asList()->elements) {
      if (elem->isFloat()) { s += elem->asFloat(); isFloat = true; }
      else if (elem->isInt()) { s += elem->asInt(); }
    }
    return isFloat ? makeFloat(s) : makeInt((int64_t)s);
  });
  defNative("col_min", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty() || !args[0]->isList() || args[0]->asList()->elements.empty()) return makeNull();
    auto l = args[0]->asList();
    ValuePtr m = l->elements[0];
    for (auto& elem : l->elements) {
      if (elem->isFloat() || elem->isInt()) {
        if (elem->asFloat() < m->asFloat()) m = elem;
      } else if (elem->toString() < m->toString()) {
        m = elem;
      }
    }
    return m;
  });
  defNative("col_max", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty() || !args[0]->isList() || args[0]->asList()->elements.empty()) return makeNull();
    auto l = args[0]->asList();
    ValuePtr m = l->elements[0];
    for (auto& elem : l->elements) {
      if (elem->isFloat() || elem->isInt()) {
        if (elem->asFloat() > m->asFloat()) m = elem;
      } else if (elem->toString() > m->toString()) {
        m = elem;
      }
    }
    return m;
  });
  defNative("col_keys", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeList(std::make_shared<ListValue>());
    return callBuiltinMethod(args[0], "keys", {}, -1);
  });
  defNative("col_values", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeList(std::make_shared<ListValue>());
    return callBuiltinMethod(args[0], "values", {}, -1);
  });
  defNative("col_entries", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty() || !args[0]->isDict()) return makeList(std::make_shared<ListValue>());
    auto res = std::make_shared<ListValue>();
    for (auto& kv : args[0]->asDict()->pairs) {
      auto pair = std::make_shared<ListValue>();
      pair->elements.push_back(makeStr(kv.first));
      pair->elements.push_back(kv.second);
      res->elements.push_back(makeList(pair));
    }
    return makeList(res);
  });
  defNative("col_merge", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2 || !args[0]->isDict() || !args[1]->isDict()) return args.empty() ? makeDict(std::make_shared<DictValue>()) : args[0];
    auto res = std::make_shared<DictValue>();
    res->pairs = args[0]->asDict()->pairs;
    for (auto& kv : args[1]->asDict()->pairs) {
      res->set(kv.first, kv.second);
    }
    return makeDict(res);
  });
  defNative("col_haskey", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeBool(false);
    return callBuiltinMethod(args[0], "has", {args[1]}, -1);
  });

  // Native FFI simulation
  auto getAddress = [](ValuePtr val) -> int64_t {
    if (val->isInt()) return val->asInt();
    std::string s = val->toString();
    if (s.rfind("0x", 0) == 0) {
      try {
        return std::stoll(s.substr(2), nullptr, 16);
      } catch (...) {}
    }
    try {
      return std::stoll(s);
    } catch (...) {}
    return 0;
  };

  defNative("sys_wifi_connect", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeBool(false);
    std::string ssid = args[0]->toString();
    std::string pass = args[1]->toString();
    // Simulate connection logic via native C++
    bool success = (ssid != "Unknown" && pass.length() > 3);
    return makeBool(success);
  });
  
  defNative("sys_memory_alloc", [this](std::vector<ValuePtr> args) -> ValuePtr {
    int64_t size = 8;
    if (!args.empty() && args[0]->isInt()) {
      size = args[0]->asInt();
    }
    int64_t addr = nextAddress_;
    nextAddress_ += size;
    char hex[32];
    snprintf(hex, sizeof(hex), "0x%llX", (unsigned long long)addr);
    virtualRAM_[addr] = makeNull();
    return makeStr(hex);
  });
  defNative("sys_memory_free", [this, getAddress](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeBool(false);
    try {
      int64_t addr = getAddress(args[0]);
      virtualRAM_.erase(addr);
      return makeBool(true);
    } catch (...) {
      return makeBool(false);
    }
  });
  defNative("sys_memory_read", [this, getAddress](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeNull();
    try {
      int64_t addr = getAddress(args[0]);
      auto it = virtualRAM_.find(addr);
      if (it != virtualRAM_.end()) {
        return it->second;
      }
    } catch (...) {}
    return makeNull();
  });
  defNative("sys_memory_write", [this, getAddress](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeBool(false);
    try {
      int64_t addr = getAddress(args[0]);
      virtualRAM_[addr] = args[1];
      return makeBool(true);
    } catch (...) {
      return makeBool(false);
    }
  });
  defNative("iot_gpio_mode", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeBool(false);
    int pin = (int)args[0]->asInt();
    int mode = (int)args[1]->asInt();
    gpioModes_[pin] = mode;
    return makeBool(true);
  });
  defNative("iot_gpio_write", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.size() < 2) return makeBool(false);
    int pin = (int)args[0]->asInt();
    int value = (int)args[1]->asInt();
    gpioStates_[pin] = value;
    return makeBool(true);
  });
  defNative("iot_gpio_read", [this](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeInt(0);
    int pin = (int)args[0]->asInt();
    auto it = gpioStates_.find(pin);
    if (it != gpioStates_.end()) {
      return makeInt(it->second);
    }
    return makeInt(0);
  });
  defNative("sys_process_exec", [](std::vector<ValuePtr> args) -> ValuePtr {
    if (args.empty()) return makeStr("");
    return makeStr("Executed: " + args[0]->toString());
  });

  builtinsRegistry_ = makeDict(builtins);
}
