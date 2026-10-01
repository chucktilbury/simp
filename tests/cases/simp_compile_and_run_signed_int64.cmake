set(CASE_NAME simp_compile_and_run_signed_int64)
set(CASE_FIXTURE positive_signed_int64.simp)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/simp_compile_and_run_signed_int64.stdout")
set(CASE_REQUIRE_GC_ROOTS [==[ON]==])
