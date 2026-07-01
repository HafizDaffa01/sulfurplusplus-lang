cmake_minimum_required(VERSION 3.12)
project(Sulfur++ LANGUAGES CXX)

# Force C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for maximum optimization and warnings
if(MSVC)
    add_compile_options(/W4 /Ot /Oy /GL)
    # Enable standards-conforming preprocessor and structured bindings in MSVC
    add_compile_options(/Zc:__cplusplus /Zc:preprocessor)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

# Explicitly list source files for combust to avoid including fuse.cpp
set(COMBUST_SOURCES
    src/tools/combust.cpp
    src/core/lexer.cpp
    src/core/parser.cpp
    src/core/interpreter.cpp
    src/core/environment.cpp
    src/core/value.cpp
 
)

# Define the combust executable target
add_executable(combust ${COMBUST_SOURCES})

# Menyalin folder src/std ke folder build agar berada di parent yang sama dengan combust
add_custom_command(TARGET combust POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/src/std
            $<TARGET_FILE_DIR:combust>/std
    COMMENT "Menyalin standard library Sulfur++ (.sfpp) ke folder build..."
)


# Define the fuse package manager executable target
add_executable(fuse src/tools/fuse.cpp)

# Define the ignitor tool executable target
add_executable(ignitor src/tools/ignitor.cpp)


# Specify include directories for compilation
target_include_directories(combust PRIVATE include)
target_include_directories(fuse PRIVATE include)
target_include_directories(ignitor PRIVATE include)

# Convenience target to run the REPL directly
add_custom_target(run
    COMMAND $<TARGET_FILE:combust>
    DEPENDS combust
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Launching Sulfur++ REPL..."
)

# Convenience target to run the hello example
add_custom_target(example
    COMMAND $<TARGET_FILE:combust> examples/hello.sfpp
    DEPENDS combust
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running hello.sfpp example..."
)
