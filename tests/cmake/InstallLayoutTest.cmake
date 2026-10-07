# Installs the configured build into a temporary prefix, then compiles and runs
# a package-importing fixture with only the installed compiler and resources.
foreach(required IN ITEMS BUILD_DIR SOURCE_DIR FIXTURE MODULE_FIXTURE EXPECTED_OUTPUT_FILE
        BINDIR LIBDIR INCLUDEDIR DATADIR DOCDIR MANDIR RUNTIME_LIBRARY_NAME CLANG)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

if(DEFINED ENV{TMPDIR} AND IS_DIRECTORY "$ENV{TMPDIR}")
    set(temporary_root "$ENV{TMPDIR}")
else()
    set(temporary_root "/tmp")
endif()
string(RANDOM LENGTH 12 random_suffix)
set(work_directory "${temporary_root}/simp-install-test-${random_suffix}")
file(REMOVE_RECURSE "${work_directory}")
file(MAKE_DIRECTORY "${work_directory}")
foreach(forbidden_root IN ITEMS "${SOURCE_DIR}" "${BUILD_DIR}")
    string(FIND "${work_directory}" "${forbidden_root}" nested_position)
    if(nested_position EQUAL 0)
        message(FATAL_ERROR "Install test work directory must be outside ${forbidden_root}")
    endif()
endforeach()

set(clean_environment
    --unset=SIMP_HOME --unset=SIMP_RUNTIME_DIR --unset=SIMP_INCLUDE_DIR
    --unset=SIMP_BUILTIN_DIR --unset=SIMP_STDLIB_MODULE_DIR --unset=SIMP_MODULE_DIR
    --unset=SIMP_PACKAGE_PATH --unset=SIMP_MODULE_REGISTRY --unset=DESTDIR
    "HOME=${work_directory}/home" "XDG_CONFIG_HOME=${work_directory}/xdg")
file(MAKE_DIRECTORY "${work_directory}/home" "${work_directory}/xdg")

function(run_checked description)
    execute_process(COMMAND ${ARGN}
        WORKING_DIRECTORY "${work_directory}"
        RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${description} failed (${result}):\n${stdout}${stderr}")
    endif()
    set(last_output "${stdout}${stderr}" PARENT_SCOPE)
endfunction()

function(require_installed_layout prefix)
    foreach(installed_file IN ITEMS
            "${BINDIR}/simp"
            "${BINDIR}/simpkg"
            "${LIBDIR}/simp/${RUNTIME_LIBRARY_NAME}"
            "${INCLUDEDIR}/simp/RuntimeGc.h"
            "${INCLUDEDIR}/simp/Stdlib.h"
            "${INCLUDEDIR}/simp/RuntimeThreads.h"
            "${INCLUDEDIR}/simp/RuntimeDemoShims.h"
            "${DATADIR}/simp/builtin/String.simp"
            "${DOCDIR}/README.md"
            "${DOCDIR}/SIMPLE-LANGUAGE-NOTES.md"
            "${MANDIR}/man1/simp.1")
        if(NOT EXISTS "${prefix}/${installed_file}")
            message(FATAL_ERROR "Install is missing ${prefix}/${installed_file}")
        endif()
    endforeach()
    if(NOT IS_DIRECTORY "${prefix}/${DATADIR}/simp/modules")
        message(FATAL_ERROR "Install is missing ${prefix}/${DATADIR}/simp/modules")
    endif()
    file(GLOB_RECURSE simp_files_under_include "${prefix}/${INCLUDEDIR}/*.simp")
    if(simp_files_under_include)
        message(FATAL_ERROR "Found .simp files installed under include: ${simp_files_under_include}")
    endif()
endfunction()

function(require_no_tree_references text context)
    foreach(forbidden_root IN ITEMS "${SOURCE_DIR}" "${BUILD_DIR}")
        string(FIND "${text}" "${forbidden_root}" reference_position)
        if(NOT reference_position EQUAL -1)
            message(FATAL_ERROR
                "${context} references the source or build tree ${forbidden_root}:\n${text}")
        endif()
    endforeach()
endfunction()

# DESTDIR staging must place every file below DESTDIR.
set(destdir "${work_directory}/destdir")
set(destdir_prefix "/opt/simp-install-test")
run_checked("DESTDIR install" "${CMAKE_COMMAND}" -E env "DESTDIR=${destdir}"
    "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${destdir_prefix}")
require_installed_layout("${destdir}${destdir_prefix}")
if(EXISTS "${destdir_prefix}")
    message(FATAL_ERROR "DESTDIR install wrote outside DESTDIR: ${destdir_prefix}")
endif()

set(prefix "${work_directory}/prefix")
run_checked("install" "${CMAKE_COMMAND}" -E env --unset=DESTDIR
    "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${prefix}")
require_installed_layout("${prefix}")

# Relocate the installed tree to prove nothing depends on the install prefix.
set(relocated_prefix "${work_directory}/relocated")
file(RENAME "${prefix}" "${relocated_prefix}")
set(compiler "${relocated_prefix}/${BINDIR}/simp")

set(project_directory "${work_directory}/project")
file(MAKE_DIRECTORY "${project_directory}/modules")
file(COPY "${MODULE_FIXTURE}/" DESTINATION "${project_directory}/modules")
get_filename_component(fixture_name "${FIXTURE}" NAME)
file(COPY "${FIXTURE}" DESTINATION "${project_directory}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env ${clean_environment} "${compiler}" --print-paths
    WORKING_DIRECTORY "${project_directory}"
    RESULT_VARIABLE print_result OUTPUT_VARIABLE print_output ERROR_VARIABLE print_error)
if(NOT print_result EQUAL 0)
    message(FATAL_ERROR "--print-paths failed (${print_result}):\n${print_error}")
endif()
string(REGEX REPLACE "([][+.*()^$?|\\\\{}])" "\\\\\\1" relocated_pattern "${relocated_prefix}")
if(NOT print_output MATCHES "prefix: ${relocated_pattern} \\(executable-relative\\)")
    message(FATAL_ERROR "Installed compiler did not derive its prefix:\n${print_output}")
endif()
if(print_output MATCHES
   "(runtime directory|runtime library|include directory|builtin directory|builtin source|standard modules): [^\n]*\\[not found\\]")
    message(FATAL_ERROR "Installed resources were not found:\n${print_output}")
endif()
string(FIND "${print_output}"
    "builtin directory: ${relocated_prefix}/${DATADIR}/simp/builtin (executable-relative)"
    builtin_directory_position)
string(FIND "${print_output}"
    "builtin source: ${relocated_prefix}/${DATADIR}/simp/builtin/String.simp"
    builtin_source_position)
if(builtin_directory_position EQUAL -1 OR builtin_source_position EQUAL -1)
    message(FATAL_ERROR "Installed builtin paths were not reported:\n${print_output}")
endif()
require_no_tree_references("${print_output}" "--print-paths")

# Compile the facade in isolation: no private headers are present on this path.
file(MAKE_DIRECTORY "${project_directory}/public/simp")
file(COPY "${relocated_prefix}/${INCLUDEDIR}/simp/Stdlib.h"
    DESTINATION "${project_directory}/public/simp")
file(COPY "${SOURCE_DIR}/tests/functional/cli/inline_stdlib.c"
    DESTINATION "${project_directory}")
set(instrumentation_flags)
if(COVERAGE)
    list(APPEND instrumentation_flags -fprofile-instr-generate -fcoverage-mapping)
endif()
if(SANITIZER_LIST)
    list(APPEND instrumentation_flags "-fsanitize=${SANITIZER_LIST}")
endif()
run_checked("standalone public C header"
    "${CLANG}" ${instrumentation_flags} -std=c11 -Wall -Wextra -Werror
    "-I${project_directory}/public" "${project_directory}/inline_stdlib.c"
    "${relocated_prefix}/${LIBDIR}/simp/${RUNTIME_LIBRARY_NAME}"
    -lm -pthread -o "${project_directory}/public-api")
run_checked("public C API executable" "${project_directory}/public-api")

set(executable "${project_directory}/installed-program")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env ${clean_environment}
        "${compiler}" -vv "${fixture_name}" -o "${executable}"
    WORKING_DIRECTORY "${project_directory}"
    RESULT_VARIABLE compile_result OUTPUT_VARIABLE compile_stdout ERROR_VARIABLE compile_stderr)
if(NOT compile_result EQUAL 0)
    message(FATAL_ERROR "Installed compiler failed (${compile_result}):\n${compile_stderr}")
endif()
if(NOT compile_stderr MATCHES "\\[command\\] [^\n]*${relocated_pattern}/[^\n]*/simp/${RUNTIME_LIBRARY_NAME}")
    message(FATAL_ERROR "Installed compiler did not link the installed runtime:\n${compile_stderr}")
endif()
require_no_tree_references("${compile_stdout}${compile_stderr}" "Installed compilation")

execute_process(COMMAND "${executable}"
    RESULT_VARIABLE run_result OUTPUT_VARIABLE program_output ERROR_VARIABLE run_stderr)
file(READ "${EXPECTED_OUTPUT_FILE}" expected_output)
if(NOT run_result EQUAL 0 OR NOT program_output STREQUAL expected_output)
    message(FATAL_ERROR
        "Installed program returned ${run_result} with '${program_output}':\n${run_stderr}")
endif()

# Initialize a project using relocated standard modules, keeping String out of
# the package allowlist while exercising its automatic availability/inheritance.
set(string_project "${work_directory}/string-project")
run_checked("relocated simpkg init" "${CMAKE_COMMAND}" -E env ${clean_environment}
    "${relocated_prefix}/${BINDIR}/simpkg" init "${string_project}")
if(NOT EXISTS "${string_project}/simpkg.toml"
   OR EXISTS "${string_project}/modules/modules.toml")
    message(FATAL_ERROR "Initialized project did not use the manifest/lock workflow")
endif()
file(READ "${string_project}/simpkg.lock" lock)
if(NOT lock MATCHES "^schema = 1\n"
   OR NOT lock MATCHES "\n\\[modules\\]\n"
   OR NOT lock MATCHES "\nmath = \\[\"0.1.0\"\\]"
   OR NOT lock MATCHES "\nsystem = \\[\"0.1.0\"\\]"
   OR lock MATCHES "\nString[ \t]*="
   OR lock MATCHES "\n\\[packages\\.String\\]")
    message(FATAL_ERROR "Initialized lock did not lock standard modules without String:\n${lock}")
endif()
file(COPY "${SOURCE_DIR}/tests/functional/positive/positive_string_class.simp"
    DESTINATION "${string_project}")
set(string_source "${string_project}/positive_string_class.simp")
set(string_executable "${string_project}/string-program")
run_checked("installed String compilation" "${CMAKE_COMMAND}" -E env ${clean_environment}
    "${compiler}" "${string_source}" -o "${string_executable}")
run_checked("installed String executable" "${string_executable}")
file(READ "${SOURCE_DIR}/tests/cases/simp_compile_and_run_string_class.stdout"
    expected_string_output)
if(NOT last_output STREQUAL expected_string_output)
    message(FATAL_ERROR "Installed String output differs:\n${last_output}")
endif()

# Compile and run the asynchronous process consumer against only the relocated
# installation's public process package and runtime library.
set(process_project "${work_directory}/process-project")
file(MAKE_DIRECTORY "${process_project}")
file(COPY "${SOURCE_DIR}/tests/functional/positive/positive_stdlib_async_process.simp"
    DESTINATION "${process_project}")
set(process_executable "${process_project}/process-consumer")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env ${clean_environment}
        "${compiler}" -vv positive_stdlib_async_process.simp -o "${process_executable}"
    WORKING_DIRECTORY "${process_project}"
    RESULT_VARIABLE process_compile_result OUTPUT_VARIABLE process_compile_stdout
    ERROR_VARIABLE process_compile_stderr)
if(NOT process_compile_result EQUAL 0)
    message(FATAL_ERROR
        "Installed process consumer failed to compile:\n${process_compile_stdout}${process_compile_stderr}")
endif()
require_no_tree_references("${process_compile_stdout}${process_compile_stderr}"
    "Installed process compilation")
execute_process(COMMAND "${process_executable}" TIMEOUT 60
    RESULT_VARIABLE process_run_result OUTPUT_VARIABLE process_output
    ERROR_VARIABLE process_run_stderr)
file(READ "${SOURCE_DIR}/tests/cases/simp_stdlib_async_process.stdout" expected_process_output)
if(NOT process_run_result EQUAL 0 OR NOT process_output STREQUAL expected_process_output)
    message(FATAL_ERROR
        "Installed process consumer returned ${process_run_result} with '${process_output}':\n${process_run_stderr}")
endif()

# Resolve the installed SQL/SQLite packages into a fresh project lock, then
# compile and execute a consumer using only the relocated installation.
set(sqlite_project "${work_directory}/sqlite-project")
file(MAKE_DIRECTORY "${sqlite_project}")
run_checked("installed SQLite simpkg init" "${CMAKE_COMMAND}" -E env ${clean_environment}
    "${relocated_prefix}/${BINDIR}/simpkg" init "${sqlite_project}")
set(sqlite_source
    "${SOURCE_DIR}/tests/functional/positive/positive_installed_sqlite_consumer.simp")
get_filename_component(sqlite_fixture_name "${sqlite_source}" NAME)
file(COPY "${sqlite_source}" DESTINATION "${sqlite_project}")
run_checked("installed SQLite simpkg install" "${CMAKE_COMMAND}" -E chdir "${sqlite_project}"
    "${CMAKE_COMMAND}" -E env ${clean_environment}
    "${relocated_prefix}/${BINDIR}/simpkg" install --yes)
file(READ "${sqlite_project}/simpkg.lock" sqlite_lock)
if(NOT sqlite_lock MATCHES "(^|\n)sqlite = \\[\"0\\.1\\.0\"\\]"
   OR NOT sqlite_lock MATCHES "(^|\n)sql = \\[\"0\\.1\\.0\"\\]"
   OR NOT sqlite_lock MATCHES "dependencies = \\[\"sql=0\\.1\\.0\"\\]")
    message(FATAL_ERROR "Installed SQLite project lock is missing its package graph:\n${sqlite_lock}")
endif()
set(sqlite_executable "${sqlite_project}/sqlite-consumer")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env ${clean_environment}
        "${compiler}" -vv "${sqlite_fixture_name}" -o "${sqlite_executable}"
    WORKING_DIRECTORY "${sqlite_project}"
    RESULT_VARIABLE sqlite_compile_result OUTPUT_VARIABLE sqlite_compile_stdout
    ERROR_VARIABLE sqlite_compile_stderr)
if(NOT sqlite_compile_result EQUAL 0)
    message(FATAL_ERROR
        "Installed SQLite consumer failed to compile:\n${sqlite_compile_stdout}${sqlite_compile_stderr}")
endif()
require_no_tree_references("${sqlite_compile_stdout}${sqlite_compile_stderr}"
    "Installed SQLite compilation")
execute_process(COMMAND "${sqlite_executable}"
    RESULT_VARIABLE sqlite_run_result OUTPUT_VARIABLE sqlite_output
    ERROR_VARIABLE sqlite_run_stderr)
file(STRINGS "${sqlite_source}" sqlite_assertions REGEX "^[ \t]*print\\(")
list(LENGTH sqlite_assertions sqlite_assertion_count)
string(REPEAT "true" ${sqlite_assertion_count} sqlite_expected_output)
if(NOT sqlite_run_result EQUAL 0 OR NOT sqlite_output STREQUAL sqlite_expected_output)
    message(FATAL_ERROR
        "Installed SQLite consumer returned ${sqlite_run_result} with '${sqlite_output}':\n${sqlite_run_stderr}")
endif()

# A program with no SQLite import must not receive the SQLite package's native
# link flags or a runtime dependency on libsqlite3.
set(plain_project "${work_directory}/no-sqlite-project")
file(MAKE_DIRECTORY "${plain_project}")
set(plain_source "${plain_project}/no-sqlite.simp")
file(WRITE "${plain_source}" "start {\n    print(true)\n}\n")
set(plain_executable "${plain_project}/no-sqlite")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env ${clean_environment}
        "${compiler}" -vv "${plain_source}" -o "${plain_executable}"
    WORKING_DIRECTORY "${plain_project}"
    RESULT_VARIABLE plain_compile_result OUTPUT_VARIABLE plain_compile_stdout
    ERROR_VARIABLE plain_compile_stderr)
if(NOT plain_compile_result EQUAL 0)
    message(FATAL_ERROR
        "No-SQLite application failed to compile:\n${plain_compile_stdout}${plain_compile_stderr}")
endif()
set(plain_link_output "${plain_compile_stdout}${plain_compile_stderr}")
if(plain_link_output MATCHES "(-lsqlite3|libsimp_sqlite)")
    message(FATAL_ERROR "No-SQLite application received SQLite link flags:\n${plain_link_output}")
endif()
run_checked("No-SQLite application" "${plain_executable}")
if(NOT last_output STREQUAL "true")
    message(FATAL_ERROR "No-SQLite application output differs: '${last_output}'")
endif()
find_program(LDD_EXECUTABLE ldd)
if(LDD_EXECUTABLE)
    execute_process(COMMAND "${LDD_EXECUTABLE}" "${plain_executable}"
        RESULT_VARIABLE ldd_result OUTPUT_VARIABLE ldd_output ERROR_VARIABLE ldd_stderr)
    if(NOT ldd_result EQUAL 0)
        message(FATAL_ERROR "Could not inspect no-SQLite runtime dependencies:\n${ldd_stderr}")
    endif()
    if(ldd_output MATCHES "libsqlite3")
        message(FATAL_ERROR "No-SQLite application has a SQLite runtime dependency:\n${ldd_output}")
    endif()
endif()

# Remove the default resource so compilation can succeed only via the override.
set(builtin_override "${work_directory}/custom-builtin")
file(RENAME "${relocated_prefix}/${DATADIR}/simp/builtin" "${builtin_override}")
run_checked("builtin override paths" "${CMAKE_COMMAND}" -E env ${clean_environment}
    "SIMP_BUILTIN_DIR=${builtin_override}" "${compiler}" --print-paths)
string(FIND "${last_output}"
    "builtin directory: ${builtin_override} (SIMP_BUILTIN_DIR)" override_position)
if(override_position EQUAL -1)
    message(FATAL_ERROR "Builtin override was not reported:\n${last_output}")
endif()
require_no_tree_references("${last_output}" "Builtin override paths")
run_checked("overridden String compilation" "${CMAKE_COMMAND}" -E env ${clean_environment}
    "SIMP_BUILTIN_DIR=${builtin_override}" "${compiler}" "${string_source}"
    -o "${string_executable}")
run_checked("overridden String executable" "${string_executable}")
if(NOT last_output STREQUAL expected_string_output)
    message(FATAL_ERROR "Overridden String output differs:\n${last_output}")
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env ${clean_environment}
        "SIMP_BUILTIN_DIR=${work_directory}/missing-builtin"
        "${compiler}" --check-only "${string_source}"
    RESULT_VARIABLE missing_result OUTPUT_VARIABLE missing_stdout
    ERROR_VARIABLE missing_stderr)
if(missing_result EQUAL 0 OR NOT missing_stderr MATCHES
   "cannot load String builtin[^\n]*; set SIMP_BUILTIN_DIR or SIMP_HOME")
    message(FATAL_ERROR
        "Missing builtin override did not fail explicitly:\n${missing_stdout}${missing_stderr}")
endif()
file(REMOVE_RECURSE "${work_directory}")
