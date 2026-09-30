set(CASE_NAME simp_compile_and_run_explicit_destructor)
set(CASE_FIXTURE positive_explicit_destructor.simp)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/simp_compile_and_run_explicit_destructor.stdout")
set(CASE_EXPECT_WARNING "warning: explicit destructor call")
