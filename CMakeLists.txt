cmake_minimum_required(VERSION 3.16)
project(Sulfur++ LANGUAGES CXX C)

# -------------------------------------------------
# LLVM / JIT support (optional)
# -------------------------------------------------
option(USE_LLVM "Enable LLVM backend / JIT" OFF)
set(USE_LLVM OFF CACHE BOOL "Force LLVM off" FORCE)

# Force C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# -------------------------------------------------
# Find LLVM early so we can adjust compiler flags
# -------------------------------------------------
set(LLVM_LIBS "")
set(CLEAN_LLVM_INCLUDES "")

if(USE_LLVM)
    find_package(LLVM REQUIRED CONFIG)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

    llvm_map_components_to_libnames(LLVM_LIBS
        core support irreader
        executionengine orcjit
        nativecodegen
        passes
        target
        mc mca
        aarch64asmparser asmparser
    )

    # Filter out Termux host system headers from include dirs
    foreach(dir ${LLVM_INCLUDE_DIRS})
        if(NOT dir MATCHES "com.termux")
            list(APPEND CLEAN_LLVM_INCLUDES ${dir})
        endif()
    endforeach()

    # Clean up imported LLVM target properties to prevent propagating bad paths
    foreach(lib ${LLVM_LIBS})
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

    message(STATUS "LLVM libs: ${LLVM_LIBS}")
endif()

# -------------------------------------------------
# Compiler flags for maximum optimization
# -------------------------------------------------
if(MSVC)
    add_compile_options(/W4 /Ot /Oy /GL)
    add_compile_options(/Zc:__cplusplus /Zc:preprocessor)
else()
    if(USE_LLVM)
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

set(ALL_CORE_SOURCES ${CORE_SOURCES})

if(USE_LLVM)
    list(APPEND ALL_CORE_SOURCES
        src/llvm/llvm_ir_builder.cpp
        src/llvm/llvm_aot.cpp
    )
endif()

# -------------------------------------------------
# sulfur shared library (core runtime)
# -------------------------------------------------
add_library(sulfur SHARED ${ALL_CORE_SOURCES})
target_include_directories(sulfur PUBLIC include)

if(USE_LLVM)
    target_include_directories(sulfur PUBLIC ${CLEAN_LLVM_INCLUDES})
    target_link_libraries(sulfur PUBLIC ${LLVM_LIBS})
    target_compile_definitions(sulfur PUBLIC ENABLE_LLVM)
endif()

# -------------------------------------------------
# combust executable
# -------------------------------------------------
add_executable(combust src/tools/combust.cpp src/tools/compiler.cpp)
target_include_directories(combust PRIVATE include)
target_link_libraries(combust PRIVATE sulfur)

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
# Convenience targets
# -------------------------------------------------
add_custom_target(run
    COMMAND $<TARGET_FILE:combust>
    DEPENDS combust
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Launching Sulfur++ REPL..."
)
