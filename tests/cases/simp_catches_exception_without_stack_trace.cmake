set(CASE_NAME simp_catches_exception_without_stack_trace)
set(CASE_FIXTURE positive_caught_exception_no_stack_trace.simp)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/simp_catches_exception_without_stack_trace.stdout")
set(CASE_EXPECT_EMPTY_RUNTIME_STDERR [==[ON]==])
