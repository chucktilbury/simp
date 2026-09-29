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
add_test(NAME simp_rejects_secondary_base_dispatch
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_secondary_base_dispatch.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_secondary_base_dispatch"
        "-DEXPECTED_DIAGNOSTIC=virtual dispatch through a secondary base path is not supported"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_secondary_base_constructor
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative_secondary_base_constructor.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_secondary_base_constructor"
        "-DEXPECTED_DIAGNOSTIC=constructors for secondary bases are not supported"
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
