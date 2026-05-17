#pragma once
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <variant>
#include "token.hpp"

struct Expr;
struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// ─── Expression Nodes ────────────────────────────────────────────────────────

struct IntLitExpr    { int64_t value; int line; };
struct FloatLitExpr  { double value; int line; };
struct BoolLitExpr   { bool value; int line; };
struct NullLitExpr   { int line; };
struct StringLitExpr { std::string value; int line; };
struct CharLitExpr   { char value; int line; };

// ps"..." with segments: either plain text or {expression}
struct PSSegment {
    bool isExpr;
    std::string text;          // for plain text
    ExprPtr expr;              // for {expression}
    std::string fmtSpec;       // e.g., ":join(', ')" or ":repeat(10)"
};
struct PSStringExpr { std::vector<PSSegment> segments; int line; };

struct IdentExpr { std::string name; int line; };

struct BinaryExpr {
    std::string op;
    ExprPtr left, right;
    int line;
};

struct UnaryExpr {
    std::string op;
    ExprPtr operand;
    int line;
};

struct AssignExpr {
    ExprPtr target;
    std::string op; // = += -= *= /=
    ExprPtr value;
    int line;
};

struct CallExpr {
    ExprPtr callee;
    std::vector<ExprPtr> args;
    int line;
};

struct IndexExpr {
    ExprPtr object;
    ExprPtr index;
    int line;
};

struct MemberExpr {
    ExprPtr object;
    std::string member;
    bool safe; // ?. vs .
    int line;
};

struct PipelineExpr {
    ExprPtr left;
    ExprPtr right; // should be a call or ident
    int line;
};

struct NullCoalExpr {
    ExprPtr left, right;
    int line;
};

struct ListLitExpr {
    std::vector<ExprPtr> elements;
    int line;
};

struct DictLitExpr {
    std::vector<std::pair<ExprPtr, ExprPtr>> pairs;
    int line;
};

struct LambdaExpr {
    std::vector<std::pair<std::string,std::string>> params; // name, type
    std::string retType;
    StmtPtr body;
    int line;
};

struct NewExpr {
    std::string className;
    std::vector<ExprPtr> args;
    int line;
};

struct TernaryExpr {
    ExprPtr cond, thenExpr, elseExpr;
    int line;
};

struct AddrOfExpr { ExprPtr operand; int line; };
struct DerefExpr  { ExprPtr operand; int line; };

struct Expr {
    std::variant<
        IntLitExpr, FloatLitExpr, BoolLitExpr, NullLitExpr,
        StringLitExpr, CharLitExpr, PSStringExpr,
        IdentExpr, BinaryExpr, UnaryExpr, AssignExpr,
        CallExpr, IndexExpr, MemberExpr,
        PipelineExpr, NullCoalExpr,
        ListLitExpr, DictLitExpr,
        LambdaExpr, NewExpr, TernaryExpr,
        AddrOfExpr, DerefExpr
    > data;

    template<typename T>
    Expr(T&& v) : data(std::forward<T>(v)) {}

    int line() const {
        return std::visit([](auto& e) { return e.line; }, data);
    }
};

// ─── Statement Nodes ─────────────────────────────────────────────────────────

struct VarDeclStmt {
    std::string keyword; // let, var, auto, dyn, reactive
    std::string name;
    std::string type;    // may be empty for auto/dyn
    ExprPtr initializer; // may be null
    int line;
};

struct FnDeclStmt {
    std::string name;
    std::vector<std::pair<std::string,std::string>> params; // name, type
    std::string retType;
    StmtPtr body;
    bool isMethod;
    int line;
};

struct ReturnStmt   { ExprPtr value; int line; };
struct BreakStmt    { int line; };
struct ContinueStmt { int line; };

struct BlockStmt { std::vector<StmtPtr> stmts; int line; };

struct IfStmt {
    ExprPtr cond;
    StmtPtr thenBranch;
    StmtPtr elseBranch;
    int line;
};

struct WhileStmt {
    ExprPtr cond;
    StmtPtr body;
    int line;
};

struct ForStmt {
    std::string var;
    ExprPtr iterable;
    StmtPtr body;
    int line;
};

struct ClassDeclStmt {
    std::string name;
    std::vector<std::string> interfaces;
    std::vector<StmtPtr> members;
    // Lifecycle orders: +N>methodName, ~N>methodName
    std::vector<std::pair<int,std::string>> ctorOrder;
    std::vector<std::pair<int,std::string>> dtorOrder;
    int line;
};

struct StructDeclStmt {
    std::string name;
    std::vector<std::pair<std::string,std::string>> fields; // name, type
    int line;
};

struct InterfaceDeclStmt {
    std::string name;
    std::vector<FnDeclStmt> methods;
    int line;
};

struct ExprStmt { ExprPtr expr; int line; };

struct StreamOutStmt {
    ExprPtr target;   // Terminal.Out / Terminal.Warn / Terminal.Err
    ExprPtr value;
    int line;
};

struct WatchStmt {
    std::string varName;
    ExprPtr cond;    // optional condition (watch hp <= 0 {...})
    StmtPtr body;
    int line;
};

struct SignalDeclStmt { std::string name; int line; };
struct EmitStmt       { std::string name; int line; };

struct OnStmt {
    std::string signal;
    StmtPtr body;
    int line;
};

struct ImportStmt {
    std::string pkg;        // @asep/mathlib
    std::string alias;      // as math
    std::vector<std::string> flags; // --use=[FULL, NOLIBNAME]
    int line;
};

struct UnsafeStmt { StmtPtr body; int line; };
struct DeferStmt  { StmtPtr body; int line; };

struct Stmt {
    std::variant<
        VarDeclStmt, FnDeclStmt,
        ReturnStmt, BreakStmt, ContinueStmt,
        BlockStmt, IfStmt, WhileStmt, ForStmt,
        ClassDeclStmt, StructDeclStmt, InterfaceDeclStmt,
        ExprStmt, StreamOutStmt,
        WatchStmt, SignalDeclStmt, EmitStmt, OnStmt,
        ImportStmt, UnsafeStmt, DeferStmt
    > data;

    template<typename T>
    Stmt(T&& v) : data(std::forward<T>(v)) {}
};
