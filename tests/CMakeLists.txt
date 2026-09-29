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

add_executable(simp_tests test_driver.cpp)
target_link_libraries(simp_tests PRIVATE simp_frontend)
set_target_properties(simp_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${SIMP_RUNTIME_OUTPUT_DIRECTORY}"
)
target_compile_definitions(simp_tests PRIVATE
    SIMP_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/functional"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(simp_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME simp_unit_and_functional COMMAND simp_tests)
add_executable(simp_gc_runtime_tests test_gc_runtime.c)
target_link_libraries(simp_gc_runtime_tests PRIVATE simp_runtime)
set_target_properties(simp_gc_runtime_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${SIMP_RUNTIME_OUTPUT_DIRECTORY}"
)
add_test(NAME simp_gc_runtime_lifecycle COMMAND simp_gc_runtime_tests)
add_test(NAME simp_compile_and_run_typed_arrays
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_arrays.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_arrays"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_arrays.ll"
        "-DEXPECTED_OUTPUT=4\ntwenty\ntwenty-five\n25\n2\n25\n25\n99\n0\n0\n11\ntwo\n7\n8\n7\n11\n1\ntwo\n<object>\nnull\narray index out of bounds\narray slice bounds out of bounds\n"
        "-DREQUIRE_GC_ROOTS=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_non_integer_array_index
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_array_index_type.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_array_index_type"
        "-DEXPECTED_DIAGNOSTIC=array index and slice bounds must be int"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_heterogeneous_maps
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_maps.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_maps"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_maps.ll"
        "-DEXPECTED_OUTPUT=5\n7\n7\nseven\n11\n1\n2\n3\n1\n2\n3\n64\n9\n88\n77\nmap key not found\n"
        "-DREQUIRE_GC_ROOTS=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_collection_iteration
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_collection_iteration.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_collection_iteration"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_collection_iteration.ll"
        "-DEXPECTED_OUTPUT=3\n1\n0\n3\n22\nfirst\n10\ndynamic\n22\nthird\n30\n3\n4\n1\ntwo\n<object>\n<object>\n42\n100\n"
        "-DREQUIRE_GC_ROOTS=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_any_map_index_key
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_map_nonliteral_key.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_nonliteral_key"
        "-DEXPECTED_DIAGNOSTIC=map keys must have type string"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_numeric_map_index_key
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_map_numeric_key.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_numeric_key"
        "-DEXPECTED_DIAGNOSTIC=map keys must have type string"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_collection_followups
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_collection_followups.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_collection_followups"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_collection_followups.ll"
        "-DEXPECTED_OUTPUT=3\n2\n20\n1\n0\n1\n2\n4\n5\n30\n5\n5\nsecond\nthird\nfourth\n10\n20\n30\n200\n77\n"
        "-DREQUIRE_GC_ROOTS=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_uncaught_map_slice_bounds
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_map_slice_bounds_uncaught.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_slice_bounds_uncaught"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_slice_bounds_uncaught.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=uncaught runtime exception at .*negative_map_slice_bounds_uncaught.simp:3:[0-9]+: map slice bounds out of bounds"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_array_key_value_iteration
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_array_iteration_arity.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_array_iteration_arity"
        "-DEXPECTED_DIAGNOSTIC=array iteration accepts one value variable"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_nonstring_map_literal_key
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_map_literal_key.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_literal_key"
        "-DEXPECTED_DIAGNOSTIC=map keys must have type string"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_map_length_assignment
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_map_length_assignment.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_length_assignment"
        "-DEXPECTED_DIAGNOSTIC=map length is read-only"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_map_extraction_mismatch
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_map_extraction_mismatch.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_extraction_mismatch"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_extraction_mismatch.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=uncaught runtime exception at .*negative_map_extraction_mismatch.simp:3:19: 'any' value does not hold a map reference"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_uncaught_missing_map_key
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_map_missing_key.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_missing_key"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_missing_key.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=uncaught runtime exception at .*negative_map_missing_key.simp:3:17: map key not found"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_any_member_access
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_any_member_access.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_any_member_access"
        "-DEXPECTED_DIAGNOSTIC='any' has no members; assign it to a typed variable first to extract its value"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_uncaught_array_index_bounds
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_array_bounds_uncaught.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_array_bounds_uncaught"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_array_bounds_uncaught.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=uncaught runtime exception at .*negative_array_bounds_uncaught.simp:3:17: array index out of bounds"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_uncaught_array_slice_bounds
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_array_slice_bounds_uncaught.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_array_slice_bounds_uncaught"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_array_slice_bounds_uncaught.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=uncaught runtime exception at .*negative_array_slice_bounds_uncaught.simp:3:24: array slice bounds out of bounds"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_uncaught_any_extraction_mismatch
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_any_extraction_mismatch.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_any_extraction_mismatch"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_any_extraction_mismatch.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=uncaught runtime exception at .*negative_any_extraction_mismatch.simp:6:28: 'any' value does not hold the requested type"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_uncaught_any_class_mismatch
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_any_class_mismatch.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_any_class_mismatch"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_any_class_mismatch.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=uncaught runtime exception at .*negative_any_class_mismatch.simp:19:26: 'any' value holds a different class than the requested type"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_default_output_uses_working_directory
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_integer_output.simp"
        "-DWORKING_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/default_output_working_directory"
        "-DOUTPUT_NAME=positive_integer_output"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/default_output_working_directory/explicit.ll"
        "-DEXPECTED_OUTPUT=42\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DefaultOutputDirectoryTest.cmake"
)
add_test(NAME simp_cli_accepts_program
    COMMAND $<TARGET_FILE:simp> "${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_control_flow.simp" --check-only --dump-ast
)
add_test(NAME simp_cli_rejects_conditional_else
    COMMAND $<TARGET_FILE:simp> "${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_conditional_else.simp"
)
set_tests_properties(simp_cli_rejects_conditional_else PROPERTIES WILL_FAIL TRUE)
add_test(NAME simp_cli_rejects_adjacent_statements
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_adjacent_statements.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_adjacent_statements"
        "-DEXPECTED_DIAGNOSTIC=expected newline after statement"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)

function(add_inline_run_test name expected_output)
    add_test(NAME simp_${name}
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/${name}.simp"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${name}"
            "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${name}.ll"
            "-DEXPECTED_OUTPUT=${expected_output}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
    )
endfunction()

function(add_inline_diagnostic_test name expected_diagnostic)
    add_test(NAME simp_${name}
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/${name}.simp"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${name}"
            "-DEXPECTED_DIAGNOSTIC=${expected_diagnostic}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
    )
endfunction()

add_inline_run_test(positive_inline_no_captures "inline { says } hello\n")
add_inline_run_test(positive_inline_int_capture "12\n")
add_inline_run_test(positive_inline_string_capture "hello from C\n")
add_inline_run_test(positive_inline_multiple_captures "answer: 42\n")
add_inline_run_test(positive_inline_array_capture "3\n3\n")
add_inline_run_test(positive_inline_method "42\n")
add_inline_diagnostic_test(negative_inline_outside_function
    "'inline' is only allowed inside a function or method body")
add_inline_diagnostic_test(negative_inline_void_capture
    "inline capture type 'void' is not supported")
add_inline_diagnostic_test(negative_inline_any_capture
    "inline capture type 'any' is not supported")
add_inline_diagnostic_test(negative_inline_duplicate_capture
    "duplicate inline capture 'value'")
add_inline_diagnostic_test(negative_inline_undefined_capture
    "undefined inline capture 'missing'")
add_inline_diagnostic_test(negative_inline_type_mismatch_capture
    "inline capture type 'int' does not match string variable 'value'")
add_test(NAME simp_compile_and_run_newline_statements
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_newline_statements.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_newline_statements"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_newline_statements.ll"
        "-DEXPECTED_OUTPUT=42\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)

add_test(NAME simp_compile_and_run_integer_program
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_integer_output.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_integer_output"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_integer_output.ll"
        "-DEXPECTED_OUTPUT=42\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_uninitialized_variable
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_uninitialized.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_uninitialized"
        "-DEXPECTED_DIAGNOSTIC=variable 'value' may be uninitialized"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_integer_control_flow
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_integer_control_flow.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_integer_control_flow"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_integer_control_flow.ll"
        "-DEXPECTED_OUTPUT=9\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_utf8_string_format
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_string_format.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_string_format"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_string_format.ll"
        "-DEXPECTED_OUTPUT=café\n\nvalue: 42\nsum 21 21\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_non_integer_formatted_value
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_string_format_value.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_string_format_value"
        "-DEXPECTED_DIAGNOSTIC=formatted print arguments must have type int"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_malformed_format
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_malformed_format.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_malformed_format"
        "-DEXPECTED_DIAGNOSTIC=placeholders are supported in formatted strings"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_string_condition
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_string_condition.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_string_condition"
        "-DEXPECTED_DIAGNOSTIC=if condition must have type int"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_class_counter
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_class_counter.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_class_counter"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_class_counter.ll"
        "-DEXPECTED_OUTPUT=42\n42\n42\n"
        "-DREQUIRE_GC_ROOTS=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_precise_gc_object_graph
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_gc_object_graph.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_gc_object_graph"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_gc_object_graph.ll"
        "-DEXPECTED_OUTPUT=1\n64\n77\n"
        "-DREQUIRE_GC_ROOTS=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_single_inheritance
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_single_inheritance.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_single_inheritance"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_single_inheritance.ll"
        "-DEXPECTED_OUTPUT=49\n49\n49\n40\n5\n"
        "-DREQUIRE_GC_ROOTS=ON"
        "-DREQUIRE_VIRTUAL_DISPATCH=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_multiple_inheritance
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_multiple_inheritance.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_multiple_inheritance"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_multiple_inheritance.ll"
        "-DEXPECTED_OUTPUT=7\n7\n10\n20\n3\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_secondary_bases
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_secondary_bases.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_secondary_bases"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_secondary_bases.ll"
        "-DEXPECTED_OUTPUT=77\n77\n77\n77\n77\n4\n8\n12\n28\n28\n77\nDiamond\nRight\n8\nLeft\n4\nsecondary destructor failure\nsecondary constructor failure\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)

function(add_namespace_run_test test_name fixture expected_output)
    add_test(NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/${fixture}"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${fixture}"
            "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${fixture}.ll"
            "-DEXPECTED_OUTPUT=${expected_output}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
    )
endfunction()

function(add_namespace_reject_test test_name fixture expected_diagnostic)
    add_test(NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/${fixture}"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${fixture}"
            "-DEXPECTED_DIAGNOSTIC=${expected_diagnostic}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
    )
endfunction()

add_namespace_run_test(simp_namespace_single_class
    positive_namespace_single.simp "42\n")
add_namespace_run_test(simp_namespace_physical_nesting
    positive_namespace_nested.simp "5\n")
add_namespace_run_test(simp_namespace_empty_body
    positive_namespace_empty.simp "42\n")
add_namespace_run_test(simp_namespace_reopen
    positive_namespace_reopen.simp "42\n")
add_namespace_run_test(simp_namespace_distinct_short_names
    positive_namespace_collisions.simp "42\n")
add_namespace_run_test(simp_namespace_qualified_construction_and_call
    positive_namespace_qualified_call.simp "42\n")
add_namespace_run_test(simp_namespace_across_included_source
    positive_namespace_include.simp "42\n")
add_namespace_run_test(simp_namespace_out_of_line_method
    positive_namespace_out_of_line.simp "42\n")
add_namespace_reject_test(simp_rejects_namespace_start
    negative_namespace_start.simp "'start' cannot be declared inside a namespace")
add_namespace_reject_test(simp_rejects_namespace_statement
    negative_namespace_statement.simp "namespace body may contain only namespace and class declarations")
add_namespace_reject_test(simp_rejects_unresolved_namespace_reference
    negative_namespace_unresolved.simp "unknown qualified class")
add_namespace_reject_test(simp_rejects_dotted_namespace_declaration
    negative_namespace_dotted.simp "expected '{' after namespace name")

function(add_import_run_test test_name fixture expected_output)
    add_test(NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/${fixture}"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${fixture}"
            "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${fixture}.ll"
            "-DEXPECTED_OUTPUT=${expected_output}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES
        ENVIRONMENT "SIMP_MODULE_REGISTRY=${CMAKE_CURRENT_SOURCE_DIR}/functional/modules/simp-modules.tsv"
    )
endfunction()

function(add_import_reject_test test_name fixture expected_diagnostic)
    add_test(NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/${fixture}"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${fixture}"
            "-DEXPECTED_DIAGNOSTIC=${expected_diagnostic}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES
        ENVIRONMENT "SIMP_MODULE_REGISTRY=${CMAKE_CURRENT_SOURCE_DIR}/functional/modules/simp-modules.tsv"
    )
endfunction()

add_import_run_test(simp_import_namespace_and_link
    positive_import_namespace.simp "/\n")
add_import_run_test(simp_import_class_and_link
    positive_import_class.simp "42\n")
add_import_run_test(simp_import_transitive_module_and_link
    positive_import_transitive.simp "42\n")
add_import_reject_test(simp_rejects_import_in_namespace
    negative_import_namespace_body.simp "'import' is only allowed at top level")
add_import_reject_test(simp_rejects_import_in_class
    negative_import_class_body.simp "'import' is only allowed at top level")
add_import_reject_test(simp_rejects_import_in_function
    negative_import_function_body.simp "'import' is only allowed at top level")
add_import_reject_test(simp_rejects_unregistered_module
    negative_import_unregistered.simp "module 'missing_module' is not registered")
add_import_reject_test(simp_rejects_reopening_imported_namespace
    negative_import_reopen.simp "import alias 'Net' conflicts with a local declaration")
add_import_reject_test(simp_rejects_unqualified_imported_names
    negative_import_unqualified.simp "undefined variable 'Network' or field")

add_test(NAME simp_compile_and_run_public_inheritance
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_public_inheritance.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_public_inheritance"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_public_inheritance.ll"
        "-DEXPECTED_OUTPUT=42\n0\n0\n0\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_member_access
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_member_access.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_member_access"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_member_access.ll"
        "-DEXPECTED_OUTPUT=42\n0\n0\n7\n1\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_mixed_member_visibility
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_mixed_member_visibility.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_mixed_member_visibility"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_mixed_member_visibility.ll"
        "-DEXPECTED_OUTPUT=9\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_private_field_access
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_private_member.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_private_member"
        "-DEXPECTED_DIAGNOSTIC=field 'value' is not accessible"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_protected_method_access
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_protected_member.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_protected_member"
        "-DEXPECTED_DIAGNOSTIC=method 'read' is not accessible"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_private_method_access
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_private_method.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_private_method"
        "-DEXPECTED_DIAGNOSTIC=method 'read' is not accessible"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_private_constructor
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_private_constructor.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_private_constructor"
        "-DEXPECTED_DIAGNOSTIC=constructor for class 'Hidden' is not accessible"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_private_destructor
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_private_destructor.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_private_destructor"
        "-DEXPECTED_DIAGNOSTIC=destructor for class 'Hidden' is not accessible here"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_explicit_destructor
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_explicit_destructor.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_explicit_destructor"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_explicit_destructor.ll"
        "-DEXPECTED_OUTPUT=42\n"
        "-DEXPECT_WARNING=warning: explicit destructor call"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_double_destructor_call
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_double_destructor.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_double_destructor"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_double_destructor.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_use_after_destructor
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_use_after_destructor.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_use_after_destructor"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_use_after_destructor.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_gc_finalizers
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_gc_finalizers.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_gc_finalizers"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_gc_finalizers.ll"
        "-DEXPECTED_OUTPUT=2\n1\n3\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_finalizer_allocation
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_finalizer_allocation.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_finalizer_allocation"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_finalizer_allocation.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_inherited_destructors
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_inherited_destructors.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_inherited_destructors"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_inherited_destructors.ll"
        "-DEXPECTED_OUTPUT=11\n10\n31\n30\n3\n2\n1\n6\n5\n4\nbase after derived error\nderived destructor failure\ncaught repeated destruction\ndiamond\nroot subobject\nroot subobject\n51\n50\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_runtime_exceptions
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_runtime_exceptions.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_runtime_exceptions"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_runtime_exceptions.ll"
        "-DEXPECTED_OUTPUT=caught raise\nfinally after exception\n1\nfinally after success\ncaught method raise\ncaught constructor raise\ncaught destructor raise\ncaught destroyed after destructor raise\nfinally after handler error\ncaught handler raise\n2\ncaught finally raise\ncaught repeated destruction\ncaught destroyed object\ncaught null reference\ncaught division by zero\ncaught conditional raise\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_uncaught_runtime_exception
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_uncaught_exception.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_uncaught_exception"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_uncaught_exception.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=simp: uncaught runtime exception at .*negative_uncaught_exception.simp:2:3: uncaught sentinel"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_exception_bindings
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_exception_binding.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_exception_binding"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_exception_binding.ll"
        "-DEXPECTED_OUTPUT=raised detail\nfinally after bound exception\nretained detail\ndivision by zero\nnull reference\nreplacement detail\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_uncaught_runtime_operator_location
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_uncaught_division.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_uncaught_division"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_uncaught_division.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=simp: uncaught runtime exception at .*negative_uncaught_division.simp:2:11: division by zero"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_original_exception_location_after_reraise
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_uncaught_reraise.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_uncaught_reraise"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_uncaught_reraise.ll"
        "-DEXPECTED_OUTPUT=finally before rethrow\n"
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=simp: uncaught runtime exception at .*negative_uncaught_reraise.simp:3:5: original source"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_writing_exception_binding
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_exception_binding_assignment.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_exception_binding_assignment"
        "-DEXPECTED_DIAGNOSTIC=exception binding 'message' is read-only"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_private_super_constructor
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_private_super_constructor.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_private_super_constructor"
        "-DEXPECTED_DIAGNOSTIC=base constructor for class 'Base' is not accessible"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_protected_inherited_member_access
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_protected_inheritance.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_protected_inheritance"
        "-DEXPECTED_DIAGNOSTIC=is not accessible through this inheritance path"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_private_inherited_member_access
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_private_inheritance.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_private_inheritance"
        "-DEXPECTED_DIAGNOSTIC=is not accessible through this inheritance path"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_private_member_from_further_derived_class
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_private_inheritance_context.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_private_inheritance_context"
        "-DEXPECTED_DIAGNOSTIC=field 'value' is not accessible in this class"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_protected_base_conversion
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_protected_base_conversion.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_protected_base_conversion"
        "-DEXPECTED_DIAGNOSTIC=cannot initialize Base variable with ProtectedChild"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_private_base_conversion
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_private_base_conversion.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_private_base_conversion"
        "-DEXPECTED_DIAGNOSTIC=cannot initialize Base variable with PrivateChild"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_ambiguous_inherited_field
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_ambiguous_inherited_field.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_ambiguous_inherited_field"
        "-DEXPECTED_DIAGNOSTIC=ambiguous inherited field"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_ambiguous_inherited_method
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_ambiguous_inherited_method.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_ambiguous_inherited_method"
        "-DEXPECTED_DIAGNOSTIC=ambiguous inherited method"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_duplicate_base
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_duplicate_base.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_duplicate_base"
        "-DEXPECTED_DIAGNOSTIC=duplicate base class"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_unknown_base
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_unknown_base.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_unknown_base"
        "-DEXPECTED_DIAGNOSTIC=unknown class 'Missing'"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_ambiguous_base_conversion
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_ambiguous_base_conversion.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_ambiguous_base_conversion"
        "-DEXPECTED_DIAGNOSTIC=cannot initialize Root variable with Diamond"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_qualified_secondary_base_dispatch
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_qualified_secondary_base_dispatch.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_qualified_secondary_base_dispatch"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/secondary_base_dispatch.ll"
        "-DEXPECTED_OUTPUT=2\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_missing_secondary_base_initialization
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_secondary_base_constructor.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_secondary_base_constructor"
        "-DEXPECTED_DIAGNOSTIC=derived constructor must initialize base 'Secondary'"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_virtual_base_inheritance
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_virtual_base_inheritance.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_virtual_base_inheritance"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_virtual_base_inheritance.ll"
        "-DEXPECTED_OUTPUT=Root+\nLeft+\nRight+\nDiamond+\n41\n123\n50\n77\n77\nDiamond-\nRight-\nLeft-\nRoot-\nvirtual initialization failed\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_parameterized_virtual_base
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_virtual_base_constructor_args.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_virtual_base_constructor_args"
        "-DEXPECTED_DIAGNOSTIC=most-derived constructor must initialize virtual base 'Root'"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_parameterized_virtual_base
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_parameterized_virtual_base.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_parameterized_virtual_base"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_parameterized_virtual_base.ll"
        "-DEXPECTED_OUTPUT=Root:42\nLeft+\nRight+\nDiamond+\n42\n42\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_missing_virtual_base_initializer
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_virtual_base_missing_initializer.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_virtual_base_missing_initializer"
        "-DEXPECTED_DIAGNOSTIC=most-derived constructor must initialize virtual base 'Root'"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_duplicate_virtual_base_initializer
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_virtual_base_duplicate_initializer.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_virtual_base_duplicate_initializer"
        "-DEXPECTED_DIAGNOSTIC=virtual base 'Root' is initialized more than once"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_virtual_base_initializer_type
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_virtual_base_initializer_type.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_virtual_base_initializer_type"
        "-DEXPECTED_DIAGNOSTIC=virtual base constructor argument type does not match"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_virtual_base_initializer_order
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_virtual_base_initializer_order.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_virtual_base_initializer_order"
        "-DEXPECTED_DIAGNOSTIC=virtual base initializers must precede direct base"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_intermediate_virtual_base_initializer
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_virtual_base_initializer_in_intermediate.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_virtual_base_initializer_in_intermediate"
        "-DEXPECTED_DIAGNOSTIC=virtual base initializers are only allowed in most-derived classes"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_transitive_virtual_bases
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_transitive_virtual_bases.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_transitive_virtual_bases"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_transitive_virtual_bases.ll"
        "-DEXPECTED_OUTPUT=Common+\nAncestor+\nRoot+\nLeft+\nRight+\nDiamond+\n7\n7\n50\n8\n50\n7\n77\n77\n40\nDiamond-\nRight-\nLeft-\nRoot-\nAncestor-\nCommon-\ntransitive initialization failed\nCommon+\nAncestor+\nRoot+\nLeft+\nRight+\nDiamond+\nbefore collection\nDiamond-\nRight-\nLeft-\nRoot-\nAncestor-\nCommon-\nafter collection\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_transitive_virtual_base_initializer_order
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_transitive_virtual_base_initializer_order.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_transitive_virtual_base_initializer_order"
        "-DEXPECTED_DIAGNOSTIC=virtual base initializers must follow virtual-base construction order"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_ambiguous_transitive_virtual_conversion
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_transitive_ambiguous_conversion.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_transitive_ambiguous_conversion"
        "-DEXPECTED_DIAGNOSTIC=cannot initialize Ancestor variable with Diamond"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_ambiguous_transitive_virtual_field
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_transitive_ambiguous_field.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_transitive_ambiguous_field"
        "-DEXPECTED_DIAGNOSTIC=ambiguous inherited field"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_ambiguous_transitive_virtual_method
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_transitive_ambiguous_method.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_transitive_ambiguous_method"
        "-DEXPECTED_DIAGNOSTIC=ambiguous inherited method"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_explicit_virtual_base_initialization
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_virtual_base_explicit_super.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_virtual_base_explicit_super"
        "-DEXPECTED_DIAGNOSTIC=virtual base constructors must use super.virtual"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_incompatible_override
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_incompatible_override.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_incompatible_override"
        "-DEXPECTED_DIAGNOSTIC=override of 'value' must preserve the inherited method signature"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_inheritance_missing_super
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_inheritance_missing_super.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_inheritance_missing_super"
        "-DEXPECTED_DIAGNOSTIC=derived constructor must begin with super.Base"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_inheritance_cycle
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_inheritance_cycle.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_inheritance_cycle"
        "-DEXPECTED_DIAGNOSTIC=inheritance cycle"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_unknown_class
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_unknown_class.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_unknown_class"
        "-DEXPECTED_DIAGNOSTIC=unknown type or class 'Missing'"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_unknown_member
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_unknown_member.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_unknown_member"
        "-DEXPECTED_DIAGNOSTIC=class 'Counter' has no field 'missing'"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_unknown_method
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_unknown_method.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_unknown_method"
        "-DEXPECTED_DIAGNOSTIC=class 'Counter' has no method 'missing'"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_object_type_mismatch
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_object_type_mismatch.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_object_type_mismatch"
        "-DEXPECTED_DIAGNOSTIC=cannot initialize int variable with Counter"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_native_method_definitions
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_extern_functions.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_extern_functions"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_extern_functions.ll"
        "-DEXPECTED_OUTPUT=7\n5\nhello\n3\n1\n42\n10\n"
        "-DREQUIRE_GC_ROOTS=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_native_method_argument_count_mismatch
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_extern_argument_count.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_extern_argument_count"
        "-DEXPECTED_DIAGNOSTIC=method 'absolute' argument count mismatch"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_native_method_argument_type_mismatch
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_extern_argument_type.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_extern_argument_type"
        "-DEXPECTED_DIAGNOSTIC=method argument type does not match parameter 'value'"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_native_method_any_parameter
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_extern_any_parameter.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_extern_any_parameter"
        "-DEXPECTED_DIAGNOSTIC='any' is not supported in external method signatures"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_native_method_any_return
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_extern_any_return.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_extern_any_return"
        "-DEXPECTED_DIAGNOSTIC='any' is not supported in external method signatures"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_duplicate_out_of_line_method_definition
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_extern_duplicate_name.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_extern_duplicate_name"
        "-DEXPECTED_DIAGNOSTIC=duplicate out-of-line definition for method 'Native.absolute'"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_out_of_line_method_signature_mismatch
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_extern_signature_mismatch.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_extern_signature_mismatch"
        "-DEXPECTED_DIAGNOSTIC=out-of-line definition signature does not match declaration"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_incompatible_native_symbol_signatures
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_extern_symbol_signature_conflict.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_extern_symbol_signature_conflict"
        "-DEXPECTED_DIAGNOSTIC=is reused with an incompatible method signature"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_native_method_missing_symbol_at_link_time
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_extern_missing_symbol.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_extern_missing_symbol"
        "-DEXPECTED_DIAGNOSTIC=undefined reference to .simp_symbol_that_does_not_exist."
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_method_declaration_without_definition
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_extern_missing_definition.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_extern_missing_definition"
        "-DEXPECTED_DIAGNOSTIC=is declared but has no out-of-line definition"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_include_basic
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_include_basic.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_include_basic"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_include_basic.ll"
        "-DEXPECTED_OUTPUT=42\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_include_namespace_merge
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_include_namespace_merge.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_include_namespace_merge"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_include_namespace_merge.ll"
        "-DEXPECTED_OUTPUT=17\n25\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_include_once
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_include_once.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_include_once"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_include_once.ll"
        "-DEXPECTED_OUTPUT=7\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_include_cycle
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_include_cycle.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_include_cycle"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_include_cycle.ll"
        "-DEXPECTED_OUTPUT=9\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_compile_and_run_include_relative_path
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive_include_relative_path.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_include_relative_path"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_include_relative_path.ll"
        "-DEXPECTED_OUTPUT=33\n"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_include_in_class_body
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_include_in_class_body.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_include_in_class_body"
        "-DEXPECTED_DIAGNOSTIC='include' is only allowed at top level"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_include_in_function_body
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_include_in_function_body.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_include_in_function_body"
        "-DEXPECTED_DIAGNOSTIC='include' is only allowed at top level"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_include_in_namespace_body
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_include_in_namespace_body.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_include_in_namespace_body"
        "-DEXPECTED_DIAGNOSTIC='include' is only allowed at top level"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_include_max_depth
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_include_max_depth.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_include_max_depth"
        "-DEXPECTED_DIAGNOSTIC=maximum include depth of 16 exceeded"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_include_max_depth_flag
    COMMAND "$<TARGET_FILE:simp>" --check-only --max-include-depth 17
        "${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_include_max_depth.simp"
)
add_test(NAME simp_rejects_include_start
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_include_start.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_include_start"
        "-DEXPECTED_DIAGNOSTIC=included source cannot declare 'start'"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_include_after_start
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_include_after_start.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_include_after_start"
        "-DEXPECTED_DIAGNOSTIC='include' must appear before top-level 'start'"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_include_missing_file
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_include_missing_file.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_include_missing_file"
        "-DEXPECTED_DIAGNOSTIC=cannot resolve included source"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
