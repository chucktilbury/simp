cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(simp_compiler VERSION 0.0.1 LANGUAGES C CXX)
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

include(GNUInstallDirs)
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)

# The staged prefix mirrors the install layout so the compiler finds its
# resources relative to its own executable both in a build and after install.
if(NOT DEFINED SIMP_STAGE_PREFIX)
    set(SIMP_STAGE_PREFIX "${CMAKE_CURRENT_BINARY_DIR}")
endif()
if(NOT DEFINED SIMP_RUNTIME_OUTPUT_DIRECTORY)
    set(SIMP_RUNTIME_OUTPUT_DIRECTORY "${SIMP_STAGE_PREFIX}/${CMAKE_INSTALL_BINDIR}")
endif()
if(NOT DEFINED SIMP_LIBRARY_OUTPUT_DIRECTORY)
    set(SIMP_LIBRARY_OUTPUT_DIRECTORY "${SIMP_STAGE_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
endif()

# Resource locations are compiled in only as paths relative to the prefix.
foreach(simp_install_kind BINDIR LIBDIR INCLUDEDIR DATADIR)
    file(RELATIVE_PATH SIMP_RELATIVE_${simp_install_kind}
         "${CMAKE_INSTALL_PREFIX}" "${CMAKE_INSTALL_FULL_${simp_install_kind}}")
    if(SIMP_RELATIVE_${simp_install_kind} STREQUAL ""
       OR SIMP_RELATIVE_${simp_install_kind} MATCHES "^\\.\\.(/|$)")
        message(FATAL_ERROR
            "CMAKE_INSTALL_${simp_install_kind} (${CMAKE_INSTALL_FULL_${simp_install_kind}}) "
            "must be inside CMAKE_INSTALL_PREFIX (${CMAKE_INSTALL_PREFIX}) for a "
            "relocatable simp installation")
    endif()
endforeach()
set(SIMP_STAGE_RUNTIME_DIRECTORY "${SIMP_STAGE_PREFIX}/${SIMP_RELATIVE_LIBDIR}/simp")
set(SIMP_STAGE_INCLUDE_DIRECTORY "${SIMP_STAGE_PREFIX}/${SIMP_RELATIVE_INCLUDEDIR}/simp")
set(SIMP_STAGE_BUILTIN_DIRECTORY "${SIMP_STAGE_PREFIX}/${SIMP_RELATIVE_DATADIR}/simp/builtin")
set(SIMP_STAGE_MODULE_DIRECTORY "${SIMP_STAGE_PREFIX}/${SIMP_RELATIVE_DATADIR}/simp/modules")
file(RELATIVE_PATH SIMPKG_STDLIB_RELATIVE_PATH
    "${SIMP_RUNTIME_OUTPUT_DIRECTORY}" "${SIMP_STAGE_MODULE_DIRECTORY}")
file(MAKE_DIRECTORY "${SIMP_RUNTIME_OUTPUT_DIRECTORY}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/../tools/simpkg.py.in"
    "${SIMP_RUNTIME_OUTPUT_DIRECTORY}/simpkg"
    @ONLY
)
file(RELATIVE_PATH SIMPKG_STDLIB_RELATIVE_PATH
    "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_FULL_DATADIR}/simp/modules")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/../tools/simpkg.py.in"
    "${CMAKE_CURRENT_BINARY_DIR}/simpkg-install"
    @ONLY
)
set(SIMP_SOURCE_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../include/simp")
set(SIMP_SOURCE_BUILTIN_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../builtin")
set(SIMP_SOURCE_STDLIB_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../stdlib")
set(SIMP_RUNTIME_HEADERS
    "${SIMP_SOURCE_INCLUDE_DIRECTORY}/Stdlib.h"
    "${SIMP_SOURCE_INCLUDE_DIRECTORY}/RuntimeDemoShims.h"
    "${SIMP_SOURCE_INCLUDE_DIRECTORY}/RuntimeGc.h"
    "${SIMP_SOURCE_INCLUDE_DIRECTORY}/RuntimeThreads.h"
)
set(SIMP_BUILTIN_SOURCES "${SIMP_SOURCE_BUILTIN_DIRECTORY}/String.simp")
file(GLOB_RECURSE SIMP_STDLIB_SOURCES CONFIGURE_DEPENDS
    "${SIMP_SOURCE_STDLIB_DIRECTORY}/*")

find_package(Threads REQUIRED)

include(CheckIncludeFile)
check_include_file(unistd.h SIMP_HAVE_UNISTD_H)
if(NOT SIMP_HAVE_UNISTD_H)
    message(FATAL_ERROR "simp requires a POSIX target with unistd.h (including inline C)")
endif()

add_library(simp_runtime STATIC RuntimeGc.c RuntimeStdlib.c RuntimeOS.c RuntimeProcess.c
    RuntimeDemoShims.c RuntimeThreads.c)
target_link_libraries(simp_runtime PUBLIC simp_headers Threads::Threads)
set_target_properties(simp_runtime PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${SIMP_STAGE_RUNTIME_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${SIMP_STAGE_RUNTIME_DIRECTORY}"
)

set(simp_stage_commands
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SIMP_STAGE_BUILTIN_DIRECTORY}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${SIMP_BUILTIN_SOURCES}
            "${SIMP_STAGE_BUILTIN_DIRECTORY}/"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SIMP_STAGE_MODULE_DIRECTORY}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${SIMP_SOURCE_STDLIB_DIRECTORY}"
            "${SIMP_STAGE_MODULE_DIRECTORY}"
)
get_filename_component(simp_source_include_real "${SIMP_SOURCE_INCLUDE_DIRECTORY}" REALPATH)
get_filename_component(simp_stage_include_real "${SIMP_STAGE_INCLUDE_DIRECTORY}" REALPATH)
if(NOT simp_source_include_real STREQUAL simp_stage_include_real)
    list(APPEND simp_stage_commands
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SIMP_STAGE_INCLUDE_DIRECTORY}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${SIMP_RUNTIME_HEADERS}
                "${SIMP_STAGE_INCLUDE_DIRECTORY}/")
endif()
add_custom_target(simp_resources ALL
    ${simp_stage_commands}
    DEPENDS ${SIMP_BUILTIN_SOURCES} ${SIMP_RUNTIME_HEADERS} ${SIMP_STDLIB_SOURCES}
    COMMENT "Staging simp builtin, runtime headers, and module directories"
    VERBATIM
)

add_library(simp_frontend
    Ast.cpp
    CommandLine.cpp
    CodeGenerator.cpp
    CodeGeneratorDebug.cpp
    CodeGeneratorDispatch.cpp
    CodeGeneratorExceptions.cpp
    CodeGeneratorLayout.cpp
    CodeGeneratorModule.cpp
    CodeGeneratorPrint.cpp
    CodeGeneratorStatements.cpp
    Diagnostic.cpp
    Lexer.cpp
    ModuleRegistry.cpp
    PackageIntegrity.cpp
    PackageRegistry.cpp
    Parser.cpp
    ParserClass.cpp
    PathResolution.cpp
    SemanticAnalyzer.cpp
    SemanticInheritance.cpp
    SemanticExpressions.cpp
    SemanticStatements.cpp
    SourceLoader.cpp
)
target_link_libraries(simp_frontend PUBLIC simp_headers)
target_compile_definitions(simp_frontend PRIVATE
    SIMP_INSTALL_BINDIR="${SIMP_RELATIVE_BINDIR}"
    SIMP_INSTALL_LIBDIR="${SIMP_RELATIVE_LIBDIR}"
    SIMP_INSTALL_INCLUDEDIR="${SIMP_RELATIVE_INCLUDEDIR}"
    SIMP_INSTALL_DATADIR="${SIMP_RELATIVE_DATADIR}"
    SIMP_RUNTIME_LIBRARY_NAME="$<TARGET_FILE_NAME:simp_runtime>"
)
add_dependencies(simp_frontend simp_resources simp_runtime)
set_target_properties(simp_frontend PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${SIMP_LIBRARY_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${SIMP_LIBRARY_OUTPUT_DIRECTORY}"
)

add_executable(simp main.cpp)
target_link_libraries(simp PRIVATE simp_frontend)
target_compile_definitions(simp PRIVATE
    SIMP_VERSION="${PROJECT_VERSION}"
    SIMP_CLANG_EXECUTABLE="${SIMP_CLANG_EXECUTABLE}"
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

install(TARGETS simp RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/simpkg-install"
    DESTINATION "${CMAKE_INSTALL_BINDIR}" RENAME simpkg)
install(TARGETS simp_runtime
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/simp"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/simp"
)
install(FILES ${SIMP_RUNTIME_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/simp")
install(FILES ${SIMP_BUILTIN_SOURCES} DESTINATION "${CMAKE_INSTALL_DATADIR}/simp/builtin")
install(DIRECTORY "${SIMP_SOURCE_STDLIB_DIRECTORY}/" DESTINATION "${CMAKE_INSTALL_DATADIR}/simp/modules")
