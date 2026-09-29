cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(simp_compiler LANGUAGES C CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)
    find_program(SIMP_CLANG_EXECUTABLE NAMES clang)
    if(NOT SIMP_CLANG_EXECUTABLE)
        message(FATAL_ERROR "Clang is required to turn generated LLVM IR into an executable")
    endif()
    execute_process(
        COMMAND "${SIMP_CLANG_EXECUTABLE}" -dumpmachine
        OUTPUT_VARIABLE SIMP_TARGET_TRIPLE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE SIMP_TARGET_TRIPLE_RESULT
    )
    if(NOT SIMP_TARGET_TRIPLE_RESULT EQUAL 0 OR SIMP_TARGET_TRIPLE STREQUAL "")
        message(FATAL_ERROR "Could not query the target triple from ${SIMP_CLANG_EXECUTABLE}")
    endif()
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../include"
                     "${CMAKE_CURRENT_BINARY_DIR}/include")
endif()

if(NOT DEFINED SIMP_RUNTIME_OUTPUT_DIRECTORY)
    set(SIMP_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
endif()
if(NOT DEFINED SIMP_LIBRARY_OUTPUT_DIRECTORY)
    set(SIMP_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
endif()

add_library(simp_runtime STATIC RuntimeGc.c)
target_link_libraries(simp_runtime PUBLIC simp_headers)
set_target_properties(simp_runtime PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${SIMP_LIBRARY_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${SIMP_LIBRARY_OUTPUT_DIRECTORY}"
)

add_library(simp_frontend
    Ast.cpp
    CodeGenerator.cpp
    CodeGeneratorDispatch.cpp
    CodeGeneratorExceptions.cpp
    CodeGeneratorLayout.cpp
    CodeGeneratorModule.cpp
    CodeGeneratorPrint.cpp
    CodeGeneratorStatements.cpp
    Diagnostic.cpp
    Lexer.cpp
    ModuleRegistry.cpp
    Parser.cpp
    ParserClass.cpp
    SemanticAnalyzer.cpp
    SemanticInheritance.cpp
    SemanticExpressions.cpp
    SemanticStatements.cpp
    SourceLoader.cpp
)
target_link_libraries(simp_frontend PUBLIC simp_headers)
set_target_properties(simp_frontend PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${SIMP_LIBRARY_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${SIMP_LIBRARY_OUTPUT_DIRECTORY}"
)

add_executable(simp main.cpp)
target_link_libraries(simp PRIVATE simp_frontend)
target_compile_definitions(simp PRIVATE
    SIMP_CLANG_EXECUTABLE="${SIMP_CLANG_EXECUTABLE}"
    SIMP_GC_RUNTIME_LIBRARY="$<TARGET_FILE:simp_runtime>"
    SIMP_TARGET_TRIPLE="${SIMP_TARGET_TRIPLE}"
)
add_dependencies(simp simp_runtime)
set_target_properties(simp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${SIMP_RUNTIME_OUTPUT_DIRECTORY}"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(simp_frontend PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(simp PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(simp_runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()
