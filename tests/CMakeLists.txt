cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(simp_tests LANGUAGES C CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)
    enable_testing()
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
        "-DEXPECTED_OUTPUT=42\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DefaultOutputDirectoryTest.cmake"
)
add_test(NAME simp_cli_accepts_program
    COMMAND $<TARGET_FILE:simp> "${CMAKE_CURRENT_SOURCE_DIR}/functional/positive/positive_control_flow.simp" --check-only --dump-ast
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
    unset(CASE_EXPECTED_DIAGNOSTIC)
    unset(CASE_EXPECT_RUNTIME_FAILURE)
    unset(CASE_EXPECT_RUNTIME_DIAGNOSTIC)
    unset(CASE_EXPECT_RUNTIME_FRAMES)
    unset(CASE_EXPECT_EMPTY_RUNTIME_STDERR)
    unset(CASE_EXPECT_WARNING)
    unset(CASE_REQUIRE_GC_ROOTS)
    unset(CASE_REQUIRE_VIRTUAL_DISPATCH)
    unset(CASE_MODULE_REGISTRY)
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
        "-DSOURCE=${source}"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${CASE_NAME}"
    )
    if(DEFINED CASE_EXPECTED_DIAGNOSTIC)
        string(REPLACE ";" "\\;" escaped_diagnostic "${CASE_EXPECTED_DIAGNOSTIC}")
        list(APPEND case_arguments "-DEXPECTED_DIAGNOSTIC=${escaped_diagnostic}")
    else()
        if(NOT DEFINED CASE_EXPECTED_OUTPUT_FILE)
            message(FATAL_ERROR "Missing output expectation: ${case_file}")
        endif()
        list(APPEND case_arguments
            "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${CASE_NAME}.ll"
            "-DEXPECTED_OUTPUT_FILE=${CASE_EXPECTED_OUTPUT_FILE}")
        foreach(flag IN ITEMS EXPECT_RUNTIME_FAILURE EXPECT_RUNTIME_DIAGNOSTIC
                EXPECT_RUNTIME_FRAMES EXPECT_EMPTY_RUNTIME_STDERR EXPECT_WARNING
                REQUIRE_GC_ROOTS REQUIRE_VIRTUAL_DISPATCH)
            if(DEFINED CASE_${flag})
                list(APPEND case_arguments "-D${flag}=${CASE_${flag}}")
            endif()
        endforeach()
    endif()
    add_test(NAME "${CASE_NAME}"
        COMMAND "${CMAKE_COMMAND}" ${case_arguments}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake")
    if(CASE_MODULE_REGISTRY)
        set_tests_properties("${CASE_NAME}" PROPERTIES
            ENVIRONMENT "SIMP_MODULE_REGISTRY=${CMAKE_CURRENT_SOURCE_DIR}/functional/modules/simp-modules.tsv")
    endif()
endforeach()

add_test(NAME simp_include_max_depth_flag
    COMMAND "$<TARGET_FILE:simp>" --check-only --max-include-depth 17
        "${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_include_max_depth.simp"
)
