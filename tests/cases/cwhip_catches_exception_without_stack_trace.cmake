set(CASE_NAME cwhip_catches_exception_without_stack_trace)
set(CASE_FIXTURE positive_caught_exception_no_stack_trace.cw)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/cwhip_catches_exception_without_stack_trace.stdout")
set(CASE_EXPECT_EMPTY_RUNTIME_STDERR [==[ON]==])
