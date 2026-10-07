cmake_minimum_required(VERSION 3.16)
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(simp_tests LANGUAGES C CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)
    enable_testing()
    include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Instrumentation.cmake")
    include(GNUInstallDirs)
    set(SIMP_STAGE_PREFIX "${CMAKE_CURRENT_BINARY_DIR}")
    set(SIMP_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
    set(SIMP_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
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
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../src"
                     "${CMAKE_CURRENT_BINARY_DIR}/src")
endif()

file(GLOB unit_case_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*_cases.cpp")
add_executable(simp_tests test_driver.cpp test_cases.cpp ${unit_case_sources})
target_link_libraries(simp_tests PRIVATE simp_frontend)
set_target_properties(simp_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${SIMP_RUNTIME_OUTPUT_DIRECTORY}"
)
target_compile_definitions(simp_tests PRIVATE
    SIMP_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/functional"
    SIMP_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(simp_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME simp_unit_and_functional COMMAND simp_tests)
add_executable(simp_command_line_tests command_line_tests.cpp)
target_link_libraries(simp_command_line_tests PRIVATE simp_frontend)
set_target_properties(simp_command_line_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${SIMP_RUNTIME_OUTPUT_DIRECTORY}"
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(simp_command_line_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME simp_command_line_parser COMMAND simp_command_line_tests)

add_executable(simp_gc_runtime_tests test_gc_runtime.c)
target_link_libraries(simp_gc_runtime_tests PRIVATE simp_runtime)
set_target_properties(simp_gc_runtime_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${SIMP_RUNTIME_OUTPUT_DIRECTORY}"
)
add_test(NAME simp_gc_runtime_lifecycle COMMAND simp_gc_runtime_tests)
add_test(NAME simp_callbacks
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_callbacks.py"
        --compiler "$<TARGET_FILE:simp>" --clang "${SIMP_CLANG_EXECUTABLE}"
        --include "${CMAKE_CURRENT_SOURCE_DIR}/../include"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/functional/cli/callbacks/native.c"
        --work "${CMAKE_CURRENT_BINARY_DIR}/callbacks"
        "--sanitize=${SIMP_SANITIZER_LIST}")
if(SIMP_GTK)
    find_program(SIMP_XVFB_EXECUTABLE NAMES Xvfb)
    if(NOT SIMP_XVFB_EXECUTABLE)
        message(FATAL_ERROR "SIMP_GTK tests require Xvfb; install it or set SIMP_XVFB_EXECUTABLE")
    endif()
    find_program(SIMP_DBUS_DAEMON_EXECUTABLE NAMES dbus-daemon)
    if(NOT SIMP_DBUS_DAEMON_EXECUTABLE)
        message(FATAL_ERROR "SIMP_GTK tests require dbus-daemon")
    endif()
    set(gtk_program_arguments --gtk --xvfb "${SIMP_XVFB_EXECUTABLE}")
    set(gtk_install_arguments)
    if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        list(APPEND gtk_install_arguments --build "${CMAKE_BINARY_DIR}")
    endif()
    add_test(NAME simp_gtk
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_gtk.py"
            --compiler "$<TARGET_FILE:simp>" --clang "${SIMP_CLANG_EXECUTABLE}"
            --include "${CMAKE_CURRENT_SOURCE_DIR}/../include"
            --native "${CMAKE_CURRENT_SOURCE_DIR}/../stdlib/gtk/0.1.0/native"
            --fixture "${CMAKE_CURRENT_SOURCE_DIR}/functional/cli/gtk/native.c"
            --work "${CMAKE_CURRENT_BINARY_DIR}/gtk"
            --xvfb "${SIMP_XVFB_EXECUTABLE}"
            "--sanitize=${SIMP_SANITIZER_LIST}" ${gtk_install_arguments})
    set_tests_properties(simp_gtk PROPERTIES LABELS gtk TIMEOUT 180)
endif()
add_test(NAME simpkg_cli
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_simpkg.py"
        --simpkg "${SIMP_RUNTIME_OUTPUT_DIRECTORY}/simpkg"
        --compiler "$<TARGET_FILE:simp>"
)

add_test(NAME simp_multi_input_compiler
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DCLANG=${SIMP_CLANG_EXECUTABLE}"
        "-DAR=${CMAKE_AR}"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        "-DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/functional/cli/multi"
        "-DCASE=combined"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/MultiInputCompilerTest.cmake"
)
foreach(multi_case IN ITEMS object mixed library module invalid_start
        invalid_missing_start invalid_duplicate_symbol invalid_overwrite
        invalid_compile_object invalid_emit_object invalid_compile_library
        invalid_check_library invalid_unsupported)
    add_test(NAME "simp_multi_input_${multi_case}"
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DCLANG=${SIMP_CLANG_EXECUTABLE}"
            "-DAR=${CMAKE_AR}"
            "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            "-DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/functional/cli/multi"
            "-DCASE=${multi_case}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/MultiInputCompilerTest.cmake"
    )
endforeach()



add_test(NAME simp_default_output_uses_working_directory
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive/positive_integer_output.simp"
        "-DWORKING_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/default_output_working_directory"
        "-DOUTPUT_NAME=positive_integer_output"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/default_output_working_directory/explicit.ll"
        "-DEXPECTED_OUTPUT=42"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DefaultOutputDirectoryTest.cmake"
)
add_test(NAME simp_cli_accepts_program
    COMMAND $<TARGET_FILE:simp> "${CMAKE_CURRENT_SOURCE_DIR}/functional/positive/positive_control_flow.simp" --check-only -t ast
)
add_test(NAME simp_cli_include_search_path
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        "-DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/functional/cli/include"
        "-DCASE=ordered"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/IncludeSearchPathTest.cmake"
)
foreach(include_case IN ITEMS missing relative)
    add_test(NAME "simp_cli_include_${include_case}"
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            "-DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/functional/cli/include"
            "-DCASE=${include_case}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/IncludeSearchPathTest.cmake"
    )
endforeach()
add_test(NAME simp_cli_help COMMAND $<TARGET_FILE:simp> --help)
add_test(NAME simp_cli_version COMMAND $<TARGET_FILE:simp> -V)
add_test(NAME simp_cli_rejects_multiple_inputs
    COMMAND $<TARGET_FILE:simp>
        "${CMAKE_CURRENT_SOURCE_DIR}/functional/cli/multi/main.simp"
        "${CMAKE_CURRENT_SOURCE_DIR}/functional/cli/multi/second-start.simp"
)
set_tests_properties(simp_cli_rejects_multiple_inputs PROPERTIES WILL_FAIL TRUE)
add_test(NAME simp_cli_rejects_conditional_else
    COMMAND $<TARGET_FILE:simp> "${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_conditional_else.simp"
)
set_tests_properties(simp_cli_rejects_conditional_else PROPERTIES WILL_FAIL TRUE)



# Each integration case owns its fixture, expectation, and optional flags.
file(GLOB case_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cases/*.cmake")
foreach(case_file IN LISTS case_files)
    unset(CASE_NAME)
    unset(CASE_FIXTURE)
    unset(CASE_EXPECTED_OUTPUT_FILE)
    unset(CASE_EXPECTED_OUTPUT)
    unset(CASE_EXPECTED_DIAGNOSTIC)
    unset(CASE_EXPECT_RUNTIME_FAILURE)
    unset(CASE_EXPECT_RUNTIME_DIAGNOSTIC)
    unset(CASE_EXPECT_RUNTIME_FRAMES)
    unset(CASE_EXPECT_EXIT_CODE)
    unset(CASE_EXPECT_EMPTY_OUTPUT)
    unset(CASE_EXPECT_EMPTY_RUNTIME_STDERR)
    unset(CASE_EXPECT_RUNTIME_STDERR_FILE)
    unset(CASE_EXPECT_WARNING)
    unset(CASE_REQUIRE_GC_ROOTS)
    unset(CASE_REQUIRE_VIRTUAL_DISPATCH)
    unset(CASE_MODULE_PACKAGES)
    unset(CASE_MODULE_ROOT)
    unset(CASE_MODULE_ROOT_SOURCE)
    unset(CASE_MODULE_ROOT_MISSING)
    unset(CASE_MODULE_DECOY)
    unset(CASE_PACKAGE_NATIVE_SOURCE)
    unset(CASE_PACKAGE_NATIVE_LIBRARY)
    unset(CASE_ARGUMENTS)
    unset(CASE_RUN_ARGUMENTS)
    unset(CASE_STDIN_FILE)
    unset(CASE_NO_SOURCE)
    unset(CASE_NO_RUN)
    unset(CASE_EXPECT_COMPILE_OUTPUT)
    unset(CASE_REJECT_COMPILE_OUTPUT)
    unset(CASE_COMPILE_ONLY)
    unset(CASE_DEBUG_INFO)
    unset(CASE_DEBUG_LINE)
    unset(CASE_DEBUG_LOCAL)
    include("${case_file}")
    if(NOT DEFINED CASE_NAME OR NOT DEFINED CASE_FIXTURE)
        message(FATAL_ERROR "Incomplete integration case: ${case_file}")
    endif()
    if(CASE_FIXTURE MATCHES "^positive_")
        set(fixture_directory positive)
    elseif(CASE_FIXTURE MATCHES "^negative_")
        set(fixture_directory negative)
    else()
        message(FATAL_ERROR "Invalid integration fixture: ${case_file}")
    endif()
    set(source "${CMAKE_CURRENT_SOURCE_DIR}/functional/${fixture_directory}/${CASE_FIXTURE}")
    if(NOT EXISTS "${source}")
        message(FATAL_ERROR "Missing integration fixture: ${source}")
    endif()
    set(case_arguments
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DCLANG=${SIMP_CLANG_EXECUTABLE}"
        "-DAR=${CMAKE_AR}"
        "-DSOURCE=${source}"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${CASE_NAME}"
    )
    if(CASE_MODULE_PACKAGES)
        list(APPEND case_arguments
            "-DPACKAGE_FIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/functional/modules/packages")
    endif()
    foreach(path_setting IN ITEMS MODULE_ROOT MODULE_DECOY)
        if(DEFINED CASE_${path_setting})
            get_filename_component(case_path "${CASE_${path_setting}}" ABSOLUTE
                BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/functional/modules")
            if(NOT IS_DIRECTORY "${case_path}")
                message(FATAL_ERROR "Missing module fixture for ${case_file}: ${case_path}")
            endif()
            list(APPEND case_arguments "-D${path_setting}=${case_path}")
        endif()
    endforeach()
    if(DEFINED CASE_STDIN_FILE)
        get_filename_component(stdin_file "${CASE_STDIN_FILE}" ABSOLUTE
            BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/functional")
        if(NOT EXISTS "${stdin_file}")
            message(FATAL_ERROR "Missing stdin fixture for ${case_file}: ${stdin_file}")
        endif()
        list(APPEND case_arguments "-DSTDIN_FILE=${stdin_file}")
    endif()
    # List-valued settings keep their separators when forwarded to the driver.
    foreach(list_setting IN ITEMS ARGUMENTS RUN_ARGUMENTS EXPECTED_OUTPUT
            EXPECT_COMPILE_OUTPUT REJECT_COMPILE_OUTPUT)
        if(DEFINED CASE_${list_setting})
            string(REPLACE ";" "\\;" escaped_list "${CASE_${list_setting}}")
            list(APPEND case_arguments "-D${list_setting}=${escaped_list}")
        endif()
    endforeach()
    foreach(flag_setting IN ITEMS MODULE_ROOT_SOURCE MODULE_ROOT_MISSING MODULE_PACKAGES
            NO_SOURCE NO_RUN)
        if(DEFINED CASE_${flag_setting})
            list(APPEND case_arguments "-D${flag_setting}=${CASE_${flag_setting}}")
        endif()
    endforeach()
    if(DEFINED CASE_EXPECT_EXIT_CODE)
        list(APPEND case_arguments "-DEXPECT_EXIT_CODE=${CASE_EXPECT_EXIT_CODE}")
    endif()
    if(DEFINED CASE_PACKAGE_NATIVE_SOURCE)
        list(APPEND case_arguments "-DPACKAGE_NATIVE_SOURCE=${CASE_PACKAGE_NATIVE_SOURCE}")
    endif()
    if(DEFINED CASE_PACKAGE_NATIVE_LIBRARY)
        list(APPEND case_arguments "-DPACKAGE_NATIVE_LIBRARY=${CASE_PACKAGE_NATIVE_LIBRARY}")
    endif()
    if(CASE_COMPILE_ONLY)
        list(APPEND case_arguments "-DCOMPILE_ONLY=${CASE_COMPILE_ONLY}")
    endif()
    if(CASE_DEBUG_INFO)
        list(APPEND case_arguments "-DDEBUG_INFO=${CASE_DEBUG_INFO}")
        if(DEFINED CASE_DEBUG_LINE)
            list(APPEND case_arguments "-DDEBUG_LINE=${CASE_DEBUG_LINE}")
        endif()
        if(DEFINED CASE_DEBUG_LOCAL)
            list(APPEND case_arguments "-DDEBUG_LOCAL=${CASE_DEBUG_LOCAL}")
        endif()
    endif()
    if(DEFINED CASE_EXPECTED_DIAGNOSTIC)
        string(REPLACE ";" "\\;" escaped_diagnostic "${CASE_EXPECTED_DIAGNOSTIC}")
        list(APPEND case_arguments "-DEXPECTED_DIAGNOSTIC=${escaped_diagnostic}")
    elseif(NOT CASE_NO_RUN)
        if(NOT DEFINED CASE_EXPECTED_OUTPUT_FILE AND NOT DEFINED CASE_EXPECTED_OUTPUT AND
           NOT CASE_EXPECT_EMPTY_OUTPUT)
            message(FATAL_ERROR "Missing output expectation: ${case_file}")
        endif()
        list(APPEND case_arguments
            "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${CASE_NAME}.ll")
        if(CASE_EXPECT_EMPTY_OUTPUT)
            list(APPEND case_arguments "-DEXPECT_EMPTY_OUTPUT=ON")
        elseif(NOT DEFINED CASE_EXPECTED_OUTPUT)
            list(APPEND case_arguments "-DEXPECTED_OUTPUT_FILE=${CASE_EXPECTED_OUTPUT_FILE}")
        endif()
        foreach(flag IN ITEMS EXPECT_RUNTIME_FAILURE EXPECT_RUNTIME_DIAGNOSTIC
                EXPECT_RUNTIME_FRAMES EXPECT_EMPTY_RUNTIME_STDERR
                EXPECT_RUNTIME_STDERR_FILE EXPECT_WARNING
                REQUIRE_GC_ROOTS REQUIRE_VIRTUAL_DISPATCH)
            if(DEFINED CASE_${flag})
                if(flag STREQUAL "EXPECT_RUNTIME_STDERR_FILE")
                    get_filename_component(stderr_file "${CASE_EXPECT_RUNTIME_STDERR_FILE}" ABSOLUTE
                        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cases")
                    list(APPEND case_arguments "-D${flag}=${stderr_file}")
                else()
                    list(APPEND case_arguments "-D${flag}=${CASE_${flag}}")
                endif()
            endif()
        endforeach()
    endif()
    add_test(NAME "${CASE_NAME}"
        COMMAND "${CMAKE_COMMAND}" ${case_arguments}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake")
endforeach()

add_test(NAME simp_include_max_depth_flag
    COMMAND "$<TARGET_FILE:simp>" --check-only --max-include-depth 17
        "${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_include_max_depth.simp"
)

# Install cases need the complete project build, so standalone tests builds skip them.
if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    file(GLOB install_case_files CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/install/*.cmake")
    foreach(install_case_file IN LISTS install_case_files)
        unset(INSTALL_CASE_NAME)
        unset(INSTALL_CASE_FIXTURE)
        unset(INSTALL_CASE_MODULE_FIXTURE)
        unset(INSTALL_CASE_EXPECTED_OUTPUT_FILE)
        include("${install_case_file}")
        foreach(required IN ITEMS NAME FIXTURE MODULE_FIXTURE EXPECTED_OUTPUT_FILE)
            if(NOT DEFINED INSTALL_CASE_${required})
                message(FATAL_ERROR
                    "Incomplete install case ${install_case_file}: INSTALL_CASE_${required}")
            endif()
        endforeach()
        add_test(NAME "${INSTALL_CASE_NAME}"
            COMMAND "${CMAKE_COMMAND}"
                "-DBUILD_DIR=${CMAKE_BINARY_DIR}"
                "-DSOURCE_DIR=${CMAKE_SOURCE_DIR}"
                "-DFIXTURE=${INSTALL_CASE_FIXTURE}"
                "-DMODULE_FIXTURE=${INSTALL_CASE_MODULE_FIXTURE}"
                "-DEXPECTED_OUTPUT_FILE=${INSTALL_CASE_EXPECTED_OUTPUT_FILE}"
                "-DBINDIR=${CMAKE_INSTALL_BINDIR}"
                "-DLIBDIR=${CMAKE_INSTALL_LIBDIR}"
                "-DINCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}"
                "-DDATADIR=${CMAKE_INSTALL_DATADIR}"
                "-DDOCDIR=${CMAKE_INSTALL_DOCDIR}"
                "-DMANDIR=${CMAKE_INSTALL_MANDIR}"
                "-DRUNTIME_LIBRARY_NAME=$<TARGET_FILE_NAME:simp_runtime>"
                "-DCLANG=${SIMP_CLANG_EXECUTABLE}"
                "-DSANITIZER_LIST=${SIMP_SANITIZER_LIST}"
                "-DCOVERAGE=${SIMP_COVERAGE}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/InstallLayoutTest.cmake"
        )
    endforeach()
endif()

set(example_directory "${CMAKE_CURRENT_SOURCE_DIR}/../examples")
file(GLOB_RECURSE example_sources CONFIGURE_DEPENDS "${example_directory}/*.simp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/examples.json")
execute_process(COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_examples.py"
    --examples "${example_directory}"
    --manifest "${CMAKE_CURRENT_SOURCE_DIR}/examples.json" --list ${gtk_program_arguments}
    OUTPUT_VARIABLE example_cases OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE example_result ERROR_VARIABLE example_error)
if(NOT example_result EQUAL 0)
    message(FATAL_ERROR "Example inventory check failed: ${example_error}")
endif()
foreach(example_case IN LISTS example_cases)
    string(REPLACE "/" "_" example_name "${example_case}")
    add_test(NAME "simp_example_${example_name}"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_examples.py"
        --compiler "$<TARGET_FILE:simp>" --examples "${example_directory}"
        --manifest "${CMAKE_CURRENT_SOURCE_DIR}/examples.json"
        --work "${CMAKE_CURRENT_BINARY_DIR}/examples" --case "${example_case}"
        ${gtk_program_arguments})
    set_tests_properties("simp_example_${example_name}" PROPERTIES LABELS examples TIMEOUT 60)
endforeach()

add_test(NAME simp_documentation_programs
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_doc_programs.py"
    --compiler "$<TARGET_FILE:simp>" --doc "${CMAKE_CURRENT_SOURCE_DIR}/../doc"
    --fixtures "${CMAKE_CURRENT_SOURCE_DIR}/doc_fixtures"
    --work "${CMAKE_CURRENT_BINARY_DIR}/doc-programs" ${gtk_program_arguments})
set_tests_properties(simp_documentation_programs PROPERTIES LABELS documentation TIMEOUT 180)

if(SIMP_FUZZ)
    add_executable(simp_fuzz_frontend fuzz_frontend.cpp)
    target_link_libraries(simp_fuzz_frontend PRIVATE simp_frontend)
    target_compile_options(simp_fuzz_frontend PRIVATE -fsanitize=fuzzer)
    target_link_options(simp_fuzz_frontend PRIVATE -fsanitize=fuzzer)
    set_target_properties(simp_fuzz_frontend PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${SIMP_RUNTIME_OUTPUT_DIRECTORY}")
    add_test(NAME simp_fuzz_smoke
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/fuzz.py"
            --fuzzer "$<TARGET_FILE:simp_fuzz_frontend>"
            --fixtures "${CMAKE_CURRENT_SOURCE_DIR}/functional"
            --work "${CMAKE_CURRENT_BINARY_DIR}/fuzz-smoke" --seconds 10 --runs 2000)
    set_tests_properties(simp_fuzz_smoke PROPERTIES LABELS fuzz TIMEOUT 75)
endif()

set(simp_test_environment)
if(SIMP_SANITIZE AND NOT SIMP_SANITIZE STREQUAL "OFF")
    list(APPEND simp_test_environment
        "ASAN_OPTIONS=detect_leaks=0:halt_on_error=1"
        "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
endif()
if(SIMP_COVERAGE)
    find_program(SIMP_LLVM_PROFDATA NAMES llvm-profdata)
    find_program(SIMP_LLVM_COV NAMES llvm-cov)
    if(NOT SIMP_LLVM_PROFDATA OR NOT SIMP_LLVM_COV)
        message(FATAL_ERROR "SIMP_COVERAGE requires llvm-profdata and llvm-cov")
    endif()
    set(profile_directory "${CMAKE_BINARY_DIR}/coverage/profiles")
    file(MAKE_DIRECTORY "${profile_directory}")
    list(APPEND simp_test_environment
        "LLVM_PROFILE_FILE=${profile_directory}/%p-%m.profraw")
    add_custom_target(coverage
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/coverage.py"
            --build "${CMAKE_BINARY_DIR}" --source "${CMAKE_CURRENT_SOURCE_DIR}/../src"
            --profdata "${SIMP_LLVM_PROFDATA}" --cov "${SIMP_LLVM_COV}"
            --binary "$<TARGET_FILE:simp>" --binary "$<TARGET_FILE:simp_tests>"
            --binary "$<TARGET_FILE:simp_command_line_tests>"
            --binary "$<TARGET_FILE:simp_gc_runtime_tests>" --run-tests
        DEPENDS simp simp_tests simp_command_line_tests simp_gc_runtime_tests
        USES_TERMINAL VERBATIM)
endif()
if(simp_test_environment)
    get_property(simp_all_tests DIRECTORY PROPERTY TESTS)
    set_tests_properties(${simp_all_tests} PROPERTIES ENVIRONMENT "${simp_test_environment}")
endif()
if(SIMP_SANITIZE AND NOT SIMP_SANITIZE STREQUAL "OFF")
    set(frontend_test_environment "${simp_test_environment}")
    list(REMOVE_ITEM frontend_test_environment "ASAN_OPTIONS=detect_leaks=0:halt_on_error=1")
    list(APPEND frontend_test_environment "ASAN_OPTIONS=detect_leaks=1:halt_on_error=1")
    set_tests_properties(simp_unit_and_functional simp_command_line_parser PROPERTIES
        ENVIRONMENT "${frontend_test_environment}")
endif()
