#pragma once

#ifdef ENABLE_LLVM

#include <string>
#include <vector>
#include "../../include/ast.hpp"

// ---------------------------------------------------------------------------
// AOT Compilation API
//
// Each function takes a parsed AST, emits LLVM IR / bitcode / native object.
// Only @jit-annotated (FnDeclStmt) functions are lowered; the rest of the
// AST is ignored in AOT mode.
// ---------------------------------------------------------------------------

/// Emit LLVM IR text (.ll) — useful for debugging and llc/clang hand-off
void sulfur_emit_ir(const std::vector<StmtPtr> &stmts,
                    const std::string &moduleName,
                    const std::string &outPath);

/// Emit LLVM bitcode (.bc) — compact binary IR
void sulfur_emit_bitcode(const std::vector<StmtPtr> &stmts,
                          const std::string &moduleName,
                          const std::string &outPath);

/// Emit a native object file (.o) — link with clang/ld for a standalone binary
void sulfur_emit_object(const std::vector<StmtPtr> &stmts,
                         const std::string &moduleName,
                         const std::string &outPath);

#endif // ENABLE_LLVM
