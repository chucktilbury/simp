set(CASE_NAME simp_compile_and_run_buffer_handle)
set(CASE_FIXTURE positive_buffer_handle.simp)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/simp_compile_and_run_buffer_handle.stdout")
set(CASE_REQUIRE_GC_ROOTS [==[ON]==])
