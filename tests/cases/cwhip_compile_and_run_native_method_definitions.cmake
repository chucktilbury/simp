set(CASE_NAME cwhip_compile_and_run_native_method_definitions)
set(CASE_FIXTURE positive_extern_functions.cw)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/cwhip_compile_and_run_native_method_definitions.stdout")
set(CASE_REQUIRE_GC_ROOTS [==[ON]==])
