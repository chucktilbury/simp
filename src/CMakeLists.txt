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
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Instrumentation.cmake")
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
if("${SIMP_RUNTIME_OUTPUT_DIRECTORY};${SIMP_LIBRARY_OUTPUT_DIRECTORY}" MATCHES "[ \t]")
    # GNU ld depfiles do not escape spaces; retain CMake's explicit link dependencies.
    set(CMAKE_LINK_DEPENDS_USE_LINKER FALSE)
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
file(RELATIVE_PATH SIMPKG_PREFIX_STDLIB_RELATIVE_PATH
    "${CMAKE_INSTALL_PREFIX}" "${CMAKE_INSTALL_FULL_DATADIR}/simp/modules")
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
    "${SIMP_SOURCE_INCLUDE_DIRECTORY}/Callbacks.h"
    "${SIMP_SOURCE_INCLUDE_DIRECTORY}/Stdlib.h"
    "${SIMP_SOURCE_INCLUDE_DIRECTORY}/RuntimeDemoShims.h"
    "${SIMP_SOURCE_INCLUDE_DIRECTORY}/RuntimeGc.h"
    "${SIMP_SOURCE_INCLUDE_DIRECTORY}/RuntimeThreads.h"
)
set(SIMP_BUILTIN_SOURCES "${SIMP_SOURCE_BUILTIN_DIRECTORY}/String.simp")
file(GLOB_RECURSE SIMP_STDLIB_SOURCES CONFIGURE_DEPENDS
    "${SIMP_SOURCE_STDLIB_DIRECTORY}/*")

find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)

include(CheckIncludeFile)
check_include_file(unistd.h SIMP_HAVE_UNISTD_H)
if(NOT SIMP_HAVE_UNISTD_H)
    message(FATAL_ERROR "simp requires a POSIX target with unistd.h (including inline C)")
endif()

add_library(simp_runtime STATIC RuntimeGc.c RuntimeStdlib.c RuntimeOS.c RuntimeProcess.c
    RuntimeProcessAsync.c
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
set(SIMP_SQLITE_PACKAGE_LIBRARY_DIRECTORY
    "${SIMP_STAGE_MODULE_DIRECTORY}/sqlite/0.1.0/lib")
file(MAKE_DIRECTORY "${SIMP_SQLITE_PACKAGE_LIBRARY_DIRECTORY}")
add_library(simp_sqlite_native STATIC
    "${SIMP_SOURCE_STDLIB_DIRECTORY}/sqlite/0.1.0/native/sqlite.c")
target_link_libraries(simp_sqlite_native PRIVATE simp_headers SQLite::SQLite3)
set_target_properties(simp_sqlite_native PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${SIMP_SQLITE_PACKAGE_LIBRARY_DIRECTORY}"
    OUTPUT_NAME simp_sqlite
)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GTK4 QUIET IMPORTED_TARGET "gtk4>=4.10")
    pkg_check_modules(GTK_SOURCEVIEW QUIET IMPORTED_TARGET gtksourceview-5)
endif()
option(SIMP_GTK "Build the GTK 4 interface package (default: detect installed dependencies)" ${GTK4_FOUND})
set(simp_sourceview_default OFF)
if(SIMP_GTK AND GTK_SOURCEVIEW_FOUND)
    set(simp_sourceview_default ON)
endif()
option(SIMP_GTK_SOURCEVIEW "Build the GtkSourceView editor package (default: detect installed dependencies)" ${simp_sourceview_default})
if(SIMP_GTK_SOURCEVIEW AND NOT SIMP_GTK)
    message(FATAL_ERROR "SIMP_GTK_SOURCEVIEW requires SIMP_GTK=ON")
endif()
if(SIMP_GTK)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET "gtk4>=4.10")
    add_library(simp_gtk_native STATIC
        "${SIMP_SOURCE_STDLIB_DIRECTORY}/gtk/0.1.0/native/gtk.c"
        "${SIMP_SOURCE_STDLIB_DIRECTORY}/gtk/0.1.0/native/toml.c")
    target_link_libraries(simp_gtk_native PRIVATE simp_headers PkgConfig::GTK4 Threads::Threads)
    if(SIMP_GTK_SOURCEVIEW)
        pkg_check_modules(GTK_SOURCEVIEW REQUIRED IMPORTED_TARGET gtksourceview-5)
        target_link_libraries(simp_gtk_native PRIVATE PkgConfig::GTK_SOURCEVIEW)
        target_compile_definitions(simp_gtk_native PRIVATE
            SIMP_GTK_SOURCEVIEW=1
            SIMP_GTK_SOURCEVIEW_SOURCE_LANG_DIR="${SIMP_SOURCE_STDLIB_DIRECTORY}/sourceview/0.1.0/language-specs"
            SIMP_GTK_SOURCEVIEW_INSTALL_LANG_DIR="${CMAKE_INSTALL_FULL_DATADIR}/simp/modules/sourceview/0.1.0/language-specs")
        set(SIMP_GTK_SOURCEVIEW_MANIFEST
            "${CMAKE_CURRENT_BINARY_DIR}/gtk-sourceview-simp-package.toml")
        configure_file(
            "${SIMP_SOURCE_STDLIB_DIRECTORY}/gtk/0.1.0/simp-package-sourceview.toml.in"
            "${SIMP_GTK_SOURCEVIEW_MANIFEST}" @ONLY)
        list(APPEND simp_stage_commands
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${SIMP_GTK_SOURCEVIEW_MANIFEST}"
                "${SIMP_STAGE_MODULE_DIRECTORY}/gtk/0.1.0/simp-package.toml")
    else()
        list(APPEND simp_stage_commands
            COMMAND "${CMAKE_COMMAND}" -E remove_directory
                "${SIMP_STAGE_MODULE_DIRECTORY}/sourceview")
    endif()
    target_compile_options(simp_gtk_native PRIVATE -Wall -Wextra -Wpedantic)
    set_target_properties(simp_gtk_native PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${SIMP_STAGE_MODULE_DIRECTORY}/gtk/0.1.0/lib"
        OUTPUT_NAME simp_gtk)
    message(STATUS "GTK 4 interface enabled; GTK links only into importing applications")
    if(SIMP_GTK_SOURCEVIEW)
        message(STATUS "GtkSourceView editor package enabled")
    endif()
else()
    # Do not advertise an unusable source-only package in a non-GTK prefix.
    list(APPEND simp_stage_commands
        COMMAND "${CMAKE_COMMAND}" -E remove_directory "${SIMP_STAGE_MODULE_DIRECTORY}/gtk")
    list(APPEND simp_stage_commands
        COMMAND "${CMAKE_COMMAND}" -E remove_directory
            "${SIMP_STAGE_MODULE_DIRECTORY}/sourceview")
    message(STATUS "GTK 4 interface disabled (enable with -DSIMP_GTK=ON)")
endif()

set(simp_stage_outputs "${SIMP_STAGE_BUILTIN_DIRECTORY}/String.simp")
if(NOT simp_source_include_real STREQUAL simp_stage_include_real)
    foreach(header IN LISTS SIMP_RUNTIME_HEADERS)
        get_filename_component(name "${header}" NAME)
        list(APPEND simp_stage_outputs "${SIMP_STAGE_INCLUDE_DIRECTORY}/${name}")
    endforeach()
endif()
foreach(source IN LISTS SIMP_STDLIB_SOURCES)
    file(RELATIVE_PATH relative "${SIMP_SOURCE_STDLIB_DIRECTORY}" "${source}")
    if((NOT SIMP_GTK AND relative MATCHES "^gtk/")
       OR (NOT SIMP_GTK_SOURCEVIEW AND relative MATCHES "^sourceview/"))
        continue()
    endif()
    list(APPEND simp_stage_outputs "${SIMP_STAGE_MODULE_DIRECTORY}/${relative}")
endforeach()
set(SIMP_RESOURCE_STAMP "${CMAKE_CURRENT_BINARY_DIR}/simp-resources.stamp")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/simp-resource-options.txt"
    CONTENT "GTK=${SIMP_GTK}\nSOURCEVIEW=${SIMP_GTK_SOURCEVIEW}\n")
add_custom_command(
    OUTPUT "${SIMP_RESOURCE_STAMP}" ${simp_stage_outputs}
    ${simp_stage_commands}
    COMMAND "${CMAKE_COMMAND}" -E touch "${SIMP_RESOURCE_STAMP}"
    DEPENDS ${SIMP_BUILTIN_SOURCES} ${SIMP_RUNTIME_HEADERS} ${SIMP_STDLIB_SOURCES}
        ${SIMP_GTK_SOURCEVIEW_MANIFEST}
        "${CMAKE_CURRENT_LIST_FILE}" "${CMAKE_CURRENT_BINARY_DIR}/simp-resource-options.txt"
    COMMENT "Staging simp builtin, runtime headers, and module directories"
    VERBATIM)
add_custom_target(simp_resources ALL DEPENDS "${SIMP_RESOURCE_STAMP}" ${simp_stage_outputs})
add_dependencies(simp_resources simp_sqlite_native)
if(SIMP_GTK)
    add_dependencies(simp_resources simp_gtk_native)
endif()

add_library(simp_frontend
    Ast.cpp
    CommandLine.cpp
    CodeGenerator.cpp
    CodeGeneratorCallbacks.cpp
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
if(SIMP_FUZZ)
    target_compile_options(simp_frontend PRIVATE -fsanitize=fuzzer-no-link)
endif()
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
    SIMP_SANITIZER_LIST="${SIMP_SANITIZER_LIST}"
    SIMP_COVERAGE=$<BOOL:${SIMP_COVERAGE}>
)
add_dependencies(simp simp_runtime)
set_target_properties(simp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${SIMP_RUNTIME_OUTPUT_DIRECTORY}"
)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Tweed.cmake")

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
install(DIRECTORY "${SIMP_STAGE_MODULE_DIRECTORY}/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/simp/modules")
