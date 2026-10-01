cmake_minimum_required(VERSION 3.16)
project(simp VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(GNUInstallDirs)

# Root builds stage a developer prefix in the source tree: <source>/bin/simp
# finds its runtime, headers, prelude, and standard modules in sibling
# directories with the same relative shape as an installed prefix.
set(SIMP_STAGE_PREFIX "${CMAKE_SOURCE_DIR}")
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

install(FILES README.md SIMPLE-LANGUAGE-NOTES.md DESTINATION "${CMAKE_INSTALL_DOCDIR}")
install(FILES doc/simp.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
