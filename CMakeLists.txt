cmake_minimum_required(VERSION 3.16)
project(Sulfur++ LANGUAGES CXX C)

# -------------------------------------------------
# LLVM / JIT support (optional)
# -------------------------------------------------
# Force LLVM backend off to avoid compilation issues
option(USE_LLVM "Enable LLVM backend / JIT" OFF)
set(USE_LLVM OFF CACHE BOOL "Force LLVM off" FORCE)

# Force C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# -------------------------------------------------
# Find LLVM early so we can adjust compiler flags
# -------------------------------------------------
if(USE_LLVM)
    find_package(LLVM REQUIRED CONFIG)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    # NOTE: We intentionally do NOT add -fno-exceptions here.
    # Sulfur++ core code uses C++ exceptions heavily.
    # LLVM 21 ships as a shared library (libLLVM-21.so) which is
    # ABI-compatible with exception-enabled callers.
endif()

# -------------------------------------------------
# Compiler flags for maximum optimization
# -------------------------------------------------
if(MSVC)
    add_compile_options(/W4 /Ot /Oy /GL)
    add_compile_options(/Zc:__cplusplus /Zc:preprocessor)
else()
    if(USE_LLVM)
        # With LLVM: no LTO (conflicts with LLVM's own LTO), keep O3
        add_compile_options(-Wall -Wextra -O3)
    else()
        add_compile_options(-Wall -Wextra -O3 -flto)
        add_link_options(-flto)
    endif()
    add_compile_definitions(_GNU_SOURCE)
endif()

# -------------------------------------------------
# Source lists
# -------------------------------------------------
set(CORE_SOURCES
    src/core/lexer.cpp
    src/core/parser.cpp
    src/core/interpreter.cpp
    src/core/environment.cpp
    src/core/value.cpp
)

set(LLVM_SOURCES
    src/llvm/llvm_ir_builder.cpp
    src/llvm/llvm_aot.cpp
)

set(COMBUST_SOURCES
    src/tools/combust.cpp
    ${CORE_SOURCES}
)

# Include LLVM sources only when LLVM backend is enabled
if(USE_LLVM)
    list(APPEND COMBUST_SOURCES ${LLVM_SOURCES})
endif()

# -------------------------------------------------
# combust executable
# -------------------------------------------------
add_executable(combust ${COMBUST_SOURCES})
target_include_directories(combust PRIVATE include)

# -------------------------------------------------
# LLVM linkage
# -------------------------------------------------
if(USE_LLVM)
    # Components needed: core IR + ORC JIT + native codegen + optimization passes
    llvm_map_components_to_libnames(llvm_libs
        core support irreader
        executionengine orcjit
        nativecodegen
        passes        # New Pass Manager (LLVM 14+)
        target
        mc mca
        aarch64asmparser asmparser
    )

    # Filter out Termux host system headers from include dirs
    set(clean_llvm_includes "")
    foreach(dir ${LLVM_INCLUDE_DIRS})
        if(NOT dir MATCHES "com.termux")
            list(APPEND clean_llvm_includes ${dir})
        endif()
    endforeach()

    # Clean up imported LLVM target properties to prevent propagating bad paths
    foreach(lib ${llvm_libs})
        if(TARGET ${lib})
            get_target_property(inc_dirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
            if(inc_dirs)
                set(clean_inc_dirs "")
                foreach(dir ${inc_dirs})
                    if(NOT dir MATCHES "com.termux")
                        list(APPEND clean_inc_dirs ${dir})
                    endif()
                endforeach()
                set_target_properties(${lib} PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${clean_inc_dirs}")
            endif()
        endif()
    endforeach()

    target_include_directories(combust PRIVATE ${clean_llvm_includes})
    target_link_libraries(combust PRIVATE ${llvm_libs})
    target_compile_definitions(combust PRIVATE ENABLE_LLVM)

    message(STATUS "LLVM libs: ${llvm_libs}")
endif()

# -------------------------------------------------
# Copy stdlib
# -------------------------------------------------
add_custom_command(TARGET combust POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/src/stdlib
            $<TARGET_FILE_DIR:combust>/std
    COMMENT "Copying standard library (.sfpp) to build directory..."
)

# -------------------------------------------------
# fuse package manager
# -------------------------------------------------
add_executable(fuse src/tools/fuse.cpp)
target_include_directories(fuse PRIVATE include)

# -------------------------------------------------
# ignitor tool
# -------------------------------------------------
add_executable(ignitor src/tools/ignitor.cpp)
target_include_directories(ignitor PRIVATE include)

# -------------------------------------------------
# Convenience targets
# -------------------------------------------------
add_custom_target(run
    COMMAND $<TARGET_FILE:combust>
    DEPENDS combust
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Launching Sulfur++ REPL..."
)

add_custom_target(example
    COMMAND $<TARGET_FILE:combust> examples/benchmarks/jit_test.sfpp
    DEPENDS combust
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running jit_test.sfpp example..."
)

add_custom_target(benchmark
    COMMAND $<TARGET_FILE:combust> examples/benchmarks/fib_jit.sfpp
    DEPENDS combust
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running JIT fibonacci benchmark..."
)
