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
    SIMP_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
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







add_test(NAME simp_reports_uncaught_map_slice_bounds
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_map_slice_bounds_uncaught.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_slice_bounds_uncaught"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_slice_bounds_uncaught.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=uncaught runtime exception at .*negative_map_slice_bounds_uncaught.simp:4:[0-9]+: map slice bounds out of bounds"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)



add_test(NAME simp_reports_map_extraction_mismatch
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_map_extraction_mismatch.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_extraction_mismatch"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_extraction_mismatch.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=uncaught runtime exception at .*negative_map_extraction_mismatch.simp:4:19: 'any' value does not hold a map reference"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_uncaught_missing_map_key
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_map_missing_key.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_missing_key"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_map_missing_key.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=uncaught runtime exception at .*negative_map_missing_key.simp:4:17: map key not found"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)

add_test(NAME simp_reports_uncaught_array_index_bounds
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_array_bounds_uncaught.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_array_bounds_uncaught"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_array_bounds_uncaught.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=uncaught runtime exception at .*negative_array_bounds_uncaught.simp:4:17: array index out of bounds"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_uncaught_array_slice_bounds
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_array_slice_bounds_uncaught.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_array_slice_bounds_uncaught"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_array_slice_bounds_uncaught.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=uncaught runtime exception at .*negative_array_slice_bounds_uncaught.simp:4:24: array slice bounds out of bounds"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_uncaught_any_extraction_mismatch
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_any_extraction_mismatch.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_any_extraction_mismatch"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_any_extraction_mismatch.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=uncaught runtime exception at .*negative_any_extraction_mismatch.simp:7:28: 'any' value does not hold the requested type"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_uncaught_any_class_mismatch
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_any_class_mismatch.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_any_class_mismatch"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_any_class_mismatch.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=uncaught runtime exception at .*negative_any_class_mismatch.simp:20:26: 'any' value holds a different class than the requested type"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
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
add_test(NAME simp_cli_rejects_conditional_else
    COMMAND $<TARGET_FILE:simp> "${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_conditional_else.simp"
)
set_tests_properties(simp_cli_rejects_conditional_else PROPERTIES WILL_FAIL TRUE)


function(get_functional_fixture_path output fixture)
    if(fixture MATCHES "^positive_")
        set(fixture_directory positive)
    elseif(fixture MATCHES "^negative_")
        set(fixture_directory negative)
    else()
        message(FATAL_ERROR "Functional fixture must be positive_*.simp or negative_*.simp: ${fixture}")
    endif()
    set(${output} "${CMAKE_CURRENT_SOURCE_DIR}/functional/${fixture_directory}/${fixture}" PARENT_SCOPE)
endfunction()

function(add_inline_run_test name expected_output)
    get_functional_fixture_path(source "${name}.simp")
    add_test(NAME simp_${name}
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DSOURCE=${source}"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${name}"
            "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${name}.ll"
            "-DEXPECTED_OUTPUT=${expected_output}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
    )
endfunction()

function(add_inline_diagnostic_test name expected_diagnostic)
    get_functional_fixture_path(source "${name}.simp")
    add_test(NAME simp_${name}
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DSOURCE=${source}"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${name}"
            "-DEXPECTED_DIAGNOSTIC=${expected_diagnostic}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
    )
endfunction()

set(FUNCTIONAL_RUN_TESTS
    "simp_compile_and_run_loop_logic|positive_loop_logic.simp|4\n1\n2\n4\n1\n2\nfalse\nfalse\ntrue\ntrue\n0\ntrue\ntrue\nfalse\ntrue\n4\n0\n0\n2\n5\n1\n|-"
    "simp_compile_and_run_scalar_types|positive_scalar_types.simp|true\nfalse\ntrue\ntrue\ntrue\nbool condition\n3\n1\n3.64\n0.0015\ntrue\n18446744073709551615\n6148914691236517205\n2\ntrue\nformatted false 0.0015 42\ntrue\n3.64\n18446744073709551615\ntrue\n3.64\n18446744073709551615\nfalse\n0.0015\n42\ntrue\n3.64\n18446744073709551615\n2\ntrue\n1.5\n7\n|gc"
    "simp_compile_and_run_float_literal_forms|positive_float_literal_forms.simp|0.5\n5\n3.14\n1000\n50\n500\n0\n|gc"
    "simp_compile_and_run_typed_arrays|positive_arrays.simp|4\ntwenty\ntwenty-five\n25\n2\n25\n25\n99\n0\n0\n11\ntwo\n7\n8\n7\n11\n1\ntwo\n<object>\nnull\narray index out of bounds\narray slice bounds out of bounds\n|gc"
    "simp_compile_and_run_heterogeneous_maps|positive_maps.simp|5\n7\n7\nseven\n11\n1\n2\n3\n1\n2\n3\n64\n9\n88\n77\nmap key not found\n|gc"
    "simp_compile_and_run_collection_iteration|positive_collection_iteration.simp|3\n1\n0\n3\n22\nfirst\n10\ndynamic\n22\nthird\n30\n3\n4\n1\ntwo\n<object>\n<object>\n42\n100\n|gc"
    "simp_compile_and_run_collection_followups|positive_collection_followups.simp|3\n2\n20\n1\n0\n1\n2\n4\n5\n30\n5\n5\nsecond\nthird\nfourth\n10\n20\n30\n200\n77\n|gc"
    "simp_compile_and_run_newline_statements|positive_newline_statements.simp|42\n|-"
    "simp_compile_and_run_integer_program|positive_integer_output.simp|42\n|-"
    "simp_compile_and_run_integer_control_flow|positive_integer_control_flow.simp|9\n|-"
    "simp_compile_and_run_utf8_string_format|positive_string_format.simp|café\n\nvalue: 42\nsum 21 21\n|-"
    "simp_compile_and_run_class_counter|positive_class_counter.simp|42\n42\n42\n|gc"
    "simp_compile_and_run_precise_gc_object_graph|positive_gc_object_graph.simp|1\n64\n77\n|gc"
    "simp_compile_and_run_multiple_inheritance|positive_multiple_inheritance.simp|7\n7\n10\n20\n3\n|-"
    "simp_compile_and_run_secondary_bases|positive_secondary_bases.simp|77\n77\n77\n77\n77\n4\n8\n12\n28\n28\n77\nDiamond\nRight\n8\nLeft\n4\nsecondary destructor failure\nsecondary constructor failure\n|-"
    "simp_compile_and_run_public_inheritance|positive_public_inheritance.simp|42\n0\n0\n0\n|-"
    "simp_compile_and_run_member_access|positive_member_access.simp|42\n0\n0\n7\n1\n|-"
    "simp_compile_and_run_mixed_member_visibility|positive_mixed_member_visibility.simp|9\n|-"
    "simp_compile_and_run_gc_finalizers|positive_gc_finalizers.simp|2\n1\n3\n|-"
    "simp_compile_and_run_inherited_destructors|positive_inherited_destructors.simp|11\n10\n31\n30\n3\n2\n1\n6\n5\n4\nbase after derived error\nderived destructor failure\ncaught repeated destruction\ndiamond\nroot subobject\nroot subobject\n51\n50\n|-"
    "simp_compile_and_run_runtime_exceptions|positive_runtime_exceptions.simp|caught raise\nfinally after exception\n1\nfinally after success\ncaught method raise\ncaught constructor raise\ncaught destructor raise\ncaught destroyed after destructor raise\nfinally after handler error\ncaught handler raise\n2\ncaught finally raise\ncaught repeated destruction\ncaught destroyed object\ncaught null reference\ncaught division by zero\ncaught conditional raise\n|-"
    "simp_compile_and_run_exception_bindings|positive_exception_binding.simp|raised detail\nfinally after bound exception\nretained detail\ndivision by zero\nnull reference\nreplacement detail\n|-"
    "simp_compile_and_run_qualified_secondary_base_dispatch|positive_qualified_secondary_base_dispatch.simp|2\n|-"
    "simp_compile_and_run_virtual_base_inheritance|positive_virtual_base_inheritance.simp|Root+\nLeft+\nRight+\nDiamond+\n41\n123\n50\n77\n77\nDiamond-\nRight-\nLeft-\nRoot-\nvirtual initialization failed\n|-"
    "simp_compile_and_run_parameterized_virtual_base|positive_parameterized_virtual_base.simp|Root:42\nLeft+\nRight+\nDiamond+\n42\n42\n|-"
    "simp_compile_and_run_transitive_virtual_bases|positive_transitive_virtual_bases.simp|Common+\nAncestor+\nRoot+\nLeft+\nRight+\nDiamond+\n7\n7\n50\n8\n50\n7\n77\n77\n40\nDiamond-\nRight-\nLeft-\nRoot-\nAncestor-\nCommon-\ntransitive initialization failed\nCommon+\nAncestor+\nRoot+\nLeft+\nRight+\nDiamond+\nbefore collection\nDiamond-\nRight-\nLeft-\nRoot-\nAncestor-\nCommon-\nafter collection\n|-"
    "simp_compile_and_run_native_method_definitions|positive_extern_functions.simp|7\n5\nhello\n3\n1\n42\n10\n|gc"
    "simp_compile_and_run_include_basic|positive_include_basic.simp|42\n|-"
    "simp_compile_and_run_include_namespace_merge|positive_include_namespace_merge.simp|17\n25\n|-"
    "simp_compile_and_run_include_once|positive_include_once.simp|7\n|-"
    "simp_compile_and_run_include_cycle|positive_include_cycle.simp|9\n|-"
    "simp_compile_and_run_include_relative_path|positive_include_relative_path.simp|33\n|-"
    "simp_compile_and_run_return_parentheses|positive_return_parentheses.simp|10\n1\n|-"
)
foreach(test_row IN LISTS FUNCTIONAL_RUN_TESTS)
    string(REPLACE "|" ";" test_fields "${test_row}")
    list(GET test_fields 0 test_name)
    list(GET test_fields 1 fixture)
    list(GET test_fields 2 expected)
    list(GET test_fields 3 gc_roots)
    string(REPLACE "__CMAKE_SEMICOLON__" ";" expected "${expected}")
    get_filename_component(fixture_stem "${fixture}" NAME_WE)
    get_functional_fixture_path(source "${fixture}")
    set(test_arguments
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${source}"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${fixture_stem}"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${fixture_stem}.ll"
        "-DEXPECTED_OUTPUT=${expected}"
    )
    if(gc_roots STREQUAL "gc")
        list(APPEND test_arguments "-DREQUIRE_GC_ROOTS=ON")
    endif()
    add_test(NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}" ${test_arguments}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
    )
endforeach()

set(FUNCTIONAL_REJECT_TESTS
    "simp_rejects_integer_condition|negative_int_condition.simp|if condition must have type bool|-"
    "simp_rejects_float_int_mixing|negative_float_int_mixing.simp|requires matching int, float, or unsigned operands|-"
    "simp_rejects_unsigned_int_mixing|negative_unsigned_int_mixing.simp|requires matching int, float, or unsigned operands|-"
    "simp_rejects_bool_arithmetic|negative_bool_arithmetic.simp|requires matching int, float, or unsigned operands|-"
    "simp_rejects_float_remainder|negative_float_remainder.simp|float remainder is unsupported|-"
    "simp_rejects_break_outside_loop|negative_break_outside_loop.simp|'break' outside of a loop|-"
    "simp_rejects_continue_outside_loop|negative_continue_outside_loop.simp|'continue' outside of a loop|-"
    "simp_rejects_do_while_non_boolean_condition|negative_do_while_condition.simp|while condition must have type bool|-"
    "simp_rejects_non_boolean_logical_operands|negative_logical_operand_type.simp|logical operator '&&' requires bool operands|-"
    "simp_rejects_non_integer_array_index|negative_array_index_type.simp|array index and slice bounds must be int|-"
    "simp_rejects_any_map_index_key|negative_map_nonliteral_key.simp|map keys must have type string|-"
    "simp_rejects_numeric_map_index_key|negative_map_numeric_key.simp|map keys must have type string|-"
    "simp_rejects_array_key_value_iteration|negative_array_iteration_arity.simp|array iteration accepts one value variable|-"
    "simp_rejects_nonstring_map_literal_key|negative_map_literal_key.simp|map keys must have type string|-"
    "simp_rejects_map_length_assignment|negative_map_length_assignment.simp|map length is read-only|-"
    "simp_rejects_any_member_access|negative_any_member_access.simp|'any' has no members__CMAKE_SEMICOLON__ assign it to a typed variable first to extract its value|-"
    "simp_cli_rejects_adjacent_statements|negative_adjacent_statements.simp|expected newline after statement|-"
    "simp_rejects_uninitialized_variable|negative_uninitialized.simp|variable 'value' may be uninitialized|-"
    "simp_rejects_non_scalar_formatted_value|negative_string_format_value.simp|formatted print arguments must have type int|-"
    "simp_rejects_malformed_format|negative_malformed_format.simp|placeholders are supported in formatted strings|-"
    "simp_rejects_string_condition|negative_string_condition.simp|if condition must have type bool|-"
    "simp_rejects_private_field_access|negative_private_member.simp|field 'value' is not accessible|-"
    "simp_rejects_protected_method_access|negative_protected_member.simp|method 'read' is not accessible|-"
    "simp_rejects_private_method_access|negative_private_method.simp|method 'read' is not accessible|-"
    "simp_rejects_private_constructor|negative_private_constructor.simp|constructor for class 'Hidden' is not accessible|-"
    "simp_rejects_private_destructor|negative_private_destructor.simp|destructor for class 'Hidden' is not accessible here|-"
    "simp_rejects_writing_exception_binding|negative_exception_binding_assignment.simp|exception binding 'message' is read-only|-"
    "simp_rejects_private_super_constructor|negative_private_super_constructor.simp|base constructor for class 'Base' is not accessible|-"
    "simp_rejects_protected_inherited_member_access|negative_protected_inheritance.simp|is not accessible through this inheritance path|-"
    "simp_rejects_private_inherited_member_access|negative_private_inheritance.simp|is not accessible through this inheritance path|-"
    "simp_rejects_private_member_from_further_derived_class|negative_private_inheritance_context.simp|field 'value' is not accessible in this class|-"
    "simp_rejects_protected_base_conversion|negative_protected_base_conversion.simp|cannot initialize Base variable with ProtectedChild|-"
    "simp_rejects_private_base_conversion|negative_private_base_conversion.simp|cannot initialize Base variable with PrivateChild|-"
    "simp_rejects_ambiguous_inherited_field|negative_ambiguous_inherited_field.simp|ambiguous inherited field|-"
    "simp_rejects_ambiguous_inherited_method|negative_ambiguous_inherited_method.simp|ambiguous inherited method|-"
    "simp_rejects_duplicate_base|negative_duplicate_base.simp|duplicate base class|-"
    "simp_rejects_unknown_base|negative_unknown_base.simp|unknown class 'Missing'|-"
    "simp_rejects_ambiguous_base_conversion|negative_ambiguous_base_conversion.simp|cannot initialize Root variable with Diamond|-"
    "simp_rejects_missing_secondary_base_initialization|negative_secondary_base_constructor.simp|derived constructor must initialize base 'Secondary'|-"
    "simp_rejects_parameterized_virtual_base|negative_virtual_base_constructor_args.simp|most-derived constructor must initialize virtual base 'Root'|-"
    "simp_rejects_missing_virtual_base_initializer|negative_virtual_base_missing_initializer.simp|most-derived constructor must initialize virtual base 'Root'|-"
    "simp_rejects_duplicate_virtual_base_initializer|negative_virtual_base_duplicate_initializer.simp|virtual base 'Root' is initialized more than once|-"
    "simp_rejects_virtual_base_initializer_type|negative_virtual_base_initializer_type.simp|virtual base constructor argument type does not match|-"
    "simp_rejects_virtual_base_initializer_order|negative_virtual_base_initializer_order.simp|virtual base initializers must precede direct base|-"
    "simp_rejects_intermediate_virtual_base_initializer|negative_virtual_base_initializer_in_intermediate.simp|virtual base initializers are only allowed in most-derived classes|-"
    "simp_rejects_transitive_virtual_base_initializer_order|negative_transitive_virtual_base_initializer_order.simp|virtual base initializers must follow virtual-base construction order|-"
    "simp_rejects_ambiguous_transitive_virtual_conversion|negative_transitive_ambiguous_conversion.simp|cannot initialize Ancestor variable with Diamond|-"
    "simp_rejects_ambiguous_transitive_virtual_field|negative_transitive_ambiguous_field.simp|ambiguous inherited field|-"
    "simp_rejects_ambiguous_transitive_virtual_method|negative_transitive_ambiguous_method.simp|ambiguous inherited method|-"
    "simp_rejects_explicit_virtual_base_initialization|negative_virtual_base_explicit_super.simp|virtual base constructors must use super.virtual|-"
    "simp_rejects_incompatible_override|negative_incompatible_override.simp|override of 'value' must preserve the inherited method signature|-"
    "simp_rejects_inheritance_missing_super|negative_inheritance_missing_super.simp|derived constructor must begin with super.Base|-"
    "simp_rejects_inheritance_cycle|negative_inheritance_cycle.simp|inheritance cycle|-"
    "simp_rejects_unknown_class|negative_unknown_class.simp|unknown type or class 'Missing'|-"
    "simp_rejects_unknown_member|negative_unknown_member.simp|class 'Counter' has no field 'missing'|-"
    "simp_rejects_unknown_method|negative_unknown_method.simp|class 'Counter' has no method 'missing'|-"
    "simp_rejects_object_type_mismatch|negative_object_type_mismatch.simp|cannot initialize int variable with Counter|-"
    "simp_rejects_native_method_argument_count_mismatch|negative_extern_argument_count.simp|method 'absolute' argument count mismatch|-"
    "simp_rejects_native_method_argument_type_mismatch|negative_extern_argument_type.simp|method argument type does not match parameter 'value'|-"
    "simp_rejects_native_method_any_parameter|negative_extern_any_parameter.simp|'any' is not supported in external method signatures|-"
    "simp_rejects_native_method_any_return|negative_extern_any_return.simp|'any' is not supported in external method signatures|-"
    "simp_rejects_duplicate_out_of_line_method_definition|negative_extern_duplicate_name.simp|duplicate out-of-line definition for method 'Native.absolute'|-"
    "simp_rejects_out_of_line_method_signature_mismatch|negative_extern_signature_mismatch.simp|out-of-line definition signature does not match declaration|-"
    "simp_rejects_incompatible_native_symbol_signatures|negative_extern_symbol_signature_conflict.simp|is reused with an incompatible method signature|-"
    "simp_rejects_native_method_missing_symbol_at_link_time|negative_extern_missing_symbol.simp|undefined reference to .simp_symbol_that_does_not_exist.|-"
    "simp_rejects_method_declaration_without_definition|negative_extern_missing_definition.simp|is declared but has no out-of-line definition|-"
    "simp_rejects_include_in_class_body|negative_include_in_class_body.simp|'include' is only allowed at top level|-"
    "simp_rejects_include_in_function_body|negative_include_in_function_body.simp|'include' is only allowed at top level|-"
    "simp_rejects_include_in_namespace_body|negative_include_in_namespace_body.simp|'include' is only allowed at top level|-"
    "simp_rejects_include_max_depth|negative_include_max_depth.simp|maximum include depth of 16 exceeded|-"
    "simp_rejects_include_start|negative_include_start.simp|included source cannot declare 'start'|-"
    "simp_rejects_include_after_start|negative_include_after_start.simp|'include' must appear before top-level 'start'|-"
    "simp_rejects_include_missing_file|negative_include_missing_file.simp|cannot resolve included source|-"
)
foreach(test_row IN LISTS FUNCTIONAL_REJECT_TESTS)
    string(REPLACE "|" ";" test_fields "${test_row}")
    list(GET test_fields 0 test_name)
    list(GET test_fields 1 fixture)
    list(GET test_fields 2 expected)
    list(GET test_fields 3 gc_roots)
    string(REPLACE "__CMAKE_SEMICOLON__" ";" expected "${expected}")
    get_filename_component(fixture_stem "${fixture}" NAME_WE)
    get_functional_fixture_path(source "${fixture}")
    add_test(NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DSOURCE=${source}"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${fixture_stem}"
            "-DEXPECTED_DIAGNOSTIC=${expected}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
    )
endforeach()


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











add_test(NAME simp_compile_and_run_single_inheritance
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive/positive_single_inheritance.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_single_inheritance"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_single_inheritance.ll"
        "-DEXPECTED_OUTPUT=49\n49\n49\n40\n5\n"
        "-DREQUIRE_GC_ROOTS=ON"
        "-DREQUIRE_VIRTUAL_DISPATCH=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)



function(add_namespace_run_test test_name fixture expected_output)
    get_functional_fixture_path(source "${fixture}")
    add_test(NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DSOURCE=${source}"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${fixture}"
            "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${fixture}.ll"
            "-DEXPECTED_OUTPUT=${expected_output}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
    )
endfunction()

function(add_namespace_reject_test test_name fixture expected_diagnostic)
    get_functional_fixture_path(source "${fixture}")
    add_test(NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DSOURCE=${source}"
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
add_namespace_reject_test(simp_rejects_out_of_line_cross_namespace
    negative_out_of_line_cross_namespace.simp
    "must be defined in the same namespace scope as its class declaration")
add_namespace_reject_test(simp_rejects_out_of_line_top_level_class_in_namespace
    negative_out_of_line_top_level_class_in_namespace.simp
    "must be defined in the same namespace scope as its class declaration")
add_namespace_reject_test(simp_rejects_namespace_start
    negative_namespace_start.simp "'start' cannot be declared inside a namespace")
add_namespace_reject_test(simp_rejects_namespace_statement
    negative_namespace_statement.simp "namespace body may contain only namespace and class declarations")
add_namespace_reject_test(simp_rejects_unresolved_namespace_reference
    negative_namespace_unresolved.simp "unknown qualified class")
add_namespace_reject_test(simp_rejects_dotted_namespace_declaration
    negative_namespace_dotted.simp "expected '{' after namespace name")

function(add_import_run_test test_name fixture expected_output)
    get_functional_fixture_path(source "${fixture}")
    add_test(NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DSOURCE=${source}"
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
    get_functional_fixture_path(source "${fixture}")
    add_test(NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DCOMPILER=$<TARGET_FILE:simp>"
            "-DSOURCE=${source}"
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









add_test(NAME simp_compile_and_run_explicit_destructor
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/positive/positive_explicit_destructor.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_explicit_destructor"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/positive_explicit_destructor.ll"
        "-DEXPECTED_OUTPUT=42\n"
        "-DEXPECT_WARNING=warning: explicit destructor call"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_double_destructor_call
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_double_destructor.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_double_destructor"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_double_destructor.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_rejects_use_after_destructor
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_use_after_destructor.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_use_after_destructor"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_use_after_destructor.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)

add_test(NAME simp_rejects_finalizer_allocation
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_finalizer_allocation.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_finalizer_allocation"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_finalizer_allocation.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)


add_test(NAME simp_reports_uncaught_runtime_exception
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_uncaught_exception.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_uncaught_exception"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_uncaught_exception.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=simp: uncaught runtime exception at .*negative_uncaught_exception.simp:3:3: uncaught sentinel"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)

add_test(NAME simp_reports_uncaught_runtime_operator_location
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_uncaught_division.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_uncaught_division"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_uncaught_division.ll"
        "-DEXPECTED_OUTPUT="
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=simp: uncaught runtime exception at .*negative_uncaught_division.simp:3:11: division by zero"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)
add_test(NAME simp_reports_original_exception_location_after_reraise
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMPILER=$<TARGET_FILE:simp>"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_uncaught_reraise.simp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_uncaught_reraise"
        "-DIR_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative_uncaught_reraise.ll"
        "-DEXPECTED_OUTPUT=finally before rethrow\n"
        "-DEXPECT_RUNTIME_FAILURE=ON"
        "-DEXPECT_RUNTIME_DIAGNOSTIC=simp: uncaught runtime exception at .*negative_uncaught_reraise.simp:4:5: original source"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerIntegrationTest.cmake"
)






















































add_test(NAME simp_include_max_depth_flag
    COMMAND "$<TARGET_FILE:simp>" --check-only --max-include-depth 17
        "${CMAKE_CURRENT_SOURCE_DIR}/functional/negative/negative_include_max_depth.simp"
)
