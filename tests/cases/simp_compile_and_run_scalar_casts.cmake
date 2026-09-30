set(CASE_NAME simp_compile_and_run_scalar_casts)
set(CASE_FIXTURE positive_scalar_casts.simp)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/simp_compile_and_run_scalar_casts.stdout")
set(CASE_REQUIRE_GC_ROOTS [==[ON]==])
