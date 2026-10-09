set(CASE_NAME cwhip_rejects_double_destructor_call)
set(CASE_FIXTURE negative_double_destructor.cw)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/cwhip_rejects_double_destructor_call.stdout")
set(CASE_EXPECT_RUNTIME_FAILURE [==[ON]==])
