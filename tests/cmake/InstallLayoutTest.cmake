# Installs the configured build into a temporary prefix, then compiles and runs
# a package-importing fixture with only the installed compiler and resources.
foreach(required IN ITEMS BUILD_DIR SOURCE_DIR FIXTURE MODULE_FIXTURE EXPECTED_OUTPUT_FILE
        BINDIR LIBDIR INCLUDEDIR DATADIR DOCDIR MANDIR RUNTIME_LIBRARY_NAME)
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
    --unset=SIMP_PRELUDE_DIR --unset=SIMP_STDLIB_MODULE_DIR --unset=SIMP_MODULE_DIR
    --unset=SIMP_PACKAGE_PATH --unset=SIMP_MODULE_REGISTRY --unset=DESTDIR)

function(run_checked description)
    execute_process(COMMAND ${ARGN}
        RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${description} failed (${result}):\n${stdout}${stderr}")
    endif()
    set(last_output "${stdout}${stderr}" PARENT_SCOPE)
endfunction()

function(require_installed_layout prefix)
    foreach(installed_file IN ITEMS
            "${BINDIR}/simp"
            "${LIBDIR}/simp/${RUNTIME_LIBRARY_NAME}"
            "${INCLUDEDIR}/simp/RuntimeGc.h"
            "${INCLUDEDIR}/simp/RuntimeThreads.h"
            "${INCLUDEDIR}/simp/RuntimeDemoShims.h"
            "${DATADIR}/simp/prelude/String.simp"
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
if(print_output MATCHES "\\[not found\\]\\)?\n(runtime|include|prelude)")
    message(FATAL_ERROR "Installed resources were not found:\n${print_output}")
endif()
require_no_tree_references("${print_output}" "--print-paths")

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
file(REMOVE_RECURSE "${work_directory}")
