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

# -------------------------------------------------
# Find libcurl for HTTP client (optional)
# -------------------------------------------------
find_package(CURL QUIET)
if(CURL_FOUND)
    message(STATUS "Found libcurl: ${CURL_VERSION}")
endif()

if(USE_LLVM)

# -------------------------------------------------
# Find LLVM early so we can adjust compiler flags
# -------------------------------------------------
set(LLVM_LIBS "")
set(CLEAN_LLVM_INCLUDES "")

# -------------------------------------------------
# Compiler flags for maximum optimization
# -------------------------------------------------
if(MSVC)
    add_compile_options(/W4 /Ot /Oy /GL)
    add_compile_options(/Zc:__cplusplus /Zc:preprocessor)
else()
    add_compile_options(-Wall -Wextra -O3)
    if(NOT USE_LLVM)
        add_compile_options(-flto)
        add_link_options(-flto)
    endif()
    add_compile_definitions(_GNU_SOURCE)
endif()
endif()

# -------------------------------------------------
# Source lists
# -------------------------------------------------
set(CORE_SOURCES
    src/core/lexer.cpp
    src/core/parser.cpp
    src/core/resolver.cpp
    src/core/interpreter.cpp
    src/core/environment.cpp
    src/core/value.cpp
    src/core/semantic/type.cpp
    src/core/semantic/symbol_table.cpp
    src/core/semantic/type_checker.cpp
    src/core/semantic/analyzer.cpp
    src/core/formatter.cpp
    src/core/linter.cpp
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

# Link libcurl for HTTP client
target_link_libraries(sulfur PUBLIC ${CURL_LIBRARIES})
target_include_directories(sulfur PUBLIC ${CURL_INCLUDE_DIRS})

# -------------------------------------------------
# combust executable
# -------------------------------------------------
add_executable(combust src/tools/combust.cpp src/tools/compiler.cpp)
target_include_directories(combust PRIVATE include)
target_link_libraries(combust PRIVATE sulfur)

# -------------------------------------------------
# sfpm executable (Package Manager)
# -------------------------------------------------
add_executable(sfpm src/tools/sfpm.cpp)
target_include_directories(sfpm PRIVATE include)
target_link_libraries(sfpm PRIVATE sulfur)

# -------------------------------------------------
# sulfur-lsp executable (Language Server Protocol)
# -------------------------------------------------
add_executable(sulfur-lsp src/tools/lsp.cpp)
target_include_directories(sulfur-lsp PRIVATE include)
target_link_libraries(sulfur-lsp PRIVATE sulfur)

# -------------------------------------------------
# sulfur-debug executable (Interactive Debugger)
# -------------------------------------------------
add_executable(sulfur-debug src/tools/debugger.cpp)
target_include_directories(sulfur-debug PRIVATE include)
target_link_libraries(sulfur-debug PRIVATE sulfur)

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

# -------------------------------------------------
# Automated Testing (CTest)
# -------------------------------------------------
enable_testing()

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.sfpp")
foreach(TEST_PATH ${TEST_FILES})
    get_filename_component(TEST_NAME ${TEST_PATH} NAME_WE)
    add_test(NAME ${TEST_NAME}
             COMMAND combust ${TEST_PATH}
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()

