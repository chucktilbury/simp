set(CASE_NAME cwhip_compile_and_run_buffer_handle)
set(CASE_FIXTURE positive_buffer_handle.cw)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/cwhip_compile_and_run_buffer_handle.stdout")
set(CASE_REQUIRE_GC_ROOTS [==[ON]==])
