cmake_minimum_required(VERSION 3.16)
project(simp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Root builds place runtime targets in the source-tree bin directory.
set(SIMP_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(SIMP_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

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

enable_testing()
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(tests)
