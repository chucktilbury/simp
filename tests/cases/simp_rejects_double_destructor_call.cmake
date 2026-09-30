set(CASE_NAME simp_rejects_double_destructor_call)
set(CASE_FIXTURE negative_double_destructor.simp)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/simp_rejects_double_destructor_call.stdout")
set(CASE_EXPECT_RUNTIME_FAILURE [==[ON]==])
