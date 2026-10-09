cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(cwhip_compiler VERSION 0.0.1 LANGUAGES C CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)
    find_program(CWHIP_CLANG_EXECUTABLE NAMES clang)
    if(NOT CWHIP_CLANG_EXECUTABLE)
        message(FATAL_ERROR "Clang is required to turn generated LLVM IR into an executable")
    endif()
    execute_process(
        COMMAND "${CWHIP_CLANG_EXECUTABLE}" -dumpmachine
        OUTPUT_VARIABLE CWHIP_TARGET_TRIPLE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE CWHIP_TARGET_TRIPLE_RESULT
    )
    if(NOT CWHIP_TARGET_TRIPLE_RESULT EQUAL 0 OR CWHIP_TARGET_TRIPLE STREQUAL "")
        message(FATAL_ERROR "Could not query the target triple from ${CWHIP_CLANG_EXECUTABLE}")
    endif()
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../include"
                     "${CMAKE_CURRENT_BINARY_DIR}/include")
endif()

include(GNUInstallDirs)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Instrumentation.cmake")
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)

# The staged prefix mirrors the install layout so the compiler finds its
# resources relative to its own executable both in a build and after install.
if(NOT DEFINED CWHIP_STAGE_PREFIX)
    set(CWHIP_STAGE_PREFIX "${CMAKE_CURRENT_BINARY_DIR}")
endif()
if(NOT DEFINED CWHIP_RUNTIME_OUTPUT_DIRECTORY)
    set(CWHIP_RUNTIME_OUTPUT_DIRECTORY "${CWHIP_STAGE_PREFIX}/${CMAKE_INSTALL_BINDIR}")
endif()
if(NOT DEFINED CWHIP_LIBRARY_OUTPUT_DIRECTORY)
    set(CWHIP_LIBRARY_OUTPUT_DIRECTORY "${CWHIP_STAGE_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
endif()
if("${CWHIP_RUNTIME_OUTPUT_DIRECTORY};${CWHIP_LIBRARY_OUTPUT_DIRECTORY}" MATCHES "[ \t]")
    # GNU ld depfiles do not escape spaces; retain CMake's explicit link dependencies.
    set(CMAKE_LINK_DEPENDS_USE_LINKER FALSE)
endif()

# Resource locations are compiled in only as paths relative to the prefix.
foreach(cwhip_install_kind BINDIR LIBDIR INCLUDEDIR DATADIR)
    file(RELATIVE_PATH CWHIP_RELATIVE_${cwhip_install_kind}
         "${CMAKE_INSTALL_PREFIX}" "${CMAKE_INSTALL_FULL_${cwhip_install_kind}}")
    if(CWHIP_RELATIVE_${cwhip_install_kind} STREQUAL ""
       OR CWHIP_RELATIVE_${cwhip_install_kind} MATCHES "^\\.\\.(/|$)")
        message(FATAL_ERROR
            "CMAKE_INSTALL_${cwhip_install_kind} (${CMAKE_INSTALL_FULL_${cwhip_install_kind}}) "
            "must be inside CMAKE_INSTALL_PREFIX (${CMAKE_INSTALL_PREFIX}) for a "
            "relocatable cwhip installation")
    endif()
endforeach()
set(CWHIP_STAGE_RUNTIME_DIRECTORY "${CWHIP_STAGE_PREFIX}/${CWHIP_RELATIVE_LIBDIR}/cwhip")
set(CWHIP_STAGE_INCLUDE_DIRECTORY "${CWHIP_STAGE_PREFIX}/${CWHIP_RELATIVE_INCLUDEDIR}/cwhip")
set(CWHIP_STAGE_BUILTIN_DIRECTORY "${CWHIP_STAGE_PREFIX}/${CWHIP_RELATIVE_DATADIR}/cwhip/builtin")
set(CWHIP_STAGE_MODULE_DIRECTORY "${CWHIP_STAGE_PREFIX}/${CWHIP_RELATIVE_DATADIR}/cwhip/modules")
file(RELATIVE_PATH CWHIP_PKG_STDLIB_RELATIVE_PATH
    "${CWHIP_RUNTIME_OUTPUT_DIRECTORY}" "${CWHIP_STAGE_MODULE_DIRECTORY}")
file(RELATIVE_PATH CWHIP_PKG_PREFIX_STDLIB_RELATIVE_PATH
    "${CMAKE_INSTALL_PREFIX}" "${CMAKE_INSTALL_FULL_DATADIR}/cwhip/modules")
file(MAKE_DIRECTORY "${CWHIP_RUNTIME_OUTPUT_DIRECTORY}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/../tools/cwhip-pkg.py.in"
    "${CWHIP_RUNTIME_OUTPUT_DIRECTORY}/cwhip-pkg"
    @ONLY
)
file(RELATIVE_PATH CWHIP_PKG_STDLIB_RELATIVE_PATH
    "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_FULL_DATADIR}/cwhip/modules")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/../tools/cwhip-pkg.py.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cwhip-pkg-install"
    @ONLY
)
set(CWHIP_SOURCE_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../include/cwhip")
set(CWHIP_SOURCE_BUILTIN_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../builtin")
set(CWHIP_SOURCE_STDLIB_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../stdlib")
set(CWHIP_RUNTIME_HEADERS
    "${CWHIP_SOURCE_INCLUDE_DIRECTORY}/Callbacks.h"
    "${CWHIP_SOURCE_INCLUDE_DIRECTORY}/Stdlib.h"
    "${CWHIP_SOURCE_INCLUDE_DIRECTORY}/RuntimeDemoShims.h"
    "${CWHIP_SOURCE_INCLUDE_DIRECTORY}/RuntimeGc.h"
    "${CWHIP_SOURCE_INCLUDE_DIRECTORY}/RuntimeThreads.h"
)
set(CWHIP_BUILTIN_SOURCES "${CWHIP_SOURCE_BUILTIN_DIRECTORY}/String.cw")
file(GLOB_RECURSE CWHIP_STDLIB_SOURCES CONFIGURE_DEPENDS
    "${CWHIP_SOURCE_STDLIB_DIRECTORY}/*")

find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)

include(CheckIncludeFile)
check_include_file(unistd.h CWHIP_HAVE_UNISTD_H)
if(NOT CWHIP_HAVE_UNISTD_H)
    message(FATAL_ERROR "cwhip requires a POSIX target with unistd.h (including inline C)")
endif()

add_library(cwhip_runtime STATIC RuntimeGc.c RuntimeStdlib.c RuntimeOS.c RuntimeProcess.c
    RuntimeProcessAsync.c
    RuntimeDemoShims.c RuntimeThreads.c)
target_link_libraries(cwhip_runtime PUBLIC cwhip_headers Threads::Threads)
set_target_properties(cwhip_runtime PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CWHIP_STAGE_RUNTIME_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${CWHIP_STAGE_RUNTIME_DIRECTORY}"
)

set(cwhip_stage_commands
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CWHIP_STAGE_BUILTIN_DIRECTORY}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${CWHIP_BUILTIN_SOURCES}
            "${CWHIP_STAGE_BUILTIN_DIRECTORY}/"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CWHIP_STAGE_MODULE_DIRECTORY}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CWHIP_SOURCE_STDLIB_DIRECTORY}"
            "${CWHIP_STAGE_MODULE_DIRECTORY}"
)
get_filename_component(cwhip_source_include_real "${CWHIP_SOURCE_INCLUDE_DIRECTORY}" REALPATH)
get_filename_component(cwhip_stage_include_real "${CWHIP_STAGE_INCLUDE_DIRECTORY}" REALPATH)
if(NOT cwhip_source_include_real STREQUAL cwhip_stage_include_real)
    list(APPEND cwhip_stage_commands
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CWHIP_STAGE_INCLUDE_DIRECTORY}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${CWHIP_RUNTIME_HEADERS}
                "${CWHIP_STAGE_INCLUDE_DIRECTORY}/")
endif()
set(CWHIP_SQLITE_PACKAGE_LIBRARY_DIRECTORY
    "${CWHIP_STAGE_MODULE_DIRECTORY}/sqlite/0.1.0/lib")
file(MAKE_DIRECTORY "${CWHIP_SQLITE_PACKAGE_LIBRARY_DIRECTORY}")
add_library(cwhip_sqlite_native STATIC
    "${CWHIP_SOURCE_STDLIB_DIRECTORY}/sqlite/0.1.0/native/sqlite.c")
target_link_libraries(cwhip_sqlite_native PRIVATE cwhip_headers SQLite::SQLite3)
set_target_properties(cwhip_sqlite_native PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CWHIP_SQLITE_PACKAGE_LIBRARY_DIRECTORY}"
    OUTPUT_NAME cwhip_sqlite
)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GTK4 QUIET IMPORTED_TARGET "gtk4>=4.10")
    pkg_check_modules(GTK_SOURCEVIEW QUIET IMPORTED_TARGET gtksourceview-5)
endif()
option(CWHIP_GTK "Build the GTK 4 interface package (default: detect installed dependencies)" ${GTK4_FOUND})
set(cwhip_sourceview_default OFF)
if(CWHIP_GTK AND GTK_SOURCEVIEW_FOUND)
    set(cwhip_sourceview_default ON)
endif()
option(CWHIP_GTK_SOURCEVIEW "Build the GtkSourceView editor package (default: detect installed dependencies)" ${cwhip_sourceview_default})
if(CWHIP_GTK_SOURCEVIEW AND NOT CWHIP_GTK)
    message(FATAL_ERROR "CWHIP_GTK_SOURCEVIEW requires CWHIP_GTK=ON")
endif()
if(CWHIP_GTK)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET "gtk4>=4.10")
    add_library(cwhip_gtk_native STATIC
        "${CWHIP_SOURCE_STDLIB_DIRECTORY}/gtk/0.1.0/native/gtk.c"
        "${CWHIP_SOURCE_STDLIB_DIRECTORY}/gtk/0.1.0/native/toml.c")
    target_link_libraries(cwhip_gtk_native PRIVATE cwhip_headers PkgConfig::GTK4 Threads::Threads)
    if(CWHIP_GTK_SOURCEVIEW)
        pkg_check_modules(GTK_SOURCEVIEW REQUIRED IMPORTED_TARGET gtksourceview-5)
        target_link_libraries(cwhip_gtk_native PRIVATE PkgConfig::GTK_SOURCEVIEW)
        target_compile_definitions(cwhip_gtk_native PRIVATE
            CWHIP_GTK_SOURCEVIEW=1
            CWHIP_GTK_SOURCEVIEW_SOURCE_LANG_DIR="${CWHIP_SOURCE_STDLIB_DIRECTORY}/sourceview/0.1.0/language-specs"
            CWHIP_GTK_SOURCEVIEW_INSTALL_LANG_DIR="${CMAKE_INSTALL_FULL_DATADIR}/cwhip/modules/sourceview/0.1.0/language-specs")
        set(CWHIP_GTK_SOURCEVIEW_MANIFEST
            "${CMAKE_CURRENT_BINARY_DIR}/gtk-sourceview-cwhip-package.toml")
        configure_file(
            "${CWHIP_SOURCE_STDLIB_DIRECTORY}/gtk/0.1.0/cwhip-package-sourceview.toml.in"
            "${CWHIP_GTK_SOURCEVIEW_MANIFEST}" @ONLY)
        list(APPEND cwhip_stage_commands
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${CWHIP_GTK_SOURCEVIEW_MANIFEST}"
                "${CWHIP_STAGE_MODULE_DIRECTORY}/gtk/0.1.0/cwhip-package.toml")
    else()
        list(APPEND cwhip_stage_commands
            COMMAND "${CMAKE_COMMAND}" -E remove_directory
                "${CWHIP_STAGE_MODULE_DIRECTORY}/sourceview")
    endif()
    target_compile_options(cwhip_gtk_native PRIVATE -Wall -Wextra -Wpedantic)
    set_target_properties(cwhip_gtk_native PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CWHIP_STAGE_MODULE_DIRECTORY}/gtk/0.1.0/lib"
        OUTPUT_NAME cwhip_gtk)
    message(STATUS "GTK 4 interface enabled; GTK links only into importing applications")
    if(CWHIP_GTK_SOURCEVIEW)
        message(STATUS "GtkSourceView editor package enabled")
    endif()
else()
    # Do not advertise an unusable source-only package in a non-GTK prefix.
    list(APPEND cwhip_stage_commands
        COMMAND "${CMAKE_COMMAND}" -E remove_directory "${CWHIP_STAGE_MODULE_DIRECTORY}/gtk")
    list(APPEND cwhip_stage_commands
        COMMAND "${CMAKE_COMMAND}" -E remove_directory
            "${CWHIP_STAGE_MODULE_DIRECTORY}/sourceview")
    message(STATUS "GTK 4 interface disabled (enable with -DCWHIP_GTK=ON)")
endif()

set(cwhip_stage_outputs "${CWHIP_STAGE_BUILTIN_DIRECTORY}/String.cw")
if(NOT cwhip_source_include_real STREQUAL cwhip_stage_include_real)
    foreach(header IN LISTS CWHIP_RUNTIME_HEADERS)
        get_filename_component(name "${header}" NAME)
        list(APPEND cwhip_stage_outputs "${CWHIP_STAGE_INCLUDE_DIRECTORY}/${name}")
    endforeach()
endif()
foreach(source IN LISTS CWHIP_STDLIB_SOURCES)
    file(RELATIVE_PATH relative "${CWHIP_SOURCE_STDLIB_DIRECTORY}" "${source}")
    if((NOT CWHIP_GTK AND relative MATCHES "^gtk/")
       OR (NOT CWHIP_GTK_SOURCEVIEW AND relative MATCHES "^sourceview/"))
        continue()
    endif()
    list(APPEND cwhip_stage_outputs "${CWHIP_STAGE_MODULE_DIRECTORY}/${relative}")
endforeach()
set(CWHIP_RESOURCE_STAMP "${CMAKE_CURRENT_BINARY_DIR}/cwhip-resources.stamp")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cwhip-resource-options.txt"
    CONTENT "GTK=${CWHIP_GTK}\nSOURCEVIEW=${CWHIP_GTK_SOURCEVIEW}\n")
add_custom_command(
    OUTPUT "${CWHIP_RESOURCE_STAMP}" ${cwhip_stage_outputs}
    ${cwhip_stage_commands}
    COMMAND "${CMAKE_COMMAND}" -E touch "${CWHIP_RESOURCE_STAMP}"
    DEPENDS ${CWHIP_BUILTIN_SOURCES} ${CWHIP_RUNTIME_HEADERS} ${CWHIP_STDLIB_SOURCES}
        ${CWHIP_GTK_SOURCEVIEW_MANIFEST}
        "${CMAKE_CURRENT_LIST_FILE}" "${CMAKE_CURRENT_BINARY_DIR}/cwhip-resource-options.txt"
    COMMENT "Staging cwhip builtin, runtime headers, and module directories"
    VERBATIM)
add_custom_target(cwhip_resources ALL DEPENDS "${CWHIP_RESOURCE_STAMP}" ${cwhip_stage_outputs})
add_dependencies(cwhip_resources cwhip_sqlite_native)
if(CWHIP_GTK)
    add_dependencies(cwhip_resources cwhip_gtk_native)
endif()

add_library(cwhip_frontend
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
target_link_libraries(cwhip_frontend PUBLIC cwhip_headers)
if(CWHIP_FUZZ)
    target_compile_options(cwhip_frontend PRIVATE -fsanitize=fuzzer-no-link)
endif()
target_compile_definitions(cwhip_frontend PRIVATE
    CWHIP_INSTALL_BINDIR="${CWHIP_RELATIVE_BINDIR}"
    CWHIP_INSTALL_LIBDIR="${CWHIP_RELATIVE_LIBDIR}"
    CWHIP_INSTALL_INCLUDEDIR="${CWHIP_RELATIVE_INCLUDEDIR}"
    CWHIP_INSTALL_DATADIR="${CWHIP_RELATIVE_DATADIR}"
    CWHIP_RUNTIME_LIBRARY_NAME="$<TARGET_FILE_NAME:cwhip_runtime>"
)
add_dependencies(cwhip_frontend cwhip_resources cwhip_runtime)
set_target_properties(cwhip_frontend PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CWHIP_LIBRARY_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${CWHIP_LIBRARY_OUTPUT_DIRECTORY}"
)

add_executable(cwhip main.cpp)
target_link_libraries(cwhip PRIVATE cwhip_frontend)
target_compile_definitions(cwhip PRIVATE
    CWHIP_VERSION="${PROJECT_VERSION}"
    CWHIP_CLANG_EXECUTABLE="${CWHIP_CLANG_EXECUTABLE}"
    CWHIP_TARGET_TRIPLE="${CWHIP_TARGET_TRIPLE}"
    CWHIP_SANITIZER_LIST="${CWHIP_SANITIZER_LIST}"
    CWHIP_COVERAGE=$<BOOL:${CWHIP_COVERAGE}>
)
add_dependencies(cwhip cwhip_runtime)
set_target_properties(cwhip PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CWHIP_RUNTIME_OUTPUT_DIRECTORY}"
)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/CwhipEditor.cmake")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cwhip_frontend PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(cwhip PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(cwhip_runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS cwhip RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/cwhip-pkg-install"
    DESTINATION "${CMAKE_INSTALL_BINDIR}" RENAME cwhip-pkg)
install(TARGETS cwhip_runtime
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/cwhip"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/cwhip"
)
install(FILES ${CWHIP_RUNTIME_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cwhip")
install(FILES ${CWHIP_BUILTIN_SOURCES} DESTINATION "${CMAKE_INSTALL_DATADIR}/cwhip/builtin")
install(DIRECTORY "${CWHIP_STAGE_MODULE_DIRECTORY}/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cwhip/modules")
