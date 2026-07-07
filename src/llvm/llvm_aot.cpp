#ifdef ENABLE_LLVM

//
// llvm_aot.cpp  —  Ahead-of-Time (AOT) compilation for Sulfur++
//
// Translates a Sulfur++ source file into:
//   * LLVM IR text   (.ll)
//   * LLVM bitcode   (.bc)
//   * Native object  (.o)
//
// The main entry point is sulfur_aot_compile(), called from combust
// when the --aot flag is provided.
//

#include "llvm_aot.hpp"
#include "llvm_ir_builder.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>

#include <llvm/Bitcode/BitcodeWriter.h>

#include <llvm/Support/FileSystem.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include <llvm/MC/TargetRegistry.h>

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/IR/PassManager.h>

#include "../../include/ast.hpp"

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static std::unique_ptr<llvm::TargetMachine>
createNativeTargetMachine(std::string &errOut) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    std::string triple = llvm::sys::getDefaultTargetTriple();
    const llvm::Target *target =
        llvm::TargetRegistry::lookupTarget(triple, errOut);
    if (!target) return nullptr;

    llvm::TargetOptions opt;
    auto reloc = llvm::Reloc::PIC_;

    return std::unique_ptr<llvm::TargetMachine>(
        target->createTargetMachine(llvm::Triple(triple), "generic", "", opt, reloc));
}

static void optimizeModule(llvm::Module &M,
                            llvm::OptimizationLevel level =
                                llvm::OptimizationLevel::O3) {
    llvm::LoopAnalysisManager     LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager    CGAM;
    llvm::ModuleAnalysisManager   MAM;

    llvm::PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    llvm::ModulePassManager MPM =
        PB.buildPerModuleDefaultPipeline(level);
    MPM.run(M, MAM);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void sulfur_emit_ir(const std::vector<StmtPtr> &stmts,
                    const std::string &moduleName,
                    const std::string &outPath) {
    llvm::LLVMContext ctx;
    LLVMIRBuilder builder(ctx);
    builder.createModule(moduleName);

    builder.forwardDeclareAll(stmts);
    builder.emitAllFunctions(stmts);
    builder.optimizeModule();

    std::error_code ec;
    llvm::raw_fd_ostream out(outPath, ec, llvm::sys::fs::OF_Text);
    if (ec)
        throw std::runtime_error("Cannot open output file: " + ec.message());

    builder.getModule()->print(out, nullptr);
    std::cerr << "[AOT] Emitted LLVM IR to " << outPath << "\n";
}

void sulfur_emit_bitcode(const std::vector<StmtPtr> &stmts,
                          const std::string &moduleName,
                          const std::string &outPath) {
    llvm::LLVMContext ctx;
    LLVMIRBuilder builder(ctx);
    builder.createModule(moduleName);

    builder.forwardDeclareAll(stmts);
    builder.emitAllFunctions(stmts);
    builder.optimizeModule();

    std::error_code ec;
    llvm::raw_fd_ostream out(outPath, ec, llvm::sys::fs::OF_None);
    if (ec)
        throw std::runtime_error("Cannot open output file: " + ec.message());

    llvm::WriteBitcodeToFile(*builder.getModule(), out);
    std::cerr << "[AOT] Emitted LLVM bitcode to " << outPath << "\n";
}

void sulfur_emit_object(const std::vector<StmtPtr> &stmts,
                         const std::string &moduleName,
                         const std::string &outPath) {
    llvm::LLVMContext ctx;
    LLVMIRBuilder builder(ctx);
    builder.createModule(moduleName);

    builder.forwardDeclareAll(stmts);
    builder.emitAllFunctions(stmts);

    // --- Set data layout & target triple BEFORE optimisation
    std::string tmErr;
    auto TM = createNativeTargetMachine(tmErr);
    if (!TM)
        throw std::runtime_error("Could not create target machine: " + tmErr);

    llvm::Module *M = builder.getModule();
    M->setDataLayout(TM->createDataLayout());
    M->setTargetTriple(llvm::Triple(TM->getTargetTriple().getTriple()));

    // Optimise (with correct data layout so the passes are accurate)
    builder.optimizeModule();

    // Emit object file via legacy pass manager
    std::error_code ec;
    llvm::raw_fd_ostream dest(outPath, ec, llvm::sys::fs::OF_None);
    if (ec)
        throw std::runtime_error("Cannot open output file: " + ec.message());

    llvm::legacy::PassManager pm;
    if (TM->addPassesToEmitFile(pm, dest, nullptr,
                                llvm::CodeGenFileType::ObjectFile)) {
        throw std::runtime_error(
            "Target machine cannot emit object files");
    }
    pm.run(*M);
    dest.flush();

    std::cerr << "[AOT] Emitted native object to " << outPath << "\n";
}

#endif // ENABLE_LLVM
