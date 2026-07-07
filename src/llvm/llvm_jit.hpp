#pragma once

#ifdef ENABLE_LLVM

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/Shared/ExecutorAddress.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Error.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

// ---------------------------------------------------------------------------
// LLVMJIT  —  Singleton ORC LLJIT wrapper for Sulfur++
//
// LLVM 21 changes:
//  - JIT->lookup() now returns Expected<ExecutorAddr>, not ExecutorSymbolDef
//  - ExecutorAddr::toPtr<T>() is the correct way to obtain a typed function ptr
//  - Process symbol lookup is enabled via LLJIT::getMainJITDylib() + link order
// ---------------------------------------------------------------------------
class LLVMJIT {
public:
    // -----------------------------------------------------------------------
    // Initialise the native target and build the JIT.
    // Call once before any JIT compilation.
    // -----------------------------------------------------------------------
    static void init() {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
    }

    // -----------------------------------------------------------------------
    // Singleton accessor.  First call creates the JIT instance.
    // -----------------------------------------------------------------------
    static LLVMJIT &get() {
        static LLVMJIT instance;
        return instance;
    }

    // -----------------------------------------------------------------------
    // Add a compiled module to the JIT's main dylib.
    // The module and its context are transferred (moved) into a
    // ThreadSafeModule so the JIT can access them from any thread.
    // -----------------------------------------------------------------------
    void addModule(std::unique_ptr<llvm::Module> M,
                   std::unique_ptr<llvm::LLVMContext> Ctx) {
        auto tsm = llvm::orc::ThreadSafeModule(std::move(M), std::move(Ctx));
        if (auto err = JIT->addIRModule(std::move(tsm))) {
            std::string msg;
            llvm::raw_string_ostream os(msg);
            llvm::logAllUnhandledErrors(std::move(err), os,
                                        "LLVM JIT addModule: ");
            throw std::runtime_error(msg);
        }
    }

    // -----------------------------------------------------------------------
    // Look up a compiled symbol by its IR name and return a raw function ptr.
    //
    // LLVM 21 API:
    //   Expected<ExecutorAddr> LLJIT::lookup(StringRef)
    //   ExecutorAddr::toPtr<T>() -> T
    // -----------------------------------------------------------------------
    void *getFunctionPointer(const std::string &name) {
        auto symOrErr = JIT->lookup(name);
        if (auto err = symOrErr.takeError()) {
            std::string msg;
            llvm::raw_string_ostream os(msg);
            llvm::logAllUnhandledErrors(std::move(err), os,
                                        "LLVM JIT lookup '" + name + "': ");
            throw std::runtime_error(msg);
        }
        // ExecutorAddr -> typed function pointer
        return symOrErr->toPtr<void *>();
    }

    // -----------------------------------------------------------------------
    // Typed helpers for common Sulfur++ calling conventions
    // -----------------------------------------------------------------------

    template <typename Ret, typename... Args>
    auto getTypedFunction(const std::string &name)
        -> Ret (*)(Args...) {
        void *raw = getFunctionPointer(name);
        return reinterpret_cast<Ret (*)(Args...)>(raw);
    }

private:
    std::unique_ptr<llvm::orc::LLJIT> JIT;

    LLVMJIT() {
        // Ensure native target is initialised (idempotent)
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();

        auto jitOrErr = llvm::orc::LLJITBuilder().create();
        if (auto err = jitOrErr.takeError()) {
            std::string msg;
            llvm::raw_string_ostream os(msg);
            llvm::logAllUnhandledErrors(std::move(err), os,
                                        "LLJIT creation: ");
            throw std::runtime_error(msg);
        }
        JIT = std::move(*jitOrErr);

        // Expose host process symbols (libc, libm, etc.) to JIT'd code
        auto &mainDylib = JIT->getMainJITDylib();
        auto gen = llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
            JIT->getDataLayout().getGlobalPrefix());
        if (auto err = gen.takeError()) {
            // Non-fatal — JIT functions won't be able to call host symbols
            llvm::consumeError(std::move(err));
        } else {
            mainDylib.addGenerator(std::move(*gen));
        }
    }

    // Non-copyable, non-movable singleton
    LLVMJIT(const LLVMJIT &) = delete;
    LLVMJIT &operator=(const LLVMJIT &) = delete;
};

#endif // ENABLE_LLVM
